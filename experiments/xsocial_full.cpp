#include <iostream>
#include <ctime>
#include <sys/stat.h>

#include "experiments.h"
#include "initial-partitioner/configuration.h"
#include "report/sqlite_reporter.h"
#include "initial-partitioner/kahip_initial_partitioner.h"
#include "refinement/fm_refiner.h"
#include "utils.h"

#define EXPERIMENT_NAME "xsocial_full"

using namespace bathesis;

void process(const std::string &filename) {
    configuration cfg;
    auto strongsocial = [&cfg](PartitionConfig &partition_configuration) {
        cfg.eco(partition_configuration);
        cfg.strongsocial(partition_configuration);
    };
    auto ecosocial = [&cfg](PartitionConfig &partition_configuration) {
        cfg.eco(partition_configuration);
        cfg.ecosocial(partition_configuration);
    };
    auto fastsocial = [&cfg](PartitionConfig &partition_configuration) {
        cfg.eco(partition_configuration);
        cfg.fastsocial(partition_configuration);
    };
    std::string output_directory = EXPERIMENTS_OUT_DIRECTORY "/" EXPERIMENT_NAME "/";
    { // strongsocial
        std::cout << "\tstrongsocial" << std::endl;

        sqlite_reporter reporter(output_directory + "strongsocial.db");
        kahip_initial_partitioner partitioner(3, 1, static_cast<uint>(std::time(nullptr)), strongsocial);
        fm_refiner refiner(3, 1);
        utils::process_graph(DATA_DIRECTORY "/fb/" + filename, <#initializer#>, partitioner, refiner, reporter, false, 1);
    }
    { // ecosocial
        std::cout << "\tecosocial" << std::endl;

        sqlite_reporter reporter(output_directory + "ecosocial.db");
        kahip_initial_partitioner partitioner(3, 1, static_cast<uint>(std::time(nullptr)), ecosocial);
        fm_refiner refiner(3, 1);
        utils::process_graph(DATA_DIRECTORY "/fb/" + filename, <#initializer#>, partitioner, refiner, reporter, false, 1);
    }
    { // fastsocial
        std::cout << "\tfastsocial" << std::endl;

        sqlite_reporter reporter(output_directory + "fastsocial.db");
        kahip_initial_partitioner partitioner(3, 1, static_cast<uint>(std::time(nullptr)), fastsocial);
        fm_refiner refiner(3, 1);
        utils::process_graph(DATA_DIRECTORY "/fb/" + filename, <#initializer#>, partitioner, refiner, reporter, false, 1);
    }
}

int main() {
    std::string output_directory = EXPERIMENTS_OUT_DIRECTORY "/" EXPERIMENT_NAME "/";

    struct stat sb;
    if (stat(output_directory.c_str(), &sb) != 0 || !S_ISDIR(sb.st_mode)) {
        std::cerr << "Ensure that the directory " << output_directory << " exists!" << std::endl;
        std::exit(1);
    }
    //remove((output_directory + "strongsocial.db").c_str());
    //remove((output_directory + "strongsocial.db-journal").c_str());
    //remove((output_directory + "ecosocial.db").c_str());
    //remove((output_directory + "ecosocial.db-journal").c_str());
    //remove((output_directory + "fastsocial.db").c_str());
    //remove((output_directory + "fastsocial.db-journal").c_str());

    process("Caltech36.graph");
    process("Reed98.graph");
    process("Simmons81.graph");
    process("Haverford76.graph");
    process("Swarthmore42.graph");
    process("USFCA72.graph");
    process("Bowdoin47.graph");
    process("Mich67.graph");

    return EXIT_SUCCESS;
}