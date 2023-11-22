/**
 * CLI tool that calculates LogGap and Log costs of a given graph.
 */

#include <iostream>
#include <getopt.h>
#include <data_structure/graph_access.h>
#include <io/graph_io.h>
#include <iomanip>
#include <tools/quality_metrics.h>

#include "utils.h"

using namespace bathesis;

void print_usage(std::ostream &stream, const char *name);

int main(int argc, char *argv[]) {
    option arguments[] = {
            {
                    "help",
                    no_argument,
                    nullptr,
                    'h'
            },
            {
                    "permute_randomly",
                    no_argument,
                    nullptr,
                    'r'
            },
            {
                    "seed",
                    required_argument,
                    nullptr,
                    's'
            },
            {
                    "partition",
                    required_argument,
                    nullptr,
                    'p'
            },
            {
                    nullptr,
                    0,
                    nullptr,
                    0
            }
    };

    // options that can be overridden
    auto permute_randomly = false;
    auto seed = static_cast<uint>(time(nullptr));
    auto use_partition = false;
    std::string partition_filename;

    int value = 0;
    do {
        value = getopt_long(argc, argv, "hrs:p:", arguments, nullptr);

        switch (value) {
            case 'h': {
                print_usage(std::cout, argv[0]);
                std::exit(0);
            }

            case 'r': {
                permute_randomly = true;
                break;
            }

            case 's': {
                seed = static_cast<uint>(std::stoul(optarg));
                break;
            }

            case 'p': {
                use_partition = true;
                partition_filename = std::string(optarg);
                break;
            }

            case '?': {
                print_usage(std::cerr, argv[0]);
                std::exit(1);
            }

            default: {
                // do nothing
            }
        }
    } while (value != -1);

    if (argc - optind != 1) {
        print_usage(std::cerr, argv[0]);
        std::exit(1);
    }

    std::srand(seed);

    std::string filename(argv[optind]);

    query_graph G;
    if (graph_io::readGraphWeighted(G.data_graph(), filename) != 0) {
        std::cerr << "Graph file '" << filename << "' could not be loaded" << std::endl;
        std::exit(1);
    }
    G.construct_query_edges();

    if (use_partition && graph_io::readPartition(G.data_graph(), partition_filename) != 0) {
        std::cerr << "Partition file " << partition_filename << " could not be loaded" << std::endl;
        std::exit(1);
    }

    std::vector<NodeID> identity = permute_randomly ? utils::create_random_layout(G.data_graph())
                                                    : utils::create_identity_layout(G.data_graph());

    std::cout << "=== Graph Metrics ===" << std::endl;
    std::cout << "|Q| = " << G.number_of_query_nodes() << std::endl;
    std::cout << "|V| = " << G.data_graph().number_of_nodes() << std::endl;
    std::cout << "|E| = " << G.data_graph().number_of_edges() << std::endl;
    std::cout << "LogGap: " << utils::calculate_loggap(G.data_graph(), identity) << " bits/gap" << std::endl;
    std::cout << "Log: " << utils::calculate_log(G.data_graph(), identity) << " bits/edge" << std::endl;
    std::cout << "Quadtree: " << utils::calculate_quadtree_size(G.data_graph()) << " bits" << std::endl;

    if (use_partition) {
        std::cout << "=== Partition Metrics ===" << std::endl;

        quality_metrics qm;
        std::cout << "Balance: " << qm.balance(G.data_graph()) << std::endl;
        std::cout << "Cut edges: " << qm.edge_cut(G.data_graph()) << std::endl;
        std::cout << "Partition cost: " << utils::calculate_partition_cost(G) << std::endl;
    }

    return 0;
}

void print_usage(std::ostream &stream, const char *name) {
    stream << "Usage: " << name << " OPTIONS input" << std::endl;
    stream << "\t-h, --help: Show this message" << std::endl;
    stream << "\t-r, --permute_randomly: Use a random permutation for log and loggap calculation" << std::endl;
    stream << "\t-s number, --seed=number: Use number as seed to generate the random permutation" << std::endl;
    stream << "\t-p file, --partition=file: Specify partition file to display additional info" << std::endl;
}