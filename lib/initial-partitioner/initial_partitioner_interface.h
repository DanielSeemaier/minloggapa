#ifndef IMPL_INITIAL_PARTITIONER_H
#define IMPL_INITIAL_PARTITIONER_H

#include <data_structure/graph_access.h>
#include "../data-structure/query_graph.h"
#include "report/reporter.h"

namespace bathesis {

    /**
     * Performs initial partitioning on the underlying data graph of {@code QG}.
     */
    class initial_partitioner_interface {
    public:
        virtual void perform_partitioning(query_graph &QG, long recursion_level, reporter &reporter) = 0;
    };
}

#endif // IMPL_INITIAL_PARTITIONER_H
