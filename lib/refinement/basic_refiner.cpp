#include <algorithm>
#include <atomic>

#include "basic_refiner.h"
#include "../utils.h"

using namespace bathesis;

basic_refiner::basic_refiner(int imbalance, int imbalance_level)
        : refiner_interface(imbalance, imbalance_level) {
}

NodeID basic_refiner::perform_refinement_iteration(int nth_iteration, int imbalance) {
    m_partition_sizes = m_query_graph->count_partition_sizes();

    // TODO Question SG: how large is the fraction of changed gains in each iteration???
    // calculate gain value for every data node
    auto gains = calculate_gain_values();

    // split the gains into one list for each partition and sort it by gain value
    std::vector<NodeID> S[2];
    for (NodeID v = 0; v < m_data_graph->number_of_nodes(); ++v) {
        PartitionID p = m_data_graph->getPartitionIndex(v);
        S[p].push_back(v);
    }

    auto sort_by_gain = [&gains](auto &left, auto &right) -> bool { return gains[left] > gains[right]; };
    std::sort(S[0].begin(), S[0].end(), sort_by_gain);
    std::sort(S[1].begin(), S[1].end(), sort_by_gain);

    auto limit = std::min(S[0].size(), S[1].size());

    // find out whether the nodes are boundary nodes or not
    std::array<std::vector<bool>, 2> is_boundary;
    for (std::size_t i = 0; i < limit; ++i) {
        if (gains[S[0][i]] + gains[S[1][i]] <= 0) {
            break;
        }

        for (std::size_t partition = 0; partition < 2; ++partition) {
            is_boundary[partition].push_back(utils::is_boundary_node(*m_data_graph, S[partition][i]));
        }
    }

    // exchange pairs as long as the sum of their move costs is positive
    NodeID num_moved_nodes = 0;
    for (std::size_t i = 0; i < limit; ++i) {
        assert (m_data_graph->getPartitionIndex(S[0][i]) == 0 && m_data_graph->getPartitionIndex(S[1][i]));

        if (gains[S[0][i]] + gains[S[1][i]] <= 0) {
            break;
        }

        num_moved_nodes += 2;
        for (PartitionID partition = 0; partition < 2; ++partition) {
            NodeID v = S[partition][i];
            m_data_graph->setPartitionIndex(v, 1 - partition);
            m_reporter->refinement_move_node(*m_query_graph, v, partition, gains[v], 0, 0, is_boundary[partition][i]);
        }
    }

    return num_moved_nodes;
}

std::vector<double> basic_refiner::calculate_gain_values() {
    std::vector<double> gains(m_data_graph->number_of_nodes());
    std::array<double, 2> nonadjacent_base_cost = {0.0, 0.0};

    for (NodeID q = 0; q < m_query_graph->number_of_query_nodes(); ++q) {
        std::array<NodeID, 2> degrees = m_query_graph->count_query_node_degrees(q);

        double cost = calculate_node_cost(q);

        // cost difference when a neighbor is moved to another partition
        std::array<double, 2> adjacent_cost_contribution = {0.0, 0.0};

        // cost difference when a nonadjacent node is moved to another partition
        std::array<double, 2> nonadjacent_cost_contribution = {0.0, 0.0};

        if (degrees[0] > 0) {
            adjacent_cost_contribution[0] = cost - calculate_node_cost(
                    std::array<NodeID, 2>{m_partition_sizes[0] - 1, m_partition_sizes[1] + 1},
                    std::array<NodeID, 2>{degrees[0] - 1, degrees[1] + 1}
            );
        }
        if (degrees[1] > 0) {
            adjacent_cost_contribution[1] = cost - calculate_node_cost(
                    std::array<NodeID, 2>{m_partition_sizes[0] + 1, m_partition_sizes[1] - 1},
                    std::array<NodeID, 2>{degrees[0] + 1, degrees[1] - 1}
            );
        }
        if (m_partition_sizes[0] > 0 && degrees[0] < m_partition_sizes[0]) {
            nonadjacent_cost_contribution[0] = cost - calculate_node_cost(
                    std::array<NodeID, 2>{m_partition_sizes[0] - 1, m_partition_sizes[1] + 1},
                    degrees
            );
            nonadjacent_base_cost[0] += nonadjacent_cost_contribution[0];
        }
        if (m_partition_sizes[1] > 0 && degrees[1] < m_partition_sizes[1]) {
            nonadjacent_cost_contribution[1] = cost - calculate_node_cost(
                    std::array<NodeID, 2>{m_partition_sizes[0] + 1, m_partition_sizes[1] - 1},
                    degrees
            );
            nonadjacent_base_cost[1] += nonadjacent_cost_contribution[1];
        }

        for (EdgeID e = m_query_graph->get_first_edge(q); e < m_query_graph->get_first_invalid_edge(q); ++e) {
            NodeID v = m_query_graph->get_edge_target(e);
            PartitionID p = m_data_graph->getPartitionIndex(v);
            gains[v] += adjacent_cost_contribution[p] - nonadjacent_cost_contribution[p];
        }
    }

    for (NodeID v = 0; v < m_data_graph->number_of_nodes(); ++v) {
        PartitionID p = m_data_graph->getPartitionIndex(v);
        gains[v] += nonadjacent_base_cost[p];
    }

    return gains;
}

double basic_refiner::calculate_node_cost(NodeID node) {
    return calculate_node_cost(m_query_graph->count_query_node_degrees(node));
}

double basic_refiner::calculate_node_cost(const std::array<NodeID, 2> &degrees) {
    return calculate_node_cost(m_partition_sizes, degrees);
}

double
basic_refiner::calculate_node_cost(const std::array<NodeID, 2> &partition_sizes, const std::array<NodeID, 2> &degrees) {
    auto cost = 0.0;
    for (std::size_t i = 0; i < 2; ++i) {
        assert (degrees[i] <= partition_sizes[i]);

        if (partition_sizes[i] > 0) { // if partition_sizes[i] == 0, then degrees[i] == 0 and hence cost += 0
            cost += degrees[i] * utils::log(static_cast<double>(partition_sizes[i]) / (degrees[i] + 1));
        }
    }
    return cost;
}
