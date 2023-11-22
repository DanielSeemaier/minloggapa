#include "fm_refiner.h"
#include "../utils.h"

using namespace bathesis;

fm_refiner::fm_refiner(int imbalance, int imbalance_level)
        : refiner_interface(imbalance, imbalance_level) {
}

NodeID fm_refiner::perform_refinement_iteration(int nth_iteration, int imbalance) {
    m_partition_sizes = m_query_graph->count_partition_sizes();

    auto node_info = calculate_gain_values();
    auto &query_node_info = node_info.first;
    auto &data_node_info = node_info.second;

    NodeID empty = std::numeric_limits<NodeID>().max();
    std::array<NodeID, 2> max_gain_nodes{empty, empty};
    forall_nodes((*m_data_graph), v)
            if (data_node_info[v].marked) continue;

            PartitionID p = m_data_graph->getPartitionIndex(v);
            if (max_gain_nodes[p] == empty) {
                max_gain_nodes[p] = v;
                continue;
            }

            NodeID max = max_gain_nodes[p];

            if (data_node_info[max].gain + data_node_info[max].gain2 < data_node_info[v].gain + data_node_info[v].gain2) {
                max_gain_nodes[p] = v;
            }
    endfor

    // selected nodes in the order they were selected
    std::vector<NodeID> S;

    // selection strategy: if the imbalance constraint allows it, choose node with the biggest gain value;
    // if the balance constraint is too tight, choose node from the bigger partition
    // if one of the queue is empty, transfer nodes from the other one as long as the imbalance constraint allows it
    while (true) {
        NodeID v;

        double current_imbalance = std::abs(static_cast<long>(m_partition_sizes[0]) - static_cast<long>(m_partition_sizes[1]))
                                   / static_cast<double>((m_partition_sizes[0] + m_partition_sizes[1]));

        NodeID m0 = max_gain_nodes[0];
        NodeID m1 = max_gain_nodes[1];
        if (max_gain_nodes[0] != empty && max_gain_nodes[1] != empty) {
            if (current_imbalance * 100 < imbalance) { // balance constraint allows us to choose from any queue
                if (data_node_info[m0].gain + data_node_info[m0].gain2 < data_node_info[m1].gain + data_node_info[m1].gain2) {
                    v = m1;
                } else {
                    v = m0;
                }
            } else { // balance constraint too tight, improve balance
                if (m_partition_sizes[0] < m_partition_sizes[1]) {
                    v = m1;
                } else {
                    v = m0;
                }
            }
        } else if (max_gain_nodes[0] != empty && (current_imbalance * 100 < imbalance || m_partition_sizes[1] < m_partition_sizes[0])) {
            v = m0;
        } else if (max_gain_nodes[1] != empty && (current_imbalance * 100 < imbalance || m_partition_sizes[0] < m_partition_sizes[1])) {
            v = m1;
        } else {
            break;
        }

        S.push_back(v);
        update_gain_values(query_node_info, data_node_info, v);

        max_gain_nodes[0] = empty;
        max_gain_nodes[1] = empty;
        forall_nodes((*m_data_graph), v)
                if (data_node_info[v].marked) continue;

                PartitionID p = m_data_graph->getPartitionIndex(v);
                if (max_gain_nodes[p] == empty) {
                    max_gain_nodes[p] = v;
                    continue;
                }

                NodeID max = max_gain_nodes[p];

                if (data_node_info[max].gain + data_node_info[max].gain2 < data_node_info[v].gain + data_node_info[v].gain2) {
                    max_gain_nodes[p] = v;
                }
        endfor

        // TODO remove
        if (S.size() % 1000 == 0) {
            std::cout << "nth_node_calculated = " << S.size() << "; processed edges so far: "
                      << m_data_graph->get_first_invalid_edge(static_cast<NodeID>(S.size() - 1)) << std::endl;
        }
    }

    // TODO remove
    std::cout << "S filled" << std::endl;

    // find maximal prefix sum of S
    std::size_t max_k = 0; // swap 0..max_k for maximal gain
    auto max_value = std::numeric_limits<double>().lowest(); // maximal cost improvement
    auto sum = 0.0;
    for (std::size_t k = 0; k < S.size(); ++k) {
        sum += data_node_info[S[k]].gain;

        if (sum > max_value) {
            max_value = sum;
            max_k = k;
        }
    }

    if (max_value > 0.01) {
        auto pre_iteration_cost = utils::calculate_partition_cost(*m_query_graph); // assert

        // perform swaps
        for (std::size_t i = 0; i <= max_k; ++i) {
            NodeID u = S[i];
            PartitionID p = m_data_graph->getPartitionIndex(u);
            bool is_boundary = utils::is_boundary_node(*m_data_graph, u);

            m_data_graph->setPartitionIndex(u, 1 - p);
            m_reporter->refinement_move_node(*m_query_graph, u, p, data_node_info[u].gain,
                                             data_node_info[u].gain - data_node_info[u].gain2, data_node_info[u].gain2,
                                             is_boundary);
        }

        auto post_iteration_cost = utils::calculate_partition_cost(*m_query_graph); // assert
        assert (std::abs(pre_iteration_cost - post_iteration_cost - max_value) < 0.05);

        return 2 * (static_cast<NodeID>(max_k) + 1);
    }

    return 0;
}

