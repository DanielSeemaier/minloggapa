#include "reporter.h"

using namespace bathesis;

reporter::reporter() {
    m_recursion_level = 0;
    m_branch_identifier = "";
}

void
reporter::start(query_graph &QG, const std::string &filename, const std::string &remark, double initial_loggap,
                double initial_log, long initial_quadtree) {
    m_filename = filename;
    m_initial_loggap = initial_loggap;
    m_initial_log = initial_log;
    m_initial_quadtree = initial_quadtree;
    m_global_timer.restart();
}

void reporter::enter_first_branch() {
    ++m_recursion_level;
    m_branch_identifier += "0";
    m_branch_timer.restart();
}

void reporter::leave_first_branch() {
    assert (m_recursion_level > 0);
    assert (m_branch_identifier.length() > 0);

    --m_recursion_level;
    m_branch_identifier = m_branch_identifier.substr(0, m_branch_identifier.length() - 1);
}

void reporter::enter_second_branch() {
    ++m_recursion_level;
    m_branch_identifier += "1";
    m_branch_timer.restart();
}

void reporter::leave_second_branch() {
    assert (m_recursion_level > 0);
    assert (m_branch_identifier.length() > 0);

    --m_recursion_level;
    m_branch_identifier = m_branch_identifier.substr(0, m_branch_identifier.length() - 1);
}

void reporter::initial_partitioning_start(query_graph &QG) {
    m_initial_partition_time.restart();
}

void reporter::refinement_start(query_graph &QG, double initial_partition_cost) {
    m_refinement_initial_partition_cost = initial_partition_cost;
    m_refinement_timer.restart();
}

void reporter::refinement_iteration_start(query_graph &QG, int nth_iteration, double initial_partition_cost) {
    m_iteration_initial_partition_cost = initial_partition_cost;
    m_nth_iteration = nth_iteration;
}
