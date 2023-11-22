#include "random_initial_partitioner.h"

#include <algorithm>
#include <ctime>
#include <random>

using namespace bathesis;

random_initial_partitioner::random_initial_partitioner(uint seed)
    : m_seed(seed) {}

void random_initial_partitioner::perform_partitioning(query_graph &QG,
                                                      long recursion_level,
                                                      reporter &reporter) {
    reporter.initial_partitioning_start(QG);

    auto &G = QG.data_graph();

    std::srand(m_seed);

    G.set_partition_count(2);
    std::vector<NodeID> random(G.number_of_nodes());
    for (NodeID i = 0; i < G.number_of_nodes() / 2; ++i) {
        random[i] = 1;
    }

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(random.begin(), random.end(), g);

    for (NodeID n = 0; n < G.number_of_nodes(); ++n) {
        G.setPartitionIndex(n, random[n]);
    }

    reporter.initial_partitioning_finish(QG);
}
