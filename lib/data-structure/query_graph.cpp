#include "query_graph.h"

using namespace bathesis;

query_graph::query_graph() {
    m_is_constructing = false;
    m_last_source_id = 0;
    m_parent = this;
}

void query_graph::construct_query_edges() {
    start_construction();

    forall_nodes(m_data_graph, node_id)
            forall_out_edges(m_data_graph, edge_id, node_id)
                    NodeID neighbor_id = m_data_graph.getEdgeTarget(edge_id);
                    add_query_edge(node_id, neighbor_id);
            endfor
    endfor

    finish_construction();
}

void query_graph::start_construction() {
    start_construction(m_data_graph.number_of_nodes());
}

void query_graph::start_construction(NodeID number_of_query_nodes) {
    assert(number_of_query_nodes >= m_data_graph.number_of_nodes());
    assert(!m_is_constructing);

    m_is_constructing = true;
    m_query_nodes.resize(number_of_query_nodes + 1);
}

void query_graph::add_query_edge(NodeID source_id, NodeID target_id) {
    assert(m_is_constructing);
    assert(source_id < number_of_query_nodes());
    assert(target_id < m_data_graph.number_of_nodes());
    assert(m_last_source_id <= source_id);

    m_query_edges.push_back(target_id);
    EdgeID next_edge_id = static_cast<EdgeID>(m_query_edges.size());
    m_query_nodes[source_id + 1] = next_edge_id;

    if (m_last_source_id + 1 < source_id) {
        for (NodeID i = source_id; i > m_last_source_id + 1; --i) {
            m_query_nodes[i] = m_query_nodes[m_last_source_id + 1];
        }
    }
    m_last_source_id = source_id;
}

void query_graph::finish_construction() {
    assert(m_is_constructing);

    if (m_last_source_id != m_query_nodes.size() - 1) {
        for (NodeID i = static_cast<NodeID>(m_query_nodes.size()) - 1; i > m_last_source_id + 1; --i) {
            m_query_nodes[i] = m_query_nodes[m_last_source_id + 1];
        }
    }

    m_is_constructing = false;
}

std::array<std::vector<NodeID>, 2>
query_graph::build_partition_induced_subgraphs(std::array<query_graph, 2> &subgraphs) {
    std::array<NodeID, 2> number_of_data_nodes = {0, 0};
    std::array<EdgeID, 2> number_of_data_edges = {0, 0};

    // Step 1: Count the number of data nodes and edges in each partition
    forall_nodes(m_data_graph, node_id)
            PartitionID partition_id = m_data_graph.getPartitionIndex(node_id);
            ++number_of_data_nodes[partition_id];

            forall_out_edges(m_data_graph, edge_id, node_id)
                    NodeID neighbor_id = m_data_graph.getEdgeTarget(edge_id);
                    if (partition_id == m_data_graph.getPartitionIndex(neighbor_id)) {
                        ++number_of_data_edges[partition_id];
                    }
            endfor
    endfor

    // Step 2: Construct the data graphs
    // When we construct new data graphs, the node ids change as the number of nodes reduces in both subgraphs
    // These vectors are used to keep track of those changes
    std::vector<NodeID> map_old_to_new(m_data_graph.number_of_nodes()); // map_old_to_new[old id] = new id
    std::array<std::vector<NodeID>, 2> map_new_to_old = {               // map_new_to_old[partition][new id] = old id
            std::vector<NodeID>(number_of_data_nodes[0]),
            std::vector<NodeID>(number_of_data_nodes[1])
    };
    for (int i = 0; i < 2; ++i) {
        subgraphs[i].data_graph().start_construction(number_of_data_nodes[i], number_of_data_edges[i]);
        subgraphs[i].start_construction(number_of_query_nodes());
    }

    // Step 2.1: Construct the induced data graphs
    // Step 2.1.1: Construct the map arrays, i.e. the translation from new to old ids and vice versa
    forall_nodes(m_data_graph, node_id)
            PartitionID partition_id = m_data_graph.getPartitionIndex(node_id);

            NodeID new_node_id = subgraphs[partition_id].data_graph().new_node();
            subgraphs[partition_id].data_graph().setPartitionIndex(new_node_id, 0);
            subgraphs[partition_id].data_graph().setNodeWeight(new_node_id, 1);

            map_old_to_new[node_id] = new_node_id;
            map_new_to_old[partition_id][new_node_id] = node_id;

            assert(new_node_id < number_of_data_nodes[partition_id]);
    endfor

    // Step 2.1.2: Actual graph construction
    forall_nodes(m_data_graph, node_id)
            PartitionID partition_id = m_data_graph.getPartitionIndex(node_id);

            forall_out_edges(m_data_graph, edge_id, node_id)
                    NodeID neighbor_id = m_data_graph.getEdgeTarget(edge_id);

                    // Omit cut edges in the data graph; we will add them between query and data nodes
                    if (partition_id != m_data_graph.getPartitionIndex(neighbor_id)) {
                        continue;
                    }

                    EdgeID new_edge_id = subgraphs[partition_id].data_graph().new_edge(
                            map_old_to_new[node_id],
                            map_old_to_new[neighbor_id]
                    );
                    subgraphs[partition_id].data_graph().setEdgeWeight(new_edge_id, 1);
            endfor
    endfor

    // Step 3: Add the edges between query and data nodes respecting the new node ids
    for (NodeID node_id = 0; node_id < number_of_query_nodes(); ++node_id) {
        for (EdgeID edge_id = m_query_nodes[node_id]; edge_id < m_query_nodes[node_id + 1]; ++edge_id) {
            NodeID neighbor_id = m_query_edges[edge_id];
            PartitionID partition_id = m_data_graph.getPartitionIndex(neighbor_id);
            subgraphs[partition_id].add_query_edge(node_id, map_old_to_new[neighbor_id]);
        }
    }

    for (int i = 0; i < 2; ++i) {
        subgraphs[i].finish_construction();
        subgraphs[i].data_graph().finish_construction();

        // TODO q&d solution to get adjacent query nodes of a data node
        subgraphs[i].m_parent = this;
        subgraphs[i].m_map_to_parent = map_new_to_old[i];
    }

    // Validate result with some basic sanity checks
    assert(number_of_query_nodes() == subgraphs[0].number_of_query_nodes());
    assert(number_of_query_nodes() == subgraphs[1].number_of_query_nodes());
    assert(number_of_query_edges() == subgraphs[0].number_of_query_edges() + subgraphs[1].number_of_query_edges());
    assert(m_data_graph.number_of_nodes()
           == subgraphs[0].data_graph().number_of_nodes() + subgraphs[1].data_graph().number_of_nodes());
    assert(m_data_graph.number_of_edges()
           >= subgraphs[0].data_graph().number_of_edges() + subgraphs[1].data_graph().number_of_edges());

    return map_new_to_old;
}

