#include <fstream>
#include <sstream>
#include <tools/quality_metrics.h>

#include "sqlite_reporter.h"

using namespace bathesis;

const char *DB_SCHEME = "CREATE TABLE IF NOT EXISTS `report` ("
        "`id` INTEGER PRIMARY KEY AUTOINCREMENT,"
        "`filename` TEXT,"
        "`remark` TEXT,"
        "`nodes` INTEGER,"
        "`edges` INTEGER,"
        "`initial_loggap` REAL,"
        "`initial_log` REAL,"
        "`initial_quadtree` INTEGER,"
        "`time` REAL,"
        "`resulting_loggap` REAL,"
        "`resulting_log` REAL,"
        "`resulting_quadtree` INTEGER,"
        "`creation_date` DATE DEFAULT (datetime('now', 'localtime'))"
        ");"
        "CREATE TABLE IF NOT EXISTS `bisection` ("
        "`id` INTEGER PRIMARY KEY AUTOINCREMENT,"
        "`rid` INTEGER,"
        "`branch` TEXT,"
        "`nodes` INTEGER,"
        "`edges` INTEGER,"
        "`p0_nodes` INTEGER,"
        "`p0_edges` INTEGER,"
        "`p1_nodes` INTEGER,"
        "`p1_edges` INTEGER,"
        "`initial_cut` INTEGER,"
        "`cut` INTEGER,"
        "`imbalance` INTEGER,"
        "`initial_partition_cost` REAL,"
        "`resulting_partition_cost` REAL,"
        "`partitioning_time` REAL,"
        "`refinement_time` REAL,"
        "FOREIGN KEY(rid) REFERENCES report(id)"
        ");"
        "CREATE TABLE IF NOT EXISTS `iteration` ("
        "`id` INTEGER PRIMARY KEY AUTOINCREMENT,"
        "`bid` INTEGER,"
        "`nth` INTEGER,"
        "`initial_partition_cost` REAL,"
        "`resulting_partition_cost` REAL,"
        "`num_moved_0to1` INTEGER,"
        "`num_moved_1to0` INTEGER,"
        "FOREIGN KEY (bid) REFERENCES bisection(id)"
        ");"
        "CREATE TABLE IF NOT EXISTS `movement` ("
        "`id` INTEGER PRIMARY KEY AUTOINCREMENT,"
        "`iid` INTEGER,"
        "`nodes0` INTEGER,"
        "`nodes1` INTEGER,"
        "`from` INTEGER,"
        "`to` INTEGER,"
        "`gain_total` REAL,"
        "`gain_adjacent` REAL,"
        "`gain_nonadjacent` REAL,"
        "`boundary` INTEGER,"
        "`deg_data0` INTEGER,"
        "`deg_data1` INTEGER,"
        "`deg_query` INTEGER,"
        "`deg_query0` INTEGER,"
        "`deg_query1` INTEGER,"
        "FOREIGN KEY (iid) REFERENCES iteration(id)"
        ");";

sqlite_reporter::sqlite_reporter(const std::string &db_filename) {
    if (sqlite3_open(db_filename.c_str(), &m_db) != SQLITE_OK) {
        throw sqlite3_errmsg(m_db);
    }

    exec(DB_SCHEME);
    exec("BEGIN TRANSACTION;");
}

sqlite_reporter::~sqlite_reporter() {
    exec("COMMIT TRANSACTION;");
    sqlite3_close(m_db);
}

void sqlite_reporter::start(query_graph &QG, const std::string &filename, const std::string &remark,
                            double initial_loggap, double initial_log, long initial_quadtree) {
    reporter::start(QG, filename, remark, initial_loggap, initial_log, 0);

    sqlite3_stmt *stmt = prepare("INSERT INTO `report` (`filename`, `remark`, `nodes`, `edges`, `initial_loggap`, `initial_log`, `initial_quadtree`) "
                                         "VALUES (?, ?, ?, ?, ?, ?, ?);");
    sqlite3_bind_text(stmt, 1, filename.c_str(), -1, nullptr);
    sqlite3_bind_text(stmt, 2, remark.c_str(), -1, nullptr);
    sqlite3_bind_int(stmt, 3, QG.data_graph().number_of_nodes());
    sqlite3_bind_int(stmt, 4, QG.data_graph().number_of_edges());
    sqlite3_bind_double(stmt, 5, initial_loggap);
    sqlite3_bind_double(stmt, 6, initial_log);
    sqlite3_bind_int(stmt, 7, static_cast<int>(initial_quadtree));
    exec(stmt);

    m_report_id = sqlite3_last_insert_rowid(m_db);
}

