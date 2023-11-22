#include <algorithm>
#include <functional>
#include <cmath>

#include <sdsl/io.hpp>
#include <sdsl/int_vector.hpp>
#include <sdsl/vlc_vector.hpp>
#include <io/graph_io.h>
#include <tools/quality_metrics.h>

#include "utils.h"

using namespace bathesis;

/**
 * Log function that gives the number of bits needed to store the argument.
 *
 * @param arg
 * @return
 */
double utils::log(double arg) {
    return 1 + std::log2(arg);
}

/**
 * Builds the identity linear layout.
 *
 * @param G
 * @return caller-owned
 */
std::vector<NodeID> utils::create_identity_layout(graph_access &G) {
    std::vector<NodeID> id(G.number_of_nodes());
    std::iota(id.begin(), id.end(), 0);
    return id;
}

std::vector<NodeID> utils::create_random_layout(graph_access &G) {
    std::vector<NodeID> id = create_identity_layout(G);
    std::random_shuffle(id.begin(), id.end());
    return id;
}

std::vector<NodeID> utils::invert_linear_layout(std::vector<NodeID> inverted_linear_layout) {
    std::vector<NodeID> linear_layout(inverted_linear_layout.size());

    for (NodeID i = 0; i < inverted_linear_layout.size(); ++i) {
        linear_layout[i] = UINT_MAX;
    }
    for (NodeID i = 0; i < inverted_linear_layout.size(); ++i) {
        assert (linear_layout[inverted_linear_layout[i]] == UINT_MAX);
        linear_layout[inverted_linear_layout[i]] = i;
    }
    for (NodeID i = 0; i < inverted_linear_layout.size(); ++i) {
        assert (linear_layout[i] != UINT_MAX);
    }

    return linear_layout;
}

double utils::calculate_loggap(graph_access &G, const std::vector<NodeID> &linear_layout) {
    auto cost = 0.0;
    auto gaps = 0L;

    for (NodeID node_id = 0; node_id < G.number_of_nodes(); ++node_id) {
        std::vector<NodeID> neighbors;

        for (EdgeID e = G.get_first_edge(node_id); e < G.get_first_invalid_edge(node_id); ++e) {
            NodeID t = G.getEdgeTarget(e);
            neighbors.push_back(linear_layout[t]);
        }

        // sort neighbors
        std::sort(neighbors.begin(), neighbors.end());

        // calculate cost
        for (int i = 0; i < static_cast<int>(neighbors.size()) - 1; ++i) {
            assert (neighbors[i + 1] > neighbors[i]);
            cost += 1 + std::floor(std::log2(neighbors[i + 1] - neighbors[i]));
            ++gaps;
        }
    }

    return cost / gaps;
}

double utils::calculate_log(graph_access &G, const std::vector<NodeID> &linear_layout) {
    auto calculator = [&linear_layout](NodeID u, NodeID v) {
        auto new_u = static_cast<long>(linear_layout[u]);
        auto new_v = static_cast<long>(linear_layout[v]);
        return 1 + std::floor(std::log2(std::abs(new_u - new_v)));
    };
    return calculate_per_edge_cost(G, calculator) / G.number_of_edges();
}

double utils::calculate_mla_cost(graph_access &G, const std::vector<NodeID> &linear_layout) {
    auto calculator = [&linear_layout](NodeID u, NodeID v) {
        auto new_u = static_cast<long>(linear_layout[u]);
        auto new_v = static_cast<long>(linear_layout[v]);
        return std::abs(new_u - new_v);
    };
    return calculate_per_edge_cost(G, calculator) / G.number_of_edges();
}

template<typename Functor>
double utils::calculate_per_edge_cost(graph_access &G, Functor &calculator) {
    auto cost = 0.0;
    for (NodeID u = 0; u < G.number_of_nodes(); ++u) {
        for (EdgeID e = G.get_first_edge(u); e < G.get_first_invalid_edge(u); ++e) {
            NodeID v = G.getEdgeTarget(e);
            assert(u != v);
            cost += calculator(u, v);
        }
    }
    return cost;
}