std::pair<std::vector<query_node_info>, std::vector<data_node_info>> fm_refiner::calculate_gain_values() {
    std::vector<query_node_info> query_node_info(m_query_graph->number_of_query_nodes());
    std::vector<data_node_info> data_node_info(m_data_graph->number_of_nodes());

    m_nonadjacent_base_cost[0] = 0.0;
    m_nonadjacent_base_cost[1] = 0.0;
    m_partition_edges[0] = 0;
    m_partition_edges[1] = 0;

    forall_nodes((*m_data_graph), v)
            data_node_info[v].node = v;
            data_node_info[v].gain = 0.0;
            data_node_info[v].gain2 = 0.0;
            data_node_info[v].marked = false;
    endfor

    for (NodeID q = 0; q < m_query_graph->number_of_query_nodes(); ++q) {
        query_node_info[q].node = q;
        query_node_info[q].degrees = m_query_graph->count_query_node_degrees(q);
        query_node_info[q].cost = calculate_node_cost(query_node_info[q].degrees);
        query_node_info[q].adjacent_node_contribution = {0.0, 0.0};

        // convenience references to make the code look cleaner
        auto &degrees = query_node_info[q].degrees;
        auto &cost = query_node_info[q].cost;
        auto &adjacent_node_contribution = query_node_info[q].adjacent_node_contribution;

        if (degrees[0] > 0) {
            adjacent_node_contribution[0] = 0.0;
            adjacent_node_contribution[0] -= degrees[0] * utils::log(degrees[0] + 1);
            adjacent_node_contribution[0] -= degrees[1] * utils::log(degrees[1] + 1);
            adjacent_node_contribution[0] += (degrees[0] - 1) * utils::log(degrees[0]);
            adjacent_node_contribution[0] += (degrees[1] + 1) * utils::log(degrees[1] + 2);
        }
        if (degrees[1] > 0) {
            adjacent_node_contribution[1] = 0.0;
            adjacent_node_contribution[1] -= degrees[0] * utils::log(degrees[0] + 1);
            adjacent_node_contribution[1] -= degrees[1] * utils::log(degrees[1] + 1);
            adjacent_node_contribution[1] += (degrees[0] + 1) * utils::log(degrees[0] + 2);
            adjacent_node_contribution[1] += (degrees[1] - 1) * utils::log(degrees[1]);
        }

        for (EdgeID e = m_query_graph->get_first_edge(q); e < m_query_graph->get_first_invalid_edge(q); ++e) {
            NodeID v = m_query_graph->get_edge_target(e);
            PartitionID p = m_data_graph->getPartitionIndex(v);

            data_node_info[v].gain += adjacent_node_contribution[p];

            ++m_partition_edges[p];
        }
    }

    forall_nodes((*m_data_graph), v)
            std::size_t adj = m_query_graph->get_adjacent_query_nodes(v).size();

            if (m_data_graph->getPartitionIndex(v) == 0) {
                assert (m_partition_edges[0] >= adj);
                assert (m_partition_sizes[0] > 0);

                data_node_info[v].gain2 = 0.0;
                data_node_info[v].gain2 += m_partition_edges[0] * (utils::log(m_partition_sizes[0]) + 1);
                if (m_partition_sizes[1] > 0) {
                    data_node_info[v].gain2 += m_partition_edges[1] * (utils::log(m_partition_sizes[1]) + 1);
                }
                if (m_partition_sizes[0] > 1) {
                    data_node_info[v].gain2 -=
                            (m_partition_edges[0] - adj) * (utils::log(m_partition_sizes[0] - 1) + 1);
                }
                data_node_info[v].gain2 -= (m_partition_edges[1] + adj) * (utils::log(m_partition_sizes[1] + 1) + 1);
            } else {
                assert (m_partition_edges[1] >= adj);
                assert (m_partition_sizes[1] > 0);

                data_node_info[v].gain2 = 0.0;
                if (m_partition_sizes[0] > 0) {
                    data_node_info[v].gain2 += m_partition_edges[0] * (utils::log(m_partition_sizes[0]) + 1);
                }
                data_node_info[v].gain2 += m_partition_edges[1] * (utils::log(m_partition_sizes[1]) + 1);
                data_node_info[v].gain2 -= (m_partition_edges[0] + adj) * (utils::log(m_partition_sizes[0] + 1) + 1);
                if (m_partition_sizes[1] > 1) {
                    data_node_info[v].gain2 -=
                            (m_partition_edges[1] - adj) * (utils::log(m_partition_sizes[1] - 1) + 1);
                }
            }

            assert(!std::isnan(data_node_info[v].gain2));
    endfor

    return {query_node_info, data_node_info};
}

