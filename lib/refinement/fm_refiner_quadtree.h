#ifndef IMPL_FM_REFINER_QUADTREE_H
#define IMPL_FM_REFINER_QUADTREE_H

#include "refiner_interface.h"
#include "../data-structure/max_node_heap.h"

namespace bathesis {
    struct node_info {
        NodeID node = 0;
        bool marked = false;
        double gain = 0.0;
        std::array<NodeID, 2> num_edges_to{0, 0};
    };

    class fm_refiner_quadtree : public refiner_interface {
        std::array<NodeID, 2> m_partition_sizes{0, 0};
        std::array<std::array<NodeID, 2>, 2> m_num_edges_from_to{std::array<NodeID, 2>{0, 0},
                                                             std::array<NodeID, 2>{0, 0}};

        void move_and_update(NodeID node, std::vector<node_info> &nodes);

        void update_gain_values(std::vector<node_info> &nodes);

        double evaluate_cost_function();

        double approx_log_faculty(double n);

        double approx_log_binom(double n, double k);

        double approx_quarter_cost(std::size_t from, std::size_t to);

        std::vector<node_info> init_partition_info();

    protected:
        NodeID perform_refinement_iteration(int nth_iteration, int imbalance);

    public:
        fm_refiner_quadtree(int imbalance = 3, int imbalance_level = 1);
    };
}


#endif // IMPL_FM_REFINER_QUADTREE_H
