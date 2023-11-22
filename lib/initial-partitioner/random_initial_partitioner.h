#ifndef IMPL_RANDOM_PARTITIONER_H
#define IMPL_RANDOM_PARTITIONER_H

#include "initial_partitioner_interface.h"
#include "report/reporter.h"

namespace bathesis {

    class random_initial_partitioner : public initial_partitioner_interface {
        uint m_seed;

    public:
        random_initial_partitioner(uint seed);

        void perform_partitioning(query_graph &QG, long recursion_level, reporter &reporter) override;
    };
}

#endif // IMPL_RANDOM_PARTITIONER_H
