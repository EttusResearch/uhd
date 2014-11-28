//
// Copyright 2014 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

// Implements templated functions from node_ctrl_base.hpp

#ifndef INCLUDED_LIBUHD_NODE_CTRL_BASE_IPP
#define INCLUDED_LIBUHD_NODE_CTRL_BASE_IPP

#include <uhd/exception.hpp>
#include <boost/shared_ptr.hpp>
#include <vector>

namespace uhd {
    namespace rfnoc {

    template <typename T>
    std::vector< boost::shared_ptr<T> > node_ctrl_base::_find_child_node(bool downstream)
    {
        typedef boost::shared_ptr<T> T_sptr;
        static const size_t MAX_ITER = 20;
        size_t iters = 0;
        // List of return values:
        std::vector< T_sptr > results;
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
                        sptr one_next_node = it->second.lock();
                        if (one_next_node and not explored.count(one_next_node)) {
                            next_nodes.insert(one_next_node);
                        }
                    }
                }
                // Iterate over all of these nodes, see if they match what we're looking for:
                BOOST_FOREACH(const sptr &next_node, next_nodes) {
                    T_sptr next_node_sptr = boost::dynamic_pointer_cast<T>(next_node);
                    if (next_node_sptr) {
                        results.push_back(next_node_sptr);
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

        return results;
    }

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_NODE_CTRL_BASE_IPP */
// vim: sw=4 et:
