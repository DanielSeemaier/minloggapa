#include <string>
#include <sys/stat.h>
#include <iostream>
#include <initial-partitioner/configuration.h>
#include <report/sqlite_reporter.h>
#include <initial-partitioner/kahip_initial_partitioner.h>
#include <io/graph_io.h>
#include <utils.h>
#include <algorithm>
#include <ctime>
#include <refinement/fm_refiner.h>
#include <tools/quality_metrics.h>
#include <initial-partitioner/random_initial_partitioner.h>
#include <random>
#include <refinement/fm_refiner_quadtree.h>
#include "experiments.h"

#define EXPERIMENT_NAME "ls_improvement_test"
#define TRIES 10

using namespace bathesis;

void process(const std::string &filename, const std::string &output_dir);
std::vector<NodeID> find_linear_arrangement(query_graph &QG, long level, reporter &reporter);

int main(int argc, char **argv) {
    std::string output_directory = EXPERIMENTS_OUT_DIRECTORY "/" EXPERIMENT_NAME "/";

    struct stat sb;
    if (stat(output_directory.c_str(), &sb) != 0 || !S_ISDIR(sb.st_mode)) {
        std::cerr << "Error: Ensure that the output directory '" << output_directory << "' exists" << std::endl;
        std::exit(1);
    }

    std::srand(static_cast<uint>(std::time(nullptr)));
    //process(DATA_DIRECTORY "email-enron.graph", output_directory);
    //process(DATA_DIRECTORY "fb/Caltech36.graph", output_directory);
    //process(DATA_DIRECTORY "fb/Reed98.graph", output_directory);
    //process(DATA_DIRECTORY "fb/Simmons81.graph", output_directory);
    //process(DATA_DIRECTORY "fb/Haverford76.graph", output_directory);
    process(DATA_DIRECTORY "fb/Swarthmore42.graph", output_directory);
    //process(DATA_DIRECTORY "fb/USFCA72.graph", output_directory);
    //process(DATA_DIRECTORY "fb/Bowdoin47.graph", output_directory);
    //process(DATA_DIRECTORY "fb/Mich67.graph", output_directory);


    return EXIT_SUCCESS;
}

void process(const std::string &filename, const std::string &output_dir) {
    sqlite_reporter reporter(output_dir + "out.random.good.db");

    // load graph
    query_graph QG;
    if (graph_io::readGraphWeighted(QG.data_graph(), filename) != 0) {
        std::cerr << "Graph " << filename << " could not be read" << std::endl;
        std::exit(1);
    }
    QG.construct_query_edges();

    // report initial graph metrics
    std::vector<NodeID> identity_layout = utils::create_identity_layout(QG.data_graph());
    auto initial_loggap = utils::calculate_loggap(QG.data_graph(), identity_layout);
    auto initial_log = utils::calculate_log(QG.data_graph(), identity_layout);
    reporter.start(QG, filename, "", initial_loggap, initial_log, utils::calculate_quadtree_size(QG.data_graph()));

    // perform reordering
    auto max_level = static_cast<long>(utils::log(QG.data_graph().number_of_nodes()));
    std::vector<NodeID> inverted_layout = find_linear_arrangement(QG, max_level, reporter);
    std::vector<NodeID> layout = utils::invert_linear_layout(inverted_layout);

    // report resulting graph metrics
    auto resulting_loggap = utils::calculate_loggap(QG.data_graph(), layout);
    auto resulting_log = utils::calculate_log(QG.data_graph(), layout);
    graph_access reordered;
    utils::apply_linear_layout(QG.data_graph(), reordered, layout);
    reporter.finish(QG, layout, resulting_loggap, resulting_log, utils::calculate_quadtree_size(reordered));
}

std::vector<NodeID> find_linear_arrangement(query_graph &QG, long level, reporter &reporter) {
    // base case
    if (level == 0 || QG.data_graph().number_of_nodes() <= 1 || QG.data_graph().number_of_edges() == 0) {
        std::vector<NodeID> inverted_layout = utils::create_identity_layout(QG.data_graph());
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(inverted_layout.begin(), inverted_layout.end(), g);
        return inverted_layout;
    }

    // partition graph
    reporter.bisection_start(QG);

    std::vector<PartitionID> best_partition;
    auto best_partition_cost = std::numeric_limits<double>().max();
    std::ofstream logfile(EXPERIMENTS_OUT_DIRECTORY "/" EXPERIMENT_NAME "/log.bad.txt", std::ofstream::out | std::ofstream::app);

    for (std::size_t i = 0; i < TRIES; ++i) {
        configuration cfg;
        auto ecosocial = [&cfg](PartitionConfig &partition_config) { cfg.eco(partition_config); };
        //kahip_initial_partitioner kahip(3, 1, static_cast<uint>(std::time(nullptr)), ecosocial);
        random_initial_partitioner kahip(static_cast<uint>(std::time(nullptr)));
        fm_refiner_quadtree fm(3);

        kahip.perform_partitioning(QG, level, reporter);
        std::cout << "Initial edge cut: " << quality_metrics().edge_cut(QG.data_graph()) * 2 << std::endl;
        fm.perform_refinement(QG, 20, level, reporter);
        std::cout << "Resulting edge cut: " << quality_metrics().edge_cut(QG.data_graph()) * 2 << std::endl;
        auto cost = utils::calculate_partition_cost(QG);

        std::cout << "[" << level << ", " << i << "] " << best_partition_cost << " -> " << cost << std::endl;

        if (cost < best_partition_cost) {
            logfile << "[" << level << ", " << i << "] " << best_partition_cost << " -> " << cost << std::endl;
            best_partition = utils::get_partition(QG.data_graph());
            best_partition_cost = cost;
        }

        utils::reset_partition(QG.data_graph());
    }

    logfile.close();
    utils::set_partition(QG.data_graph(), best_partition);

    std::array<query_graph, 2> subgraphs;
    auto map = QG.build_partition_induced_subgraphs(subgraphs);
    reporter.bisection_finish(QG, subgraphs[0], subgraphs[1]);

    // calculate layouts for subgraphs recursively
    reporter.enter_first_branch();
    std::vector<NodeID> first = find_linear_arrangement(subgraphs[0], level - 1, reporter);
    reporter.leave_first_branch();
    reporter.enter_second_branch();
    std::vector<NodeID> second = find_linear_arrangement(subgraphs[1], level - 1, reporter);
    reporter.leave_second_branch();

    // concatenate linear layouts
    std::vector<NodeID> inverted_layout(QG.data_graph().number_of_nodes());
    long offset = subgraphs[0].data_graph().number_of_nodes();

    for (NodeID v = 0; v < QG.data_graph().number_of_nodes(); ++v) {
        if (v < subgraphs[0].data_graph().number_of_nodes()) {
            inverted_layout[v] = map[0][first[v]];
        } else {
            inverted_layout[v] = map[1][second[v - offset]];
        }
    }
    return inverted_layout;
}
