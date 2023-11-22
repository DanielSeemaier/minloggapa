#ifndef IMPL_KAHIP_INITIAL_PARTITIONER_H
#define IMPL_KAHIP_INITIAL_PARTITIONER_H

#include <functional>
#include <partition_config.h>
#include "initial_partitioner_interface.h"
#include "report/reporter.h"

namespace bathesis {
    using partition_configurator = std::function<void(PartitionConfig &)>;

    class kahip_initial_partitioner : public initial_partitioner_interface {
        ImbalanceType m_imbalance;

        int m_imbalance_level;

        uint m_seed;

        partition_configurator m_configurator;

    public:
        kahip_initial_partitioner(ImbalanceType imbalance, int imbalance_level, uint seed, partition_configurator configurator);

        void perform_partitioning(query_graph &QG, long recursion_level, reporter &reporter) override;
    };
}

#endif // IMPL_KAHIP_INITIAL_PARTITIONER_H
