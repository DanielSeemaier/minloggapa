#include "fm_refiner_quadtree.h"
#include "../utils.h"

using namespace bathesis;

fm_refiner_quadtree::fm_refiner_quadtree(int imbalance, int imbalance_level)
        : refiner_interface(imbalance, imbalance_level) {
}

NodeID fm_refiner_quadtree::perform_refinement_iteration(int nth_iteration, int imbalance) {
    auto node_info = init_partition_info();

    std::array<maxNodeHeap, 2> queues;
    forall_nodes((*m_data_graph), v)
            PartitionID p = m_data_graph->getPartitionIndex(v);
            queues[p].insert(v, node_info[v].gain);
    endfor

    // selected nodes in the order they were selected
    std::vector<NodeID> S;

    // selection strategy: choose queues alternatively
    auto limit = std::min(queues[0].size(), queues[1].size());
    auto old_partition = utils::get_partition(*m_data_graph);
    for (std::size_t k = 0; k < 2 * limit; ++k) {
        PartitionID p = static_cast<PartitionID>(k % 2); // select queues alternatively
        NodeID v = queues[p].deleteMax();

        // moves v to the other partition and updates the gain value of all unmarked nodes in node_info
        move_and_update(v, node_info); // also marks v
        S.push_back(v);

        // update keys in the priority queues
        for (NodeID u = 0; u < m_data_graph->number_of_nodes(); ++u) {
            if (node_info[u].marked) continue;
            queues[m_data_graph->getPartitionIndex(u)].changeKey(u, node_info[u].gain);
        }
    }
    utils::set_partition(*m_data_graph, old_partition); // restore initial partition

    // find maximal prefix sum of S
    std::size_t max_k = 0; // swap 0..max_k for maximal gain
    auto max_value = std::numeric_limits<double>().lowest(); // maximal cost improvement
    auto sum = 0.0;
    for (std::size_t k = 0; k < S.size(); ++k) {
        sum += node_info[S[k]].gain;

        if (sum > max_value) {
            max_value = sum;
            max_k = k;
        }
    }

    if (max_value > 0) {
        init_partition_info();
        auto pre_iteration_cost = evaluate_cost_function(); // assert

        // perform swaps
        for (std::size_t i = 0; i <= max_k; ++i) {
            NodeID u = S[i];
            PartitionID p = m_data_graph->getPartitionIndex(u);
            bool is_boundary = utils::is_boundary_node(*m_data_graph, u);

            m_data_graph->setPartitionIndex(u, 1 - p);
            m_reporter->refinement_move_node(*m_query_graph, u, p, node_info[u].gain, 0, 0, is_boundary);
        }

        init_partition_info();
        auto post_iteration_cost = evaluate_cost_function(); // assert
        //std::cout << "pre_iteration_cost = " << pre_iteration_cost << std::endl;
        //std::cout << "post_iteration_cost = " << post_iteration_cost << std::endl;
        //std::cout << "max_value = " << max_value << std::endl;
        //std::cout << "error = " << std::abs(pre_iteration_cost - post_iteration_cost - max_value) << std::endl;
        assert (std::abs(pre_iteration_cost - post_iteration_cost - max_value) < 0.05);

        return static_cast<NodeID>(max_k) + 1;
    }

    return 0;
}

void fm_refiner_quadtree::move_and_update(NodeID node, std::vector<node_info> &nodes) {
    PartitionID old_partition = m_data_graph->getPartitionIndex(node);
    PartitionID new_partition = 1 - old_partition;
    m_data_graph->setPartitionIndex(node, new_partition);
    nodes[node].marked = true;

    // update m_partition_sizes
    --m_partition_sizes[old_partition];
    ++m_partition_sizes[new_partition];

    // update m_num_edges_from_to
    for (EdgeID e = m_data_graph->get_first_edge(node); e < m_data_graph->get_first_invalid_edge(node); ++e) {
        NodeID u = m_data_graph->getEdgeTarget(e);
        PartitionID p = m_data_graph->getPartitionIndex(u);

        --m_num_edges_from_to[old_partition][p];
        ++m_num_edges_from_to[new_partition][p];
        --m_num_edges_from_to[p][old_partition]; // assuming m_data_graph is undirected
        ++m_num_edges_from_to[p][new_partition]; // assuming m_data_graph is undirected
        --nodes[u].num_edges_to[old_partition]; // assuming m_data_graph is undirected
        ++nodes[u].num_edges_to[new_partition]; // assuming m_data_graph is undirected
    }

    update_gain_values(nodes);
}

/**
 * Updates the node_info::gain member after other members of the data structure were changed (or m_partition_sizes or
 * m_num_edges_from_to).
 *
 * @param nodes
 */
