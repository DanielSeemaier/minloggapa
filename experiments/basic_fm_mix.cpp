#include <io/graph_io.h>
#include <algorithm>
#include <ctime>
#include <sys/stat.h>
#include <random>
#include "utils.h"
#include "experiments.h"
#include "initial-partitioner/configuration.h"
#include "report/sqlite_reporter.h"
#include "initial-partitioner/kahip_initial_partitioner.h"
#include "refinement/fm_refiner.h"
#include "refinement/basic_refiner.h"

#define EXPERIMENT_NAME "basic_fm_mix"

using namespace bathesis;

std::vector<NodeID>
find_linear_arrangement(query_graph &QG, long level, initial_partitioner_interface &partitioner, reporter &reporter) {
    // base case: maximum recursion depth reached or no more nodes to work with; order the remaining nodes randomly
    if (QG.data_graph().number_of_nodes() <= 1 || QG.data_graph().number_of_edges() == 0) {
        std::vector<NodeID> inverted_layout(QG.data_graph().number_of_nodes());
        forall_nodes(QG.data_graph(), v)
                inverted_layout[v] = v;
        endfor

        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(inverted_layout.begin(), inverted_layout.end(), g);
        return inverted_layout;
    }

    // perform bisection
    fm_refiner fm(3);
    basic_refiner basic(3);

    reporter.bisection_start(QG);
    partitioner.perform_partitioning(QG, level, reporter);

    auto initial_partition = utils::get_partition(QG.data_graph());
    fm.perform_refinement(QG, 20, level, reporter);
    auto fm_partition = utils::get_partition(QG.data_graph());
    auto fm_partition_cost = utils::calculate_partition_cost(QG);

    utils::set_partition(QG.data_graph(), initial_partition);
    basic.perform_refinement(QG, 20, level, reporter);
    auto basic_partition_cost = utils::calculate_partition_cost(QG);

    std::ofstream logfile(EXPERIMENTS_OUT_DIRECTORY "/" EXPERIMENT_NAME "/log.txt", std::ofstream::out | std::ofstream::app);
    std::cout << "fm_partition_cost: " << fm_partition_cost << std::endl;
    std::cout << "basic_partition_cost: " << basic_partition_cost << std::endl;
    if (fm_partition_cost <= basic_partition_cost) {
        logfile << "FM wins with " << fm_partition_cost << " vs " << basic_partition_cost << std::endl;
        utils::set_partition(QG.data_graph(), fm_partition);
    } else {
        logfile << "BASIC wins with " << fm_partition_cost << " vs " << basic_partition_cost << std::endl;
    }
    logfile.close();

    std::array<query_graph, 2> subgraphs;
    auto map = QG.build_partition_induced_subgraphs(subgraphs);


    reporter.bisection_finish(QG, subgraphs[0], subgraphs[1]);

    // calculate layouts recursively
    reporter.enter_first_branch();
    std::vector<NodeID> lower = find_linear_arrangement(subgraphs[0], level + 1, partitioner, reporter);
    reporter.leave_first_branch();
    reporter.enter_second_branch();
    std::vector<NodeID> higher = find_linear_arrangement(subgraphs[1], level + 1, partitioner, reporter);
    reporter.leave_second_branch();

    // concatenate linear layouts
    std::vector<NodeID> inverted_layout(QG.data_graph().number_of_nodes());
    forall_nodes(QG.data_graph(), v)
            if (v < subgraphs[0].data_graph().number_of_nodes()) {
                inverted_layout[v] = map[0][lower[v]];
            } else {
                long offset = subgraphs[0].data_graph().number_of_nodes();
                inverted_layout[v] = map[1][higher[v - offset]];
            }
    endfor
    return inverted_layout;
}

std::vector<NodeID>
process_graph(const std::string &graph_filename, initial_partitioner_interface &initial_partitioner, reporter &reporter) {
    query_graph QG;
    if (graph_io::readGraphWeighted(QG.data_graph(), graph_filename)) {
        std::cerr << "Graph " << graph_filename << " could not be loaded!" << std::endl;
        std::exit(1);
    }
    QG.construct_query_edges();

    // report initial graph metrics
    std::vector<NodeID> identity_layout = utils::create_identity_layout(QG.data_graph());
    double initial_loggap = utils::calculate_loggap(QG.data_graph(), identity_layout);
    double initial_log = utils::calculate_log(QG.data_graph(), identity_layout);
    reporter.start(QG, graph_filename, "", initial_loggap, initial_log, 0);


    std::vector<NodeID> inverted_layout = find_linear_arrangement(QG, 0, initial_partitioner, reporter);
    std::vector<NodeID> layout = utils::invert_linear_layout(inverted_layout);

    // report resulting graph metrics
    double resulting_loggap = utils::calculate_loggap(QG.data_graph(), layout);
    double resulting_log = utils::calculate_log(QG.data_graph(), layout);
    reporter.finish(QG, layout, resulting_loggap, resulting_log, 0);

    return layout;
}

void
process(std::string filename) {
    std::string output_directory = EXPERIMENTS_OUT_DIRECTORY "/" EXPERIMENT_NAME "/";
    configuration cfg;
    auto ecosocial = [&cfg](auto &partition_configuration) { cfg.ecosocial(partition_configuration); };

    sqlite_reporter reporter(output_directory + "out.db");
    kahip_initial_partitioner partitioner(3, 2, static_cast<uint>(std::time(0)), ecosocial);
    process_graph(DATA_DIRECTORY "/" + filename, partitioner, reporter);
}

void process_small_graphs() {
    process("/email-enron.graph");
    process("/fb/Caltech36.graph");
    process("/fb/Reed98.graph");
    process("/fb/Simmons81.graph");
    process("/fb/Haverford76.graph");
    process("/fb/Swarthmore42.graph");
    process("/fb/USFCA72.graph");
    process("/fb/Bowdoin47.graph");
    process("/fb/Mich67.graph");
}

int main() {
    std::string output_directory = EXPERIMENTS_OUT_DIRECTORY "/" EXPERIMENT_NAME "/";

    struct stat sb;
    if (stat(output_directory.c_str(), &sb) != 0 || !S_ISDIR(sb.st_mode)) {
        std::cerr << "Ensure that the directory " << output_directory << " exists!" << std::endl;
        std::exit(1);
    }

    for (std::size_t i = 0; i < 3; ++i) {
        process_small_graphs();
    }

    return EXIT_SUCCESS;
}