double utils::calculate_partition_cost(query_graph &G) {
    auto cost = 0.0;
    auto partition_sizes = G.count_partition_sizes();

    for (NodeID q = 0; q < G.number_of_query_nodes(); ++q) {
        auto degrees = G.count_query_node_degrees(q);
        for (PartitionID p = 0; p < 2; ++p) {
            if (degrees[p] > 0) {
                cost += degrees[p] * utils::log(static_cast<double>(partition_sizes[p]) / (degrees[p] + 1));
            }
        }
    }

    assert (!std::isnan(cost));
    return cost;
}

bool utils::is_boundary_node(graph_access &G, NodeID node) {
    PartitionID p = G.getPartitionIndex(node);

    // a node is a boundary node if it is adjacent on a node that belongs to the other partition
    forall_out_edges(G, edge, node)
            NodeID neighbor = G.getEdgeTarget(edge);
            if (G.getPartitionIndex(neighbor) != p) {
                return true;
            }
    endfor

    return false;
}

std::vector<NodeID> utils::process_graph(const std::string &graph_filename, const std::string &remark,
                                         initial_partitioner_interface &initial_partitioner, refiner_interface &refiner,
                                         reporter &reporter, bool calculate_quadtree_cost, int max_levels) {
    query_graph QG;
    if (graph_io::readGraphWeighted(QG.data_graph(), graph_filename) != 0) {
        std::cerr << "Graph " << graph_filename << " could not be loaded!" << std::endl;
        std::exit(1);
    }
    QG.construct_query_edges();

    // report initial graph metrics
    std::vector<NodeID> identity_layout = create_identity_layout(QG.data_graph());
    double initial_loggap = calculate_loggap(QG.data_graph(), identity_layout);
    double initial_log = calculate_log(QG.data_graph(), identity_layout);
    if (calculate_quadtree_cost) {
        reporter.start(QG, graph_filename, remark, initial_loggap, initial_log, calculate_quadtree_size(QG.data_graph()));
    } else {
        reporter.start(QG, graph_filename, remark, initial_loggap, initial_log, -1);
    }

    // initiate recursive graph reordering
    auto num_recursion_levels = static_cast<int>(log(QG.data_graph().number_of_nodes()));
    if (max_levels > 0) {
        num_recursion_levels = std::min(num_recursion_levels, max_levels);
    }

    std::vector<NodeID> inverted_layout = find_linear_arrangement(QG, num_recursion_levels, initial_partitioner, refiner, reporter);
    std::vector<NodeID> layout = invert_linear_layout(inverted_layout);

    // save partition
    graph_io::writePartition(QG.data_graph(), graph_filename + ".partition_" + std::to_string(std::time(nullptr)));

    // report resulting graph metrics
    double resulting_loggap = calculate_loggap(QG.data_graph(), layout);
    double resulting_log = calculate_log(QG.data_graph(), layout);

    if (calculate_quadtree_cost) {
        graph_access reordered;
        apply_linear_layout(QG.data_graph(), reordered, layout);
        reporter.finish(QG, layout, resulting_loggap, resulting_log, calculate_quadtree_size(reordered));
    } else {
        reporter.finish(QG, layout, resulting_loggap, resulting_log, -1);
    }

    return layout;
}

std::vector<NodeID>
utils::find_linear_arrangement(query_graph &QG, int level, initial_partitioner_interface &partitioner,
                               refiner_interface &refiner, reporter &reporter) {
    // base case: maximum recursion depth reached or no more nodes to work with; order the remaining nodes randomly
    if (level == 0 || QG.data_graph().number_of_nodes() <= 1) {
        std::vector<NodeID> inverted_layout(QG.data_graph().number_of_nodes());
        forall_nodes(QG.data_graph(), v)
                inverted_layout[v] = v;
        endfor
        std::random_shuffle(inverted_layout.begin(), inverted_layout.end());
        return inverted_layout;
    }

    // perform bisection
    reporter.bisection_start(QG);
    partitioner.perform_partitioning(QG, level, reporter);
    quality_metrics qm;
    std::cout << "edge cut on level " << level << ": " << qm.edge_cut(QG.data_graph()) << "; balance: "
              << qm.balance(QG.data_graph()) << std::endl;
    refiner.perform_refinement(QG, 20, level, reporter);
    std::cout << "after refinement: " << qm.edge_cut(QG.data_graph()) << "; balance: " << qm.balance(QG.data_graph())
              << std::endl;
    std::array<query_graph, 2> subgraphs;
    auto map = QG.build_partition_induced_subgraphs(subgraphs);
    reporter.bisection_finish(QG, subgraphs[0], subgraphs[1]);

    // calculate layouts recursively
    reporter.enter_first_branch();
    std::vector<NodeID> lower = find_linear_arrangement(subgraphs[0], level - 1, partitioner, refiner, reporter);
    reporter.leave_first_branch();
    reporter.enter_second_branch();
    std::vector<NodeID> higher = find_linear_arrangement(subgraphs[1], level - 1, partitioner, refiner, reporter);
    reporter.leave_second_branch();

    // concatenate linear layouts
    std::vector<NodeID> inverted_layout(QG.data_graph().number_of_nodes());
    long offset = subgraphs[0].data_graph().number_of_nodes();
    forall_nodes(QG.data_graph(), v)
            if (v < subgraphs[0].data_graph().number_of_nodes()) {
                inverted_layout[v] = map[0][lower[v]];
            } else {
                inverted_layout[v] = map[1][higher[v - offset]];
            }
    endfor
    return inverted_layout;
}