void fm_refiner::update_gain_values(std::vector<query_node_info> &query_node_info,
                                    std::vector<data_node_info> &data_node_info, NodeID node) {
    assert (!data_node_info[node].marked);

    auto partition = m_data_graph->getPartitionIndex(node);
    data_node_info[node].marked = true;
    data_node_info[node].gain += data_node_info[node].gain2;

    auto adjacent_query_nodes = m_query_graph->get_adjacent_query_nodes(node);

    assert (m_partition_sizes[partition] > 0);
    --m_partition_sizes[partition];
    ++m_partition_sizes[1 - partition];

    assert (m_partition_edges[partition] >= adjacent_query_nodes.size());
    m_partition_edges[partition] -= adjacent_query_nodes.size();
    m_partition_edges[1 - partition] += adjacent_query_nodes.size();

    // O(MaxDegree(QG)^2)
    for (NodeID q : adjacent_query_nodes) {
        auto &degrees = query_node_info[q].degrees;
        auto &adjacent_node_contribution = query_node_info[q].adjacent_node_contribution;

        assert (degrees[partition] > 0);
        --degrees[partition];
        ++degrees[1 - partition];

        std::array<double, 2> new_adjacent_node_contribution = {0.0, 0.0};

        if (degrees[0] > 0) {
            new_adjacent_node_contribution[0] = 0.0;
            new_adjacent_node_contribution[0] -= degrees[0] * utils::log(degrees[0] + 1);
            new_adjacent_node_contribution[0] -= degrees[1] * utils::log(degrees[1] + 1);
            new_adjacent_node_contribution[0] += (degrees[0] - 1) * utils::log(degrees[0]);
            new_adjacent_node_contribution[0] += (degrees[1] + 1) * utils::log(degrees[1] + 2);
        }
        if (degrees[1] > 0) {
            new_adjacent_node_contribution[1] = 0.0;
            new_adjacent_node_contribution[1] -= degrees[0] * utils::log(degrees[0] + 1);
            new_adjacent_node_contribution[1] -= degrees[1] * utils::log(degrees[1] + 1);
            new_adjacent_node_contribution[1] += (degrees[0] + 1) * utils::log(degrees[0] + 2);
            new_adjacent_node_contribution[1] += (degrees[1] - 1) * utils::log(degrees[1]);
        }

        for (EdgeID edge = m_query_graph->get_first_edge(q); edge < m_query_graph->get_first_invalid_edge(q); ++edge) {
            NodeID v = m_query_graph->get_edge_target(edge);
            PartitionID p = m_data_graph->getPartitionIndex(v);

            if (!data_node_info[v].marked) {
                data_node_info[v].gain -= adjacent_node_contribution[p];
                data_node_info[v].gain += new_adjacent_node_contribution[p];
            }
        }

        adjacent_node_contribution[0] = new_adjacent_node_contribution[0];
        adjacent_node_contribution[1] = new_adjacent_node_contribution[1];
    }

    double nonadjacent_cost = 0.0;
    if (m_partition_sizes[0] > 0) {
        nonadjacent_cost += m_partition_edges[0] * (utils::log(m_partition_sizes[0]) + 1);
    }
    if (m_partition_sizes[1] > 0) {
        nonadjacent_cost += m_partition_edges[1] * (utils::log(m_partition_sizes[1]) + 1);
    }

    // O(|V|)
    #pragma omp parallel for
    for (NodeID v = 0; v < m_data_graph->number_of_nodes(); ++v) {
        if (data_node_info[v].marked) continue;

        auto p = m_data_graph->getPartitionIndex(v);
        auto number_of_adjacent_query_nodes = m_query_graph->get_number_of_adjacent_query_nodes(v); // O(level)

        assert (m_partition_edges[p] >= number_of_adjacent_query_nodes);
        assert (m_partition_sizes[p] > 0);

        data_node_info[v].gain2 = nonadjacent_cost;

        if (m_data_graph->getPartitionIndex(v) == 0) {
            if (m_partition_sizes[0] > 1) {
                data_node_info[v].gain2 -= (m_partition_edges[0] - number_of_adjacent_query_nodes) * (utils::log(m_partition_sizes[0] - 1) + 1);
            }
            data_node_info[v].gain2 -= (m_partition_edges[1] + number_of_adjacent_query_nodes) * (utils::log(m_partition_sizes[1] + 1) + 1);
        } else {
            data_node_info[v].gain2 -= (m_partition_edges[0] + number_of_adjacent_query_nodes) * (utils::log(m_partition_sizes[0] + 1) + 1);
            if (m_partition_sizes[1] > 1) {
                data_node_info[v].gain2 -= (m_partition_edges[1] - number_of_adjacent_query_nodes) * (utils::log(m_partition_sizes[1] - 1) + 1);
            }
        }
    }
}

double fm_refiner::calculate_node_cost(const std::array<NodeID, 2> &degrees) {
    return calculate_node_cost(m_partition_sizes, degrees);
}

double fm_refiner::calculate_node_cost(const std::array<NodeID, 2> &partition_sizes,
                                       const std::array<NodeID, 2> &degrees) {
    auto cost = 0.0;

    for (std::size_t i = 0; i < 2; ++i) {
        assert (degrees[i] <= partition_sizes[i]);

        // if partition_sizes[i] == 0, then degrees[i] == 0 and hence cost += 0
        if (partition_sizes[i] > 0) {
            cost += degrees[i] * utils::log(static_cast<double>(partition_sizes[i]) / (degrees[i] + 1));
        }
    }

    return cost;
}