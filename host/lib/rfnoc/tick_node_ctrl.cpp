//
// Copyright 2014-2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/rfnoc/tick_node_ctrl.hpp>

using namespace uhd::rfnoc;

const double tick_node_ctrl::RATE_UNDEFINED = 0;

double tick_node_ctrl::get_tick_rate(
    const std::set< node_ctrl_base::sptr > &_explored_nodes
) {
    // First, see if we've implemented _get_tick_rate()
    {
        double my_tick_rate = _get_tick_rate();
        if (my_tick_rate != RATE_UNDEFINED) {
            return my_tick_rate;
        }
    }

    // If not, we ask all our neighbours for the tick rate.
    // This will fail if we get different values.
    std::set< node_ctrl_base::sptr > explored_nodes(_explored_nodes);
    explored_nodes.insert(shared_from_this());
    // Here, we need all up- and downstream nodes
    std::vector< sptr > neighbouring_tick_nodes = find_downstream_node<tick_node_ctrl>(true);
    {
        std::vector< sptr > upstream_neighbouring_tick_nodes = find_upstream_node<tick_node_ctrl>(true);
        neighbouring_tick_nodes.insert(
                neighbouring_tick_nodes.end(),
                upstream_neighbouring_tick_nodes.begin(),
                upstream_neighbouring_tick_nodes.end()
        );
    } // neighbouring_tick_nodes is now initialized
    double ret_val = RATE_UNDEFINED;
    for(const sptr &node:  neighbouring_tick_nodes) {
        if (_explored_nodes.count(node)) {
            continue;
        }
        double tick_rate = node->get_tick_rate(explored_nodes);
        if (tick_rate == RATE_UNDEFINED) {
            continue;
        }
        if (ret_val == RATE_UNDEFINED) {
            ret_val = tick_rate;
            // TODO: Remember name of this node so we can make the throw message more descriptive.
            continue;
        }
        if (tick_rate != ret_val) {
            throw uhd::runtime_error(
                str(
                    // TODO add node names
                    boost::format("Conflicting tick rates: One neighbouring block specifies %d MHz, another %d MHz.")
                    % tick_rate % ret_val
                )
            );
        }
    }
    return ret_val;
}

