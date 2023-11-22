#ifndef IMPL_UTILS_H
#define IMPL_UTILS_H

#include <data_structure/graph_access.h>
#include "data-structure/query_graph.h"
#include "refinement/refiner_interface.h"
#include "initial-partitioner/initial_partitioner_interface.h"

namespace bathesis {

    class utils {
        template<typename Functor>
        static double calculate_per_edge_cost(graph_access &G, Functor &calculator);

        static bool calculate_quadtree_size(graph_access &G, std::size_t *size,
                                            NodeID x_start, NodeID x_end,
                                            NodeID y_start, NodeID y_end);

    public:

        static double log(double arg);

        static std::vector<NodeID> create_identity_layout(graph_access &G);

        static std::vector<NodeID> create_random_layout(graph_access &G);

        static std::vector<NodeID> invert_linear_layout(std::vector<NodeID> inverted_linear_layout);

        static double calculate_loggap(graph_access &G, const std::vector<NodeID> &linear_layout);

        static double calculate_log(graph_access &G, const std::vector<NodeID> &linear_layout);

        static double calculate_mla_cost(graph_access &G, const std::vector<NodeID> &linear_layout);

        static double calculate_partition_cost(query_graph &G);

        static bool is_boundary_node(graph_access &G, NodeID node);

        static std::vector<PartitionID> get_partition(graph_access &G);

        static void set_partition(graph_access &G, const std::vector<PartitionID> &partition);

        static void reset_partition(graph_access &G);

        static std::vector<NodeID>
        process_graph(const std::string &graph_filename, const std::string &remark,
                      initial_partitioner_interface &initial_partitioner, refiner_interface &refiner,
                      reporter &reporter, bool calculate_quadtree_cost = false, int max_levels = 0);

        static std::vector<NodeID>
        find_linear_arrangement(query_graph &QG, int level, initial_partitioner_interface &partitioner,
                                refiner_interface &refiner, reporter &reporter);

        static std::size_t calculate_quadtree_size(graph_access &G);

        static void apply_linear_layout(graph_access &original, graph_access &reordered, const std::vector<NodeID> &linear_layout);
    };
}

#endif //IMPL_UTILS_H
