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

#include <uhd/usrp/rfnoc/scalar_node_ctrl.hpp>

using namespace uhd::rfnoc;

// FIXME add recursion limiters (i.e. list of explored nodes)
double scalar_node_ctrl::get_input_scale_factor(
        size_t /* port */
) {
    return _get_downstream_unique_scale_factor();
}

double scalar_node_ctrl::get_output_scale_factor(
        size_t /* port */
) {
    return _get_upstream_unique_scale_factor();
}

// TODO get rid of this code redundancy
double scalar_node_ctrl::_get_upstream_unique_scale_factor()
{
    std::vector< sptr > descendant_rate_nodes = find_upstream_node<scalar_node_ctrl>();
    double ret_val = SCALE_NONE;
    BOOST_FOREACH(const sptr &node, descendant_rate_nodes) {
        // FIXME we need to know the port!!!
        size_t port = ANY_PORT; // NOOO! this is wrong!!!!
        double scaling = node->get_output_scale_factor(port);
        if (scaling == SCALE_NONE) {
            continue;
        }
        if (ret_val == SCALE_NONE) {
            ret_val = scaling;
            // TODO: Remember name of this node so we can make the throw message more descriptive.
            continue;
        }
        if (scaling != ret_val) {
            throw uhd::runtime_error(
                str(
                    // TODO add node names
                    boost::format("Conflicting scaling values: One upstream block wants %f, another wants %f.")
                    % scaling % ret_val
                )
            );
        }
    }
    return ret_val;
}

// TODO get rid of this code redundancy
double scalar_node_ctrl::_get_downstream_unique_scale_factor()
{
    std::vector< sptr > descendant_rate_nodes = find_downstream_node<scalar_node_ctrl>();
    double ret_val = SCALE_NONE;
    BOOST_FOREACH(const sptr &node, descendant_rate_nodes) {
        // FIXME we need to know the port!!!
        size_t port = ANY_PORT; // NOOO! this is wrong!!!!
        double scaling = node->get_input_scale_factor(port);
        if (scaling == SCALE_NONE) {
            continue;
        }
        if (ret_val == SCALE_NONE) {
            ret_val = scaling;
            // TODO: Remember name of this node so we can make the throw message more descriptive.
            continue;
        }
        if (scaling != ret_val) {
            throw uhd::runtime_error(
                str(
                    // TODO add node names
                    boost::format("Conflicting scaling values: One upstream block wants %f, another wants %f.")
                    % scaling % ret_val
                )
            );
        }
    }
    return ret_val;
}