/**
 * Counts the number of data nodes in each partition.
 *
 * @return
 */
std::array<NodeID, 2> query_graph::count_partition_sizes() {
    std::array<NodeID, 2> sizes = {0, 0};
    forall_nodes(m_data_graph, node_id)
            ++sizes[m_data_graph.getPartitionIndex(node_id)];
    endfor
    return sizes;
}

/**
 * Counts the number of a query node's neighbors in each partition.
 *
 * @return
 */
std::array<NodeID, 2> query_graph::count_query_node_degrees(NodeID node_id) {
    std::array<NodeID, 2> degrees = {0, 0};
    for (EdgeID edge_id = get_first_edge(node_id); edge_id < get_first_invalid_edge(node_id); ++edge_id) {
        NodeID neighbor_id = get_edge_target(edge_id);
        ++degrees[m_data_graph.getPartitionIndex(neighbor_id)];
    }
    return degrees;
}

NodeID query_graph::number_of_query_nodes() {
    return static_cast<NodeID>(m_query_nodes.size()) - 1;
}

EdgeID query_graph::number_of_query_edges() {
    return static_cast<EdgeID>(m_query_edges.size());
}

graph_access &query_graph::data_graph() {
    return m_data_graph;
}

EdgeID query_graph::get_first_edge(NodeID node_id) {
    assert(node_id < number_of_query_nodes());

    return m_query_nodes[node_id];
}

EdgeID query_graph::get_first_invalid_edge(NodeID node_id) {
    assert(node_id < number_of_query_nodes());

    return m_query_nodes[node_id + 1];
}

std::vector<NodeID> query_graph::get_adjacent_query_nodes(NodeID data_node_id) {
    if (m_parent != this) {
        return m_parent->get_adjacent_query_nodes(m_map_to_parent[data_node_id]);
    }

    std::vector<NodeID> adjacent_query_nodes;
    forall_out_edges(data_graph(), edge, data_node_id)
            adjacent_query_nodes.push_back(data_graph().getEdgeTarget(edge));
    endfor
    return adjacent_query_nodes;
}

std::size_t query_graph::get_number_of_adjacent_query_nodes(NodeID data_node_id) {
    if (m_parent != this) {
        return m_parent->get_number_of_adjacent_query_nodes(m_map_to_parent[data_node_id]);
    }

    return data_graph().get_first_invalid_edge(data_node_id) - data_graph().get_first_edge(data_node_id);
}

NodeID query_graph::get_edge_target(EdgeID edge_id) {
    assert(edge_id < number_of_query_edges());

    return m_query_edges[edge_id];
}