void sqlite_reporter::finish(query_graph &QG, const std::vector<NodeID> &linear_layout, double resulting_loggap,
                             double resulting_log, long resulting_quadtree) {
    sqlite3_stmt *stmt = prepare("UPDATE `report` "
                                         "SET `time` = ?, `resulting_loggap` = ?, `resulting_log` = ?, `resulting_quadtree` = ? "
                                         "WHERE `id` = ?;");
    sqlite3_bind_double(stmt, 1, m_global_timer.elapsed());
    sqlite3_bind_double(stmt, 2, resulting_loggap);
    sqlite3_bind_double(stmt, 3, resulting_log);
    sqlite3_bind_int(stmt, 4, static_cast<int>(resulting_quadtree));
    sqlite3_bind_int64(stmt, 5, m_report_id);
    exec(stmt);
}

void sqlite_reporter::enter_first_branch() {
    reporter::enter_first_branch();
}

void sqlite_reporter::leave_first_branch() {
    reporter::leave_first_branch();
}

void sqlite_reporter::enter_second_branch() {
    reporter::enter_second_branch();
}

void sqlite_reporter::leave_second_branch() {
    reporter::leave_second_branch();
}

void sqlite_reporter::bisection_start(query_graph &QG) {
    sqlite3_stmt *stmt = prepare("INSERT INTO `bisection` (`rid`, `branch`, `nodes`, `edges`) VALUES (?, ?, ?, ?);");
    sqlite3_bind_int64(stmt, 1, m_report_id);
    sqlite3_bind_text(stmt, 2, m_branch_identifier.c_str(), -1, nullptr);
    sqlite3_bind_int(stmt, 3, QG.data_graph().number_of_nodes());
    sqlite3_bind_int(stmt, 4, QG.data_graph().number_of_edges());
    exec(stmt);

    m_bisection_id = sqlite3_last_insert_rowid(m_db);
}

void sqlite_reporter::bisection_finish(query_graph &QG, query_graph &first_subgraph, query_graph &second_subgraph) {
    double imbalance = std::abs(static_cast<long>(first_subgraph.data_graph().number_of_nodes())
                                - static_cast<long>(second_subgraph.data_graph().number_of_nodes()));
    imbalance /= first_subgraph.data_graph().number_of_nodes() + second_subgraph.data_graph().number_of_nodes();
    imbalance *= 100;

    sqlite3_stmt *stmt = prepare("UPDATE `bisection` "
                                         "SET `p0_nodes` = ?, `p0_edges` = ?, `p1_nodes` = ?, `p1_edges` = ?, `cut` = ?, `imbalance` = ? "
                                         "WHERE `id` = ?;");
    sqlite3_bind_int(stmt, 1, first_subgraph.data_graph().number_of_nodes());
    sqlite3_bind_int(stmt, 2, first_subgraph.data_graph().number_of_edges());
    sqlite3_bind_int(stmt, 3, second_subgraph.data_graph().number_of_nodes());
    sqlite3_bind_int(stmt, 4, second_subgraph.data_graph().number_of_edges());
    sqlite3_bind_int(stmt, 5, quality_metrics().edge_cut(QG.data_graph()));
    sqlite3_bind_int(stmt, 6, static_cast<int>(imbalance));
    sqlite3_bind_int64(stmt, 7, m_bisection_id);
    exec(stmt);
}

void sqlite_reporter::initial_partitioning_finish(query_graph &QG) {
    sqlite3_stmt *stmt = prepare("UPDATE `bisection` SET `partitioning_time` = ?, `initial_cut` = ? WHERE `id` = ?;");
    sqlite3_bind_double(stmt, 1, m_initial_partition_time.elapsed());
    sqlite3_bind_int(stmt, 2, quality_metrics().edge_cut(QG.data_graph()));
    sqlite3_bind_int64(stmt, 3, m_bisection_id);
    exec(stmt);
}

void sqlite_reporter::refinement_start(query_graph &QG, double initial_partition_cost) {
    reporter::refinement_start(QG, initial_partition_cost);
    sqlite3_stmt *stmt = prepare("UPDATE `bisection` SET `initial_partition_cost` = ? WHERE `id` = ?;");
    sqlite3_bind_double(stmt, 1, initial_partition_cost);
    sqlite3_bind_int64(stmt, 2, m_bisection_id);
    exec(stmt);
}

void sqlite_reporter::refinement_finish(query_graph &QG, int iterations_executed, double resulting_partition_cost) {
    sqlite3_stmt *stmt = prepare("UPDATE `bisection` "
                                         "SET `resulting_partition_cost` = ?, `refinement_time` = ? "
                                         "WHERE `id` = ?;");
    sqlite3_bind_double(stmt, 1, resulting_partition_cost);
    sqlite3_bind_double(stmt, 2, m_refinement_timer.elapsed());
    sqlite3_bind_int64(stmt, 3, m_bisection_id);
    exec(stmt);
}

void sqlite_reporter::refinement_iteration_start(query_graph &QG, int nth_iteration, double initial_partition_cost) {
    reporter::refinement_iteration_start(QG, nth_iteration, initial_partition_cost);

    sqlite3_stmt *stmt = prepare("INSERT INTO `iteration` (`bid`, `nth`, `initial_partition_cost`) VALUES (?, ?, ?);");
    sqlite3_bind_int64(stmt, 1, m_bisection_id);
    sqlite3_bind_int(stmt, 2, nth_iteration);
    sqlite3_bind_double(stmt, 3, initial_partition_cost);
    exec(stmt);

    m_refinement_iteration_id = sqlite3_last_insert_rowid(m_db);
    m_num_moved_0to1 = 0;
    m_num_moved_1to0 = 0;
}

