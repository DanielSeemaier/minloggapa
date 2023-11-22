#include "cli_reporter.h"

#include <tools/quality_metrics.h>

#include <fstream>
#include <sstream>

using namespace bathesis;

void cli_reporter::start(query_graph &QG, const std::string &filename,
                         const std::string &remark, double initial_loggap,
                         double initial_log, long initial_quadtree) {
    reporter::start(QG, filename, remark, initial_loggap, initial_log, 0);

    std::cout << "initial_loggap = " << initial_loggap << std::endl;
    std::cout << "initial_log = " << initial_log << std::endl;
    std::cout << "initial_quadtree = " << initial_quadtree << std::endl;
}

void cli_reporter::finish(query_graph &QG,
                          const std::vector<NodeID> &linear_layout,
                          double resulting_loggap, double resulting_log,
                          long resulting_quadtree) {
    std::cout << "resulting_loggap = " << resulting_loggap << std::endl;
    std::cout << "resulting_log = " << resulting_log << std::endl;
    std::cout << "resulting_quadtree = " << resulting_quadtree << std::endl;
}

void cli_reporter::enter_first_branch() { reporter::enter_first_branch(); }

void cli_reporter::leave_first_branch() { reporter::leave_first_branch(); }

void cli_reporter::enter_second_branch() { reporter::enter_second_branch(); }

void cli_reporter::leave_second_branch() { reporter::leave_second_branch(); }

void cli_reporter::bisection_start(query_graph &QG) {}

void cli_reporter::bisection_finish(query_graph &QG,
                                    query_graph &first_subgraph,
                                    query_graph &second_subgraph) {}

void cli_reporter::initial_partitioning_finish(query_graph &QG) {}

void cli_reporter::refinement_start(query_graph &QG,
                                    double initial_partition_cost) {}

void cli_reporter::refinement_finish(query_graph &QG, int iterations_executed,
                                     double resulting_partition_cost) {}

void cli_reporter::refinement_iteration_start(query_graph &QG,
                                              int nth_iteration,
                                              double initial_partition_cost) {}

void cli_reporter::refinement_move_node(query_graph &QG, NodeID node,
                                        PartitionID from_partition,
                                        double gain_total, double gain_adjacent,
                                        double gain_nonadjacent,
                                        bool is_boundary) {}

void cli_reporter::refinement_iteration_finish(
    query_graph &QG, int num_nodes_exchanged, double resulting_partition_cost) {
}
