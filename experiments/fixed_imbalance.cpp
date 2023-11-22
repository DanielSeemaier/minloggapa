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

#define EXPERIMENT_NAME "fixed_imbalance"

using namespace bathesis;

std::vector<NodeID>
find_linear_arrangement(query_graph &QG, long level, initial_partitioner_interface &partitioner, reporter &reporter) {
    // base case: maximum recursion depth reached or no more nodes to work with; order the remaining nodes randomly
    if (QG.data_graph().number_of_nodes() <= 1) {
        std::vector<NodeID> inverted_layout(QG.data_graph().number_of_nodes());
        forall_nodes(QG.data_graph(), v)
                inverted_layout[v] = v;
        endfor

        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(inverted_layout.begin(), inverted_layout.end(), g);
        return inverted_layout;
    }

    std::vector<PartitionID> best_partition;
    int best_imbalance = -1;
    std::vector<int> imbalances {3, 18, 33, 48}; // <-- imbalances to try
    double best_partition_cost = std::numeric_limits<double>().max();

    reporter.bisection_start(QG);

    for (int imbalance : imbalances) {
        configuration cfg;
        auto strongsocial = [&cfg](PartitionConfig &partition_configuration) {
            cfg.eco(partition_configuration);
            cfg.strongsocial(partition_configuration);
        };

        kahip_initial_partitioner kahip(imbalance, 2, static_cast<uint>(std::time(nullptr)), strongsocial);
        fm_refiner refiner(imbalance, 2);
        kahip.perform_partitioning(QG, level, reporter);
        refiner.perform_refinement(QG, 20, level, reporter);

        double cost = utils::calculate_partition_cost(QG);
        if (cost < best_partition_cost) {
            best_partition = utils::get_partition(QG.data_graph());
            best_partition_cost = cost;
            best_imbalance = imbalance;
        }

        utils::reset_partition(QG.data_graph());
    }

    utils::set_partition(QG.data_graph(), best_partition);


    std::ofstream logfile(EXPERIMENTS_OUT_DIRECTORY "/" EXPERIMENT_NAME "/log.txt",
                          std::ofstream::out | std::ofstream::app);
    logfile << "best_imbalance: " << best_imbalance << std::endl;
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
    if (graph_io::readGraphWeighted(QG.data_graph(), graph_filename) != 0) {
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
process(const std::string &filename) {
    std::string output_directory = EXPERIMENTS_OUT_DIRECTORY "/" EXPERIMENT_NAME "/";

    configuration cfg;
    auto strongsocial = [&cfg](PartitionConfig &partition_configuration) {
        cfg.eco(partition_configuration);
        cfg.strongsocial(partition_configuration);
    };

    sqlite_reporter reporter(output_directory + "out.db");
    kahip_initial_partitioner partitioner(3, 2, static_cast<uint>(std::time(0)), strongsocial);
    process_graph(DATA_DIRECTORY "/" + filename, partitioner, reporter);
}

void process_small_graphs() {
    //process("/email-enron.graph");
    process("/fb/Caltech36.graph");
    //process("/fb/Caltech36.graph");
    //process("/fb/Reed98.graph");
    //process("/fb/Simmons81.graph");
    //process("/fb/Haverford76.graph");
    //process("/fb/Swarthmore42.graph");
    //process("/fb/USFCA72.graph");
    //process("/fb/Bowdoin47.graph");
    //process("/fb/Mich67.graph");
}

int main() {
    std::string output_directory = EXPERIMENTS_OUT_DIRECTORY "/" EXPERIMENT_NAME "/";

    struct stat sb;
    if (stat(output_directory.c_str(), &sb) != 0 || !S_ISDIR(sb.st_mode)) {
        std::cerr << "Ensure that the directory " << output_directory << " exists!" << std::endl;
        std::exit(1);
    }

    process_small_graphs();

    return EXIT_SUCCESS;
}