void
sqlite_reporter::refinement_move_node(query_graph &QG, NodeID node, PartitionID from_partition, double gain_total, double gain_adjacent,
                                      double gain_nonadjacent, bool is_boundary) {
    std::array<NodeID, 2> degrees = {0, 0};
    forall_out_edges(QG.data_graph(), edge, node)
            NodeID neighbor = QG.data_graph().getEdgeTarget(edge);
            ++degrees[QG.data_graph().getPartitionIndex(neighbor)];
    endfor

    std::array<NodeID, 2> query_degrees = {0, 0};
    auto adjacent_query_nodes = QG.get_adjacent_query_nodes(node);
    for (auto &q : adjacent_query_nodes) {
        auto q_degrees = QG.count_query_node_degrees(q);
        query_degrees[0] += q_degrees[0];
        query_degrees[1] += q_degrees[1];
    }
    query_degrees[from_partition] += adjacent_query_nodes.size();
    query_degrees[1 - from_partition] -= adjacent_query_nodes.size();

    auto partition_sizes = QG.count_partition_sizes();
    ++partition_sizes[from_partition];
    --partition_sizes[1 - from_partition];

    sqlite3_stmt *stmt = prepare("INSERT INTO `movement` ("
                                         "`iid`, `nodes0`, `nodes1`, `from`, `to`, `gain_total`, `gain_adjacent`, "
                                         "`gain_nonadjacent`, `boundary`, `deg_data0`, `deg_data1`, `deg_query`, "
                                         "`deg_query0`, `deg_query1`) "
                                         "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");
    sqlite3_bind_int64(stmt, 1, m_refinement_iteration_id);
    sqlite3_bind_int(stmt, 2, partition_sizes[0]);
    sqlite3_bind_int(stmt, 3, partition_sizes[1]);
    sqlite3_bind_int(stmt, 4, from_partition);
    sqlite3_bind_int(stmt, 5, 1 - from_partition);
    sqlite3_bind_double(stmt, 6, gain_total);
    sqlite3_bind_double(stmt, 7, gain_adjacent);
    sqlite3_bind_double(stmt, 8, gain_nonadjacent);
    sqlite3_bind_int(stmt, 9, is_boundary ? 1 : 0);
    sqlite3_bind_int(stmt, 10, degrees[0]);
    sqlite3_bind_int(stmt, 11, degrees[1]);
    sqlite3_bind_int(stmt, 12, static_cast<int>(adjacent_query_nodes.size()));
    sqlite3_bind_int(stmt, 13, query_degrees[0]);
    sqlite3_bind_int(stmt, 14, query_degrees[1]);
    exec(stmt);

    if (from_partition == 1) {
        ++m_num_moved_1to0;
    } else {
        ++m_num_moved_0to1;
    }
}

void
sqlite_reporter::refinement_iteration_finish(query_graph &QG, int num_nodes_exchanged, double resulting_partition_cost) {
    sqlite3_stmt *stmt = prepare("UPDATE `iteration` "
                                         "SET `resulting_partition_cost` = ?, `num_moved_0to1` = ?, `num_moved_1to0` = ? "
                                         "WHERE `id` = ?;");
    sqlite3_bind_double(stmt, 1, resulting_partition_cost);
    sqlite3_bind_int(stmt, 2, m_num_moved_0to1);
    sqlite3_bind_int(stmt, 3, m_num_moved_1to0);
    sqlite3_bind_int64(stmt, 4, m_refinement_iteration_id);
    exec(stmt);

    // TODO remove
    std::cout << "sqlite_reporter::refinement_iteration_finish(num_nodes_exchanged = "
              << num_nodes_exchanged << ", resulting_partition_cost = " << resulting_partition_cost << ")"
              << std::endl;
}

void sqlite_reporter::exec(const char *sql) {
    exec_or_throw(sql, nullptr, nullptr);
}

void sqlite_reporter::exec_or_throw(const char *sql, int (*callback)(void *, int, char **, char **), void *arg) {
    char *error = nullptr;
    if (sqlite3_exec(m_db, sql, callback, arg, &error)) {
        std::runtime_error exp(error);
        sqlite3_free(error);
        throw exp;
    }
}

sqlite3_stmt *sqlite_reporter::prepare(const char *sql) {
    sqlite3_stmt *statement;
    if (SQLITE_OK != sqlite3_prepare_v2(m_db, sql, -1, &statement, nullptr)) {
        throw std::runtime_error(sqlite3_errmsg(m_db));
    }
    return statement;
}

void sqlite_reporter::exec(sqlite3_stmt *statement) {
    sqlite3_step(statement);
    sqlite3_finalize(statement);
}