void fm_refiner_quadtree::update_gain_values(std::vector<node_info> &nodes) {
    double old_cost = evaluate_cost_function();

    for (NodeID v = 0; v < m_data_graph->number_of_nodes(); ++v) {
        if (nodes[v].marked) continue;

        PartitionID old_p = m_data_graph->getPartitionIndex(v);
        PartitionID new_p = 1 - old_p;

        assert (m_partition_sizes[old_p] > 0);
        --m_partition_sizes[old_p];
        ++m_partition_sizes[new_p];

        // this node
        assert (m_num_edges_from_to[old_p][old_p] >= nodes[v].num_edges_to[old_p]);
        assert (m_num_edges_from_to[old_p][new_p] >= nodes[v].num_edges_to[new_p]);
        m_num_edges_from_to[old_p][old_p] -= nodes[v].num_edges_to[old_p];
        m_num_edges_from_to[old_p][new_p] -= nodes[v].num_edges_to[new_p];
        m_num_edges_from_to[new_p][old_p] += nodes[v].num_edges_to[old_p];
        m_num_edges_from_to[new_p][new_p] += nodes[v].num_edges_to[new_p];

        // this node's neighbors -- assuming that m_data_graph is undirected
        assert (m_num_edges_from_to[old_p][old_p] >= nodes[v].num_edges_to[old_p]);
        m_num_edges_from_to[old_p][old_p] -= nodes[v].num_edges_to[old_p];
        m_num_edges_from_to[old_p][new_p] += nodes[v].num_edges_to[old_p];
        assert (m_num_edges_from_to[new_p][old_p] >= nodes[v].num_edges_to[new_p]);
        m_num_edges_from_to[new_p][old_p] -= nodes[v].num_edges_to[new_p];
        m_num_edges_from_to[new_p][new_p] += nodes[v].num_edges_to[new_p];

        double new_cost = evaluate_cost_function();

        m_num_edges_from_to[new_p][new_p] -= nodes[v].num_edges_to[new_p];
        m_num_edges_from_to[new_p][old_p] -= nodes[v].num_edges_to[old_p];
        m_num_edges_from_to[old_p][new_p] += nodes[v].num_edges_to[new_p];
        m_num_edges_from_to[old_p][old_p] += nodes[v].num_edges_to[old_p];

        m_num_edges_from_to[new_p][new_p] -= nodes[v].num_edges_to[new_p];
        m_num_edges_from_to[new_p][old_p] += nodes[v].num_edges_to[new_p];
        m_num_edges_from_to[old_p][new_p] -= nodes[v].num_edges_to[old_p];
        m_num_edges_from_to[old_p][old_p] += nodes[v].num_edges_to[old_p];

        --m_partition_sizes[new_p];
        ++m_partition_sizes[old_p];

        nodes[v].gain = old_cost - new_cost;
    }
}

/**
 * Evaluates the cost function in O(1) after update_partition_info_members() was called
 *
 * @return
 */
double fm_refiner_quadtree::evaluate_cost_function() {
    double cost = 0.0;
    cost += approx_quarter_cost(0, 0);
    cost += approx_quarter_cost(0, 1);
    cost += approx_quarter_cost(1, 0);
    cost += approx_quarter_cost(1, 1);
    return cost;
}

double fm_refiner_quadtree::approx_log_faculty(double n) { // stirling formula: log of faculty of n
    return (1.0 / std::log(2)) * (0.5 * std::log(2 * M_PI * n) + n * std::log(n / M_E));
}

double fm_refiner_quadtree::approx_log_binom(double n, double k) { // log of binomial coefficient using stirling formula
    return approx_log_faculty(n) - approx_log_faculty(k) - approx_log_faculty(n - k);
}

double fm_refiner_quadtree::approx_quarter_cost(std::size_t from, std::size_t to) {
    return approx_log_binom(m_partition_sizes[from] * m_partition_sizes[to], m_num_edges_from_to[from][to]);
    //if (from == 0 && to == 0) return approx_log_binom(m_partition_sizes[0] * m_partition_sizes[0], m_num_edges_from_to[0][0] + m_num_edges_from_to[0][1]);
    //if (from == 0 && to == 1) return approx_log_binom(m_partition_sizes[0] * m_partition_sizes[0], m_num_edges_from_to[1][0] + m_num_edges_from_to[1][1]);
    //if (from == 1 && to == 0) return approx_log_binom(m_partition_sizes[1] * m_partition_sizes[1], m_num_edges_from_to[0][0] + m_num_edges_from_to[0][1]);
    //if (from == 1 && to == 1) return approx_log_binom(m_partition_sizes[1] * m_partition_sizes[1], m_num_edges_from_to[1][0] + m_num_edges_from_to[1][1]);
}

/**
 * Updates the values of m_num_edges_from_to and m_partition_sizes in O(m)
 */
std::vector<node_info> fm_refiner_quadtree::init_partition_info() {
    std::vector<node_info> nodes(m_data_graph->number_of_nodes());

    m_num_edges_from_to[0][0] = 0;
    m_num_edges_from_to[0][1] = 0;
    m_num_edges_from_to[1][0] = 0;
    m_num_edges_from_to[1][1] = 0;
    m_partition_sizes[0] = 0;
    m_partition_sizes[1] = 0;

    for (NodeID v = 0; v < m_data_graph->number_of_nodes(); ++v) {
        ++m_partition_sizes[m_data_graph->getPartitionIndex(v)];

        nodes[v].node = v;
        nodes[v].marked = false;
        nodes[v].num_edges_to[0] = 0;
        nodes[v].num_edges_to[1] = 0;

        for (EdgeID e = m_data_graph->get_first_edge(v); e < m_data_graph->get_first_invalid_edge(v); ++e) {
            NodeID u = m_data_graph->getEdgeTarget(e);
            PartitionID p = m_data_graph->getPartitionIndex(u);
            ++nodes[v].num_edges_to[p];
            ++m_num_edges_from_to[m_data_graph->getPartitionIndex(v)][p];
        }
    }

    update_gain_values(nodes);
    return nodes;
}
