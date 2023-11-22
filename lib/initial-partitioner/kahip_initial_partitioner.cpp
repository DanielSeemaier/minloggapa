#include <partition/initial_partitioning/initial_partitioning.h>
#include <tools/random_functions.h>
#include <partition/graph_partitioner.h>
#include <partition/uncoarsening/refinement/cycle_improvements/cycle_refinement.h>
#include <ctime>

#include "kahip_initial_partitioner.h"
#include "balance_configuration.h"
#include "configuration.h"

using namespace bathesis;

kahip_initial_partitioner::kahip_initial_partitioner(ImbalanceType imbalance, int imbalance_level, uint seed, partition_configurator configurator)
        : m_imbalance(imbalance),
          m_imbalance_level(imbalance_level),
          m_seed(seed),
          m_configurator(configurator) {
}

void
kahip_initial_partitioner::perform_partitioning(query_graph &QG, long recursion_level, reporter &reporter) {
    assert (0 <= m_imbalance && m_imbalance <= 100);
    assert (m_imbalance_level > 0);

    reporter.initial_partitioning_start(QG);

    auto &G = QG.data_graph();

    // configure KaHIP for bisection
    PartitionConfig partition_config;
    partition_config.k = 2;

    configuration cfg;
    cfg.standard(partition_config);
    m_configurator(partition_config);

    if (recursion_level % m_imbalance_level == 0) {
        partition_config.imbalance = m_imbalance;
    }

    // VVV copied from vendor/KaHIP/app/kaffpa.cpp
    G.set_partition_count(partition_config.k);

    balance_configuration bc;
    bc.configurate_balance(partition_config, G);

    //partition_config.seed = 0;
    partition_config.seed = m_seed; // not from kaffpa.cpp
    srand(partition_config.seed);
    random_functions::setSeed(partition_config.seed);

    std::cout << "graph has " << G.number_of_nodes() << " nodes and " << G.number_of_edges() << " edges" << std::endl;
    // ***************************** perform partitioning ***************************************
    timer t;
    t.restart();
    graph_partitioner partitioner;
    quality_metrics qm;

    std::cout << "performing partitioning!" << std::endl;
    if (partition_config.time_limit == 0) {
        partitioner.perform_partitioning(partition_config, G);
    } else {
        PartitionID *map = new PartitionID[G.number_of_nodes()];
        EdgeWeight best_cut = std::numeric_limits<EdgeWeight>::max();
        while (t.elapsed() < partition_config.time_limit) {
            partition_config.graph_allready_partitioned = false;
            partitioner.perform_partitioning(partition_config, G);
            EdgeWeight cut = qm.edge_cut(G);
            if (cut < best_cut) {
                best_cut = cut;
                forall_nodes(G, node){
                            map[node] = G.getPartitionIndex(node);
                        }endfor
            }
        }

        forall_nodes(G, node){
                    G.setPartitionIndex(node, map[node]);
                }endfor
    }

    if (partition_config.kaffpa_perfectly_balance) {
        double epsilon = partition_config.imbalance / 100.0;
        partition_config.upper_bound_partition =
                (1 + epsilon) * ceil(partition_config.largest_graph_weight / (double) partition_config.k);

        complete_boundary boundary(&G);
        boundary.build();

        cycle_refinement cr;
        cr.perform_refinement(partition_config, G, boundary);
    }

    reporter.initial_partitioning_finish(QG);
}
