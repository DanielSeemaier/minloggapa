#ifndef IMPL_CLI_REPORTER_H
#define IMPL_CLI_REPORTER_H

#include "reporter.h"

namespace bathesis {
class cli_reporter : public reporter {
   public:
    void start(query_graph &QG, const std::string &filename,
               const std::string &remark, double initial_loggap,
               double initial_log, long initial_quadtree) override;

    void enter_first_branch() override;

    void leave_first_branch() override;

    void enter_second_branch() override;

    void leave_second_branch() override;

    void finish(query_graph &QG, const std::vector<NodeID> &linear_layout,
                double resulting_loggap, double resulting_log,
                long resulting_quadtree) override;

    void bisection_start(query_graph &QG) override;

    void bisection_finish(query_graph &QG, query_graph &first_subgraph,
                          query_graph &second_subgraph) override;

    void initial_partitioning_finish(query_graph &QG) override;

    void refinement_start(query_graph &QG,
                          double initial_partition_cost) override;

    void refinement_finish(query_graph &QG, int iterations_executed,
                           double resulting_partition_cost) override;

    void refinement_iteration_start(query_graph &QG, int nth_iteration,
                                    double initial_partition_cost) override;

    void refinement_move_node(query_graph &QG, NodeID node,
                              PartitionID from_partition, double gain_total,
                              double gain_adjacent, double gain_nonadjacent,
                              bool is_boundary) override;

    void refinement_iteration_finish(query_graph &QG, int num_nodes_exchanged,
                                     double resulting_partition_cost) override;
};
}  // namespace bathesis

#endif  // IMPL_CLI_REPORTER_H

