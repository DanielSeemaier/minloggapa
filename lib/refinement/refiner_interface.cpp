#include "refiner_interface.h"
#include "../utils.h"

using namespace bathesis;

refiner_interface::refiner_interface(int imbalance, int imbalance_level)
    : m_imbalance(imbalance), m_imbalance_level(imbalance_level) {
}

void refiner_interface::perform_refinement(query_graph &query_graph, int max_iterations, int level, reporter &reporter) {
    m_query_graph = &query_graph;
    m_data_graph = &query_graph.data_graph();
    m_reporter = &reporter;

    double initial_cost = utils::calculate_partition_cost(*m_query_graph);
    double pre_iteration_cost = initial_cost;
    m_reporter->refinement_start(query_graph, initial_cost);

    int i = 0;
    for (i = 0; i < max_iterations; ++i) {
        int imbalance = 3;
        if (level % m_imbalance_level == 0) {
            imbalance = m_imbalance;
        }

        // perform a single refinement iterations
        reporter.refinement_iteration_start(query_graph, i, pre_iteration_cost);
        NodeID nodes_moved = perform_refinement_iteration(i, imbalance);
        double post_iteration_cost = utils::calculate_partition_cost(*m_query_graph);
        reporter.refinement_iteration_finish(query_graph, nodes_moved, post_iteration_cost);
        pre_iteration_cost = post_iteration_cost;

        // if no nodes were moved during an iteration, abort
        if (nodes_moved == 0) {
            break;
        }
    }

    // pre_iteration_cost is the resulting partition cost
    m_reporter->refinement_finish(query_graph, i, pre_iteration_cost);
}
