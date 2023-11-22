#include <iostream>
#include <ctime>
#include <sys/stat.h>
#include <dirent.h>

#include "experiments.h"
#include "initial-partitioner/configuration.h"
#include "report/sqlite_reporter.h"
#include "initial-partitioner/kahip_initial_partitioner.h"
#include "refinement/fm_refiner.h"
#include "utils.h"

#define EXPERIMENT_NAME "xsocial"

using namespace bathesis;

int main() {
    configuration cfg;
    auto strongsocial = [&cfg](auto &partition_configuration) { cfg.strongsocial(partition_configuration); };
    auto ecosocial = [&cfg](auto &partition_configuration) { cfg.ecosocial(partition_configuration); };
    auto fastsocial = [&cfg](auto &partition_configuration) { cfg.fastsocial(partition_configuration); };

    std::string output_directory = EXPERIMENTS_OUT_DIRECTORY "/" EXPERIMENT_NAME "/";

    struct stat sb;
    if (stat(output_directory.c_str(), &sb) != 0 || !S_ISDIR(sb.st_mode)) {
        std::cerr << "Ensure that the directory " << output_directory << " exists!" << std::endl;
        std::exit(1);
    }
    remove((output_directory + "strongsocial.db").c_str());
    remove((output_directory + "strongsocial.db-journal").c_str());
    remove((output_directory + "ecosocial.db").c_str());
    remove((output_directory + "ecosocial.db-journal").c_str());
    remove((output_directory + "fastsocial.db").c_str());
    remove((output_directory + "fastsocial.db-journal").c_str());

    int max_level = 1;

    DIR *dir;
    dirent *ent;
    if ((dir = opendir(DATA_DIRECTORY "/fb/")) != nullptr) {
        while ((ent = readdir(dir)) != nullptr) {
            std::string filename(ent->d_name);
            if (filename == "." || filename == "..") {
                continue;
            }

            std::cout << filename << std::endl;

            { // strongsocial
                std::cout << "\tstrongsocial" << std::endl;

                sqlite_reporter reporter(output_directory + "strongsocial.db");
                kahip_initial_partitioner partitioner(3, 2, static_cast<uint>(std::time(0)), strongsocial);
                fm_refiner refiner(3);
                utils::process_graph(DATA_DIRECTORY "/fb/" + filename, <#initializer#>, partitioner, refiner, reporter, false, max_level);
            }
            { // ecosocial
                std::cout << "\tecosocial" << std::endl;

                sqlite_reporter reporter(output_directory + "ecosocial.db");
                kahip_initial_partitioner partitioner(3, 2, static_cast<uint>(std::time(0)), ecosocial);
                fm_refiner refiner(3);
                utils::process_graph(DATA_DIRECTORY "/fb/" + filename, <#initializer#>, partitioner, refiner, reporter, false, max_level);
            }
            { // fastsocial
                std::cout << "\tfastsocial" << std::endl;

                sqlite_reporter reporter(output_directory + "fastsocial.db");
                kahip_initial_partitioner partitioner(3, 2, static_cast<uint>(std::time(0)), fastsocial);
                fm_refiner refiner(3);
                utils::process_graph(DATA_DIRECTORY "/fb/" + filename, <#initializer#>, partitioner, refiner, reporter, false, max_level);
            }
        }
    }

    return EXIT_SUCCESS;
}