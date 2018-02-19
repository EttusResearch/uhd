//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

// Implements templated functions from node_ctrl_base.hpp

#ifndef INCLUDED_LIBUHD_NODE_CTRL_BASE_IPP
#define INCLUDED_LIBUHD_NODE_CTRL_BASE_IPP

#include <uhd/exception.hpp>

#include <boost/shared_ptr.hpp>
#include <vector>

namespace uhd {
    namespace rfnoc {

    template <typename T, bool downstream>
    std::vector< boost::shared_ptr<T> > node_ctrl_base::_find_child_node(bool active_only)
    {
        typedef boost::shared_ptr<T> T_sptr;
        static const size_t MAX_ITER = 20;
        size_t iters = 0;
        // List of return values:
        std::set< T_sptr > results_s;
        // To avoid cycles:
        std::set< sptr > explored;
        // Initialize our search queue with ourself:
        std::set< sptr > search_q;
        search_q.insert(shared_from_this());
        std::set< sptr > next_q;

        while (iters++ < MAX_ITER) {
            next_q.clear();
            BOOST_FOREACH(const sptr &this_node, search_q) {
                // Add this node to the list of explored nodes
                explored.insert(this_node);
                // Create set of all child nodes of this_node that are not in explored:
                std::set< sptr > next_nodes;
                {
                    node_map_t all_next_nodes = downstream ? this_node->list_downstream_nodes() : this_node->list_upstream_nodes();
                    for (
                        node_map_t::iterator it = all_next_nodes.begin();
                        it != all_next_nodes.end();
                        ++it
                    ) {
                        size_t our_port = it->first;
                        if (active_only
                            and not (downstream ? _tx_streamer_active[our_port] : _rx_streamer_active[our_port] )) {
                            continue;
                        }
                        sptr one_next_node = it->second.lock();
                        if (not one_next_node or explored.count(one_next_node)) {
                            continue;
                        }
                        T_sptr next_node_sptr = boost::dynamic_pointer_cast<T>(one_next_node);
                        if (next_node_sptr) {
                            results_s.insert(next_node_sptr);
                        } else {
                            next_nodes.insert(one_next_node);
                        }
                    }
                }
                // Add all of these nodes to the next search queue
                next_q.insert(next_nodes.begin(), next_nodes.end());
            }
            // If next_q is empty, we've exhausted our graph
            if (next_q.empty()) {
                break;
            }
            // Re-init the search queue
            search_q = next_q;
        }

        std::vector< T_sptr > results(results_s.begin(), results_s.end());
        return results;
    }

    template <typename T, typename value_type, bool downstream>
    value_type node_ctrl_base::_find_unique_property(
            boost::function<value_type(boost::shared_ptr<T>, size_t)> get_property,
            value_type NULL_VALUE,
            const std::set< boost::shared_ptr<T> > &exclude_nodes
    ) {
        std::vector< boost::shared_ptr<T> > descendant_rate_nodes = _find_child_node<T, downstream>();
        value_type ret_val = NULL_VALUE;
        std::string first_node_id;
        BOOST_FOREACH(const boost::shared_ptr<T> &node, descendant_rate_nodes) {
            if (exclude_nodes.count(node)) {
                continue;
            }
            // FIXME we need to know the port!!!
            size_t port = ANY_PORT; // NOOO! this is wrong!!!! FIXME
            value_type this_property = get_property(node, port);
            if (this_property == NULL_VALUE) {
                continue;
            }
            // We use the first property we find as reference
            if (ret_val == NULL_VALUE) {
                ret_val = this_property;
                first_node_id = node->unique_id();
                continue;
            }
            // In all subsequent finds, we make sure the property is equal to the reference
            if (this_property != ret_val) {
                throw uhd::runtime_error(
                    str(
                        boost::format("Node %1% specifies %2%, node %3% specifies %4%")
                            % first_node_id % ret_val % node->unique_id() % this_property
                    )
                );
            }
        }
        return ret_val;
    }

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_NODE_CTRL_BASE_IPP */
// vim: sw=4 et:
