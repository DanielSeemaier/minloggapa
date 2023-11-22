//
// Created by daniel on 4/23/17.
//

#ifndef IMPL_BASIC_REFINER_H
#define IMPL_BASIC_REFINER_H

#include <fstream>
#include "refiner_interface.h"
#include "../data-structure/query_graph.h"

namespace bathesis {

    class basic_refiner : public refiner_interface {
        std::array<NodeID, 2> m_partition_sizes;

        std::vector<double> calculate_gain_values();

        double calculate_node_cost(NodeID node);

        double calculate_node_cost(const std::array<NodeID, 2> &degrees);

        double calculate_node_cost(const std::array<NodeID, 2> &partition_sizes, const std::array<NodeID, 2> &degrees);

    protected:
        NodeID perform_refinement_iteration(int nth_iteration, int imbalance);

    public:
        basic_refiner(int imbalance = 3, int imbalance_level = 1);
    };
}


#endif //IMPL_BASIC_REFINER_H
