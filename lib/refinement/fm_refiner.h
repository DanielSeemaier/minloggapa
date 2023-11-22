#ifndef IMPL_FM_REFINER_H
#define IMPL_FM_REFINER_H

#include "refiner_interface.h"
#include "../data-structure/max_node_heap.h"

namespace bathesis {
    struct query_node_info {
        NodeID node;
        std::array<NodeID, 2> degrees;
        double cost;
        std::array<double, 2> adjacent_node_contribution;
    };

    struct data_node_info {
        NodeID node;
        double gain = 0.0;
        double gain2 = 0.0;
        bool marked = false;
    };

    class fm_refiner : public refiner_interface {
        int m_max_refinement_iterations;

        std::array<NodeID, 2> m_partition_sizes{0, 0};

        std::array<EdgeID, 2> m_partition_edges{0, 0};

        std::pair<std::vector<query_node_info>, std::vector<data_node_info>> calculate_gain_values();

        std::array<double, 2> m_nonadjacent_base_cost{0.0, 0.0};

        void update_gain_values(std::vector<query_node_info> &query_node_info,
                                std::vector<data_node_info> &data_node_info, NodeID node);

        double calculate_node_cost(const std::array<NodeID, 2> &degrees);

        double calculate_node_cost(const std::array<NodeID, 2> &partition_sizes, const std::array<NodeID, 2> &degrees);

    protected:
        NodeID perform_refinement_iteration(int nth_iteration, int imbalance);

    public:
        fm_refiner(int imbalance = 3, int imbalance_level = 1);
    };
}


#endif // IMPL_FM_REFINER_H
