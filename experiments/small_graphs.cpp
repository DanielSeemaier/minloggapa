#include <iostream>
#include <ctime>
#include <sys/stat.h>
#include <dirent.h>
#include <initial-partitioner/random_initial_partitioner.h>
#include <io/graph_io.h>
#include <refinement/fm_refiner_quadtree.h>

#include "experiments.h"
#include "initial-partitioner/configuration.h"
#include "report/sqlite_reporter.h"
#include "initial-partitioner/kahip_initial_partitioner.h"
#include "refinement/fm_refiner.h"
#include "utils.h"
#include "refinement/basic_refiner.h"

#define EXPERIMENT_NAME "small_graphs"

using namespace bathesis;

int main() {
    std::string output_directory = EXPERIMENTS_OUT_DIRECTORY EXPERIMENT_NAME "/";

    struct stat sb;
    if (stat(output_directory.c_str(), &sb) != 0 || !S_ISDIR(sb.st_mode)) {
        std::cerr << "Ensure that the directory " << output_directory << " exists" << std::endl;
        std::exit(1);
    }

    // Configuration
    configuration cfg;
    auto kahip_configuration = [&cfg](PartitionConfig &partition_configuration) {
        cfg.eco(partition_configuration);
        //cfg.ecosocial(partition_configuration);
    };
    uint seed = static_cast<uint>(std::time(nullptr));
    kahip_initial_partitioner kahip(3, 1, seed, kahip_configuration);
    random_initial_partitioner random(seed);
    sqlite_reporter reporter(output_directory + "out.db");
    fm_refiner fm;
    basic_refiner basic;

    // List of graphs to process
    std::vector<std::string> graphs{
            //"/generated/cycle_10.graph",
            //"/generated/cycle_100.graph",
            "/generated/cycle_5000.graph",

            // list of all FB graphs < 8 MB
            /*"/fb/UCSC68.graph",
            "/fb/USC35.graph",
            "/fb/BU10.graph",
            "/fb/Maine59.graph",
            "/fb/Columbia2.graph",
            "/fb/Northwestern25.graph",
            "/fb/UVA16.graph",
            "/fb/Haverford76.graph",
            "/fb/Princeton12.graph",
            "/fb/BC17.graph",
            "/fb/Georgetown15.graph",
            "/fb/Williams40.graph",
            "/fb/Mississippi66.graph",
            "/fb/Trinity100.graph",
            "/fb/UCSB37.graph",
            "/fb/Duke14.graph",
            "/fb/Santa74.graph",
            "/fb/USFCA72.graph",
            "/fb/Middlebury45.graph",
            "/fb/Tulane29.graph",
            "/fb/Smith60.graph",
            "/fb/Villanova62.graph",
            "/fb/Rutgers89.graph",
            "/fb/Temple83.graph",
            "/fb/William77.graph",
            "/fb/UC33.graph",
            "/fb/test.graph",
            "/fb/Brandeis99.graph",
            "/fb/Rice31.graph",
            "/fb/Pepperdine86.graph",
            "/fb/Wisconsin87.graph",
            "/fb/Rochester38.graph",
            "/fb/Howard90.graph",
            "/fb/Simmons81.graph",
            "/fb/UChicago30.graph",
            "/fb/American75.graph",
            "/fb/Dartmouth6.graph",
            "/fb/Reed98.graph",
            "/fb/WashU32.graph",
            "/fb/UConn91.graph",
            "/fb/Hamilton46.graph",
            "/fb/Bingham82.graph",
            "/fb/Syracuse56.graph",
            "/fb/UCSD34.graph",
            "/fb/Oberlin44.graph",
            "/fb/NotreDame57.graph",
            "/fb/Lehigh96.graph",
            "/fb/JMU79.graph",
            "/fb/Bucknell39.graph",
            "/fb/Mich67.graph",
            "/fb/Amherst41.graph",
            "/fb/Bowdoin47.graph",
            "/fb/Northeastern19.graph",
            "/fb/MU78.graph",
            "/fb/Cal65.graph",
            "/fb/Wellesley22.graph",
            "/fb/Caltech36.graph",
            "/fb/MIT8.graph",
            "/fb/Wesleyan43.graph",
            "/fb/Emory27.graph",
            "/fb/UC64.graph",
            "/fb/UC61.graph",
            "/fb/Baylor93.graph",
            "/fb/Carnegie49.graph",
            "/fb/Tufts18.graph",
            "/fb/Colgate88.graph",
            "/fb/Stanford3.graph",
            "/fb/Swarthmore42.graph",
            "/fb/Brown11.graph",
            "/fb/Tennessee95.graph",
            "/fb/UCF52.graph",
            "/fb/Wake73.graph",
            "/fb/USF51.graph",
            "/fb/Vassar85.graph",
            "/fb/Vermont70.graph",
            "/fb/UNC28.graph",
            "/fb/UMass92.graph",
            "/fb/GWU54.graph",
            "/fb/Yale4.graph",
            "/fb/Vanderbilt48.graph",*/

            //"/email-enron.graph",
            //"/kronecker-simple-16.graph"
    };

    // Process graphs with KaHIP vs Random and FM vs Basic
    for (auto &graph : graphs) {
        utils::process_graph(DATA_DIRECTORY + graph, "kahip,fm", kahip, fm, reporter, false, 1);
        //utils::process_graph(DATA_DIRECTORY + graph, "kahip,basic", kahip, basic, reporter);
        //utils::process_graph(DATA_DIRECTORY + graph, "random,fm", random, fm, reporter);
        //utils::process_graph(DATA_DIRECTORY + graph, "random,basic", random, basic, reporter);
    }

    return EXIT_SUCCESS;
}