std::vector<PartitionID> utils::get_partition(graph_access &G) {
    std::vector<PartitionID> partition(G.number_of_nodes());
    forall_nodes(G, node)
            partition[node] = G.getPartitionIndex(node);
    endfor
    return partition;
}

void utils::set_partition(graph_access &G, const std::vector<PartitionID> &partition) {
    forall_nodes(G, node)
            G.setPartitionIndex(node, partition[node]);
    endfor
}

void utils::reset_partition(graph_access &G) {
    forall_nodes(G, v)
            G.setPartitionIndex(v, 0);
    endfor
}

std::size_t utils::calculate_quadtree_size(graph_access &G) {
    std::size_t size = 0;

    // round end up to the next power of 2
    // http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
    NodeID end = G.number_of_nodes();
    --end;
    end |= end >> 1;
    end |= end >> 2;
    end |= end >> 4;
    end |= end >> 8;
    end |= end >> 16;
    ++end;

    calculate_quadtree_size(G, &size, 0, end, 0, end);
    return size;
}

bool utils::calculate_quadtree_size(graph_access &G, std::size_t *size,
                                    NodeID x_start, NodeID x_end,
                                    NodeID y_start, NodeID y_end) {
    if (x_end - x_start == 1 || y_end - y_start == 1) { // base case
        assert (x_end - x_start == 1);
        assert (y_end - y_start == 1);

        if (x_start >= G.number_of_nodes() || y_start >= G.number_of_nodes()) {
            return true;
        }

        NodeID v = y_start;
        for (EdgeID e = G.get_first_edge(v); e < G.get_first_invalid_edge(v); ++e) {
            NodeID u = G.getEdgeTarget(e);
            if (x_start == u) {
                return false;
            }
        }

        return true;
    }

    NodeID x_mid = x_start + (x_end - x_start) / 2;
    NodeID y_mid = y_start + (y_end - y_start) / 2;
    bool zero_1 = calculate_quadtree_size(G, size, x_start, x_mid, y_start, y_mid);
    bool zero_2 = calculate_quadtree_size(G, size, x_mid, x_end, y_start, y_mid);
    bool zero_3 = calculate_quadtree_size(G, size, x_mid, x_end, y_mid, y_end);
    bool zero_4 = calculate_quadtree_size(G, size, x_start, x_mid, y_mid, y_end);

    if (zero_1 && zero_2 && zero_3 && zero_4) {
        return true;
    }

    *size += 4;
    return false;
}

void utils::apply_linear_layout(graph_access &original, graph_access &reordered, const std::vector<NodeID> &linear_layout) {
    reordered.start_construction(original.number_of_nodes(), original.number_of_edges());

    std::vector<NodeID> inverted_linear_layout = utils::invert_linear_layout(linear_layout);
    for (NodeID v = 0; v < original.number_of_nodes(); ++v) {
        NodeID node = reordered.new_node();
        reordered.setNodeWeight(node, 1);
        assert (v == node);

        NodeID old_v = inverted_linear_layout[v];
        for (EdgeID old_e = original.get_first_edge(old_v); old_e < original.get_first_invalid_edge(old_v); ++old_e) {
            NodeID old_u = original.getEdgeTarget(old_e);
            NodeID u = linear_layout[old_u];

            EdgeID edge = reordered.new_edge(v, u);
            reordered.setEdgeWeight(edge, 1);
        }
    }

    reordered.finish_construction();
}