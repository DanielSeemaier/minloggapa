#include <dirent.h>
#include <initial-partitioner/random_initial_partitioner.h>
#include <io/graph_io.h>
#include <refinement/fm_refiner_quadtree.h>
#include <sys/stat.h>

#include <ctime>
#include <iostream>

#include "experiments.h"
#include "initial-partitioner/configuration.h"
#include "initial-partitioner/kahip_initial_partitioner.h"
#include "refinement/basic_refiner.h"
#include "refinement/fm_refiner.h"
#include "report/cli_reporter.h"
#include "utils.h"

using namespace bathesis;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr
            << "usage: ./minloggapa <graph> [<kahip|random> <fm|basic>]\n";
        std::exit(1);
    }

    const std::string graph = argv[1];
    const std::string partitioner = (argc >= 3 ? argv[2] : "kahip");
    const std::string refiner = (argc >= 4 ? argv[3] : "basic");

    std::cerr << "graph: " << graph << " partitioner=" << partitioner
              << " refiner=" << refiner << "\n";

    configuration cfg;
    auto kahip_configuration =
        [&cfg](PartitionConfig &partition_configuration) {
            cfg.fastsocial(partition_configuration);
        };
    uint seed = static_cast<uint>(std::time(nullptr));
    kahip_initial_partitioner kahip(3, 1, seed, kahip_configuration);
    random_initial_partitioner random(seed);
    fm_refiner fm;
    basic_refiner basic;

    cli_reporter rep;

    const bool compute_quadtree_cost = false;
    const int max_levels = 7;

    if (partitioner == "kahip") {
        if (refiner == "fm") {
            utils::process_graph(graph, "kahip,fm", kahip, fm, rep,
                                 compute_quadtree_cost, max_levels);
        } else if (refiner == "basic") {
            utils::process_graph(graph, "kahip,basic", kahip, basic, rep,
                                 compute_quadtree_cost, max_levels);
        }
    } else if (partitioner == "random") {
        if (refiner == "fm") {
            utils::process_graph(graph, "random,fm", random, fm, rep,
                                 compute_quadtree_cost, max_levels);
        } else if (refiner == "basic") {
            utils::process_graph(graph, "random,basic", random, basic, rep,
                                 compute_quadtree_cost, max_levels);
        }
    }

    return EXIT_SUCCESS;
}
