#ifndef IMPL_SQLITE_REPORTER_H
#define IMPL_SQLITE_REPORTER_H

#include <sqlite3.h>
#include "reporter.h"

namespace bathesis {
    class sqlite_reporter : public reporter {
        sqlite3 *m_db;

        sqlite3_int64 m_report_id;

        sqlite3_int64 m_bisection_id;

        sqlite3_int64 m_refinement_iteration_id;

        int m_num_moved_0to1;

        int m_num_moved_1to0;

        sqlite3_stmt *prepare(const char *sql);

        void exec(sqlite3_stmt *statement);

        void exec(const char *sql);

        void exec_or_throw(const char *sql, int (*callback)(void *, int, char **, char **), void *arg);

    public:
        sqlite_reporter(const std::string &db_filename);

        ~sqlite_reporter();

        void start(query_graph &QG, const std::string &filename, const std::string &remark,
                   double initial_loggap, double initial_log, long initial_quadtree) override;

        void enter_first_branch() override;

        void leave_first_branch() override;

        void enter_second_branch() override;

        void leave_second_branch() override;

        void finish(query_graph &QG, const std::vector<NodeID> &linear_layout, double resulting_loggap,
                    double resulting_log, long resulting_quadtree) override;

        void bisection_start(query_graph &QG) override;

        void bisection_finish(query_graph &QG, query_graph &first_subgraph, query_graph &second_subgraph) override;

        void initial_partitioning_finish(query_graph &QG) override;

        void refinement_start(query_graph &QG, double initial_partition_cost) override;

        void refinement_finish(query_graph &QG, int iterations_executed, double resulting_partition_cost) override;

        void refinement_iteration_start(query_graph &QG, int nth_iteration, double initial_partition_cost) override;

        void refinement_move_node(query_graph &QG, NodeID node, PartitionID from_partition, double gain_total,
                                  double gain_adjacent, double gain_nonadjacent, bool is_boundary) override;

        void refinement_iteration_finish(query_graph &QG, int num_nodes_exchanged, double resulting_partition_cost) override;
    };
}

#endif // IMPL_SQLITE_REPORTER_H
