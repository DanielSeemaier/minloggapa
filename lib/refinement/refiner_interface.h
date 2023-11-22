#ifndef IMPL_REFINEMENT_H
#define IMPL_REFINEMENT_H

#include <data_structure/graph_access.h>

#include "../data-structure/query_graph.h"
#include "report/reporter.h"

namespace bathesis {
    class refiner_interface {
    protected:
        query_graph *m_query_graph;
        graph_access *m_data_graph;
        reporter *m_reporter;

        int m_imbalance_level;
        int m_imbalance;

        virtual NodeID perform_refinement_iteration(int nth_iteration, int imbalance) = 0;

    public:
        refiner_interface(int imbalance = 3, int imbalance_level = 1);

        void perform_refinement(query_graph &query_graph, int max_iterations, int level, reporter &reporter);
    };
}

#endif // IMPL_REFINEMENT_H
