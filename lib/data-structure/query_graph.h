#ifndef IMPL_BIPARTITE_GRAPH_H
#define IMPL_BIPARTITE_GRAPH_H

#include <data_structure/graph_access.h>
#include <array>

namespace bathesis {

    /**
     * Wrapper around {@code graph_access} that manages query nodes.
     *
     * To use this class, first load the data graph using {@code graph_io::readGraphWeighted(G.data_graph(), "...")},
     * then either add query nodes manually using {@code start_construction()}, {@code add_query_edge()} and
     * {@code finish_construction()} or use {@code extract_query_edges()} to automatically construct a query node for
     * every data node.
     *
     * This class only offers access to query nodes; use {@code data_graph()} to gain access to the underlying data
     * graph.
     */
    class query_graph {
        query_graph *m_parent; // q&d solution to get the query neighbors of a data node
        graph_access m_data_graph;
        std::vector<EdgeID> m_query_nodes; // m_query_nodes[node id] = first edge id
        std::vector<NodeID> m_query_edges; // m_query_edges[edge id] = target node id
        std::vector<NodeID> m_map_to_parent;

        // construction
        bool m_is_constructing;
        NodeID m_last_source_id;

    public:
        query_graph();

        void start_construction();

        void start_construction(NodeID number_of_query_nodes);

        void add_query_edge(NodeID source_id, NodeID target_id);

        void construct_query_edges();

        void finish_construction();

        std::array<std::vector<NodeID>, 2> build_partition_induced_subgraphs(std::array<query_graph, 2> &subgraphs);

        std::array<NodeID, 2> count_partition_sizes();

        std::array<NodeID, 2> count_query_node_degrees(NodeID node_id);

        NodeID number_of_query_nodes();

        EdgeID number_of_query_edges();

        EdgeID get_first_edge(NodeID node_id);

        EdgeID get_first_invalid_edge(NodeID node_id);

        NodeID get_edge_target(EdgeID edge_id);

        std::vector<NodeID> get_adjacent_query_nodes(NodeID data_node_id);

        std::size_t get_number_of_adjacent_query_nodes(NodeID data_node_id);

        graph_access &data_graph();
    };
}


#endif // IMPL_BIPARTITE_GRAPH_H
