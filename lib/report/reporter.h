#ifndef IMPL_NEW_REPORTER_H
#define IMPL_NEW_REPORTER_H

#include <tools/timer.h>

#include "../data-structure/query_graph.h"

namespace bathesis {
class reporter {
   protected:
    std::string m_filename;
    int m_recursion_level;
    std::string m_branch_identifier;
    int m_nth_iteration;
    double m_initial_loggap;
    double m_initial_log;
    long m_initial_quadtree;
    double m_refinement_initial_partition_cost;
    double m_iteration_initial_partition_cost;

    timer m_global_timer;
    timer m_branch_timer;
    timer m_initial_partition_time;
    timer m_refinement_timer;

   public:
    reporter();

    virtual ~reporter() = default;

    virtual void start(query_graph &QG, const std::string &filename,
                       const std::string &remark, double initial_loggap,
                       double initial_log, long initial_quadtree);

    virtual void finish(query_graph &QG,
                        const std::vector<NodeID> &linear_layout,
                        double resulting_loggap, double resulting_log,
                        long resulting_quadtree) = 0;

    virtual void enter_first_branch();

    virtual void leave_first_branch();

    virtual void enter_second_branch();

    virtual void leave_second_branch();

    virtual void bisection_start(query_graph &QG) = 0;

    virtual void bisection_finish(query_graph &QG, query_graph &first_subgraph,
                                  query_graph &second_subgraph) = 0;

    virtual void initial_partitioning_start(query_graph &QG);

    virtual void initial_partitioning_finish(query_graph &QG) = 0;

    virtual void refinement_start(query_graph &QG,
                                  double initial_partition_cost);

    virtual void refinement_finish(query_graph &QG, int iterations_executed,
                                   double resulting_partition_cost) = 0;

    virtual void refinement_iteration_start(query_graph &QG, int nth_iteration,
                                            double initial_partition_cost);

    virtual void refinement_move_node(query_graph &QG, NodeID node,
                                      PartitionID from_partition,
                                      double gain_total, double gain_adjacent,
                                      double gain_nonadjacent,
                                      bool is_boundary) = 0;

    virtual void refinement_iteration_finish(
        query_graph &QG, int num_nodes_exchanged,
        double resulting_partition_cost) = 0;
};
}  // namespace bathesis

#endif  // IMPL_NEW_REPORTER_H
