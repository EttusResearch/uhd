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

#include <uhd/usrp/rfnoc/rate_node_ctrl.hpp>

using namespace uhd::rfnoc;

// FIXME add recursion limiters (i.e. list of explored nodes)
double rate_node_ctrl::get_input_samp_rate(
        size_t /* port */
) {
    return _get_downstream_unique_input_samp_rate();
}

double rate_node_ctrl::get_output_samp_rate(
        size_t /* port */
) {
    return _get_upstream_unique_output_samp_rate();
}

double rate_node_ctrl::_get_downstream_unique_input_samp_rate()
{
    std::vector< sptr > descendant_rate_nodes = find_downstream_node<rate_node_ctrl>();
    double ret_val = RATE_NONE;
    BOOST_FOREACH(const sptr &node, descendant_rate_nodes) {
        // FIXME we need to know the port!!!
        size_t port = ANY_PORT; // NOOO! this is wrong!!!!
        double samp_rate = node->get_input_samp_rate(port);
        if (samp_rate == RATE_NONE) {
            continue;
        }
        if (ret_val == RATE_NONE) {
            ret_val = samp_rate;
            // TODO: Remember name of this node so we can make the throw message more descriptive.
            continue;
        }
        if (samp_rate != ret_val) {
            throw uhd::runtime_error(
                str(
                    // TODO add node names
                    boost::format("Conflicting sampling rates: One downstream block wants %d Msps, another wants %d Msps.")
                    % samp_rate % ret_val
                )
            );
        }
    }
    return ret_val;
}

// TODO get rid of this code redundancy
double rate_node_ctrl::_get_upstream_unique_output_samp_rate()
{
    std::vector< sptr > descendant_rate_nodes = find_upstream_node<rate_node_ctrl>();
    double ret_val = RATE_NONE;
    BOOST_FOREACH(const sptr &node, descendant_rate_nodes) {
        // FIXME we need to know the port!!!
        size_t port = ANY_PORT; // NOOO! this is wrong!!!!
        double samp_rate = node->get_output_samp_rate(port);
        if (samp_rate == RATE_NONE) {
            continue;
        }
        if (ret_val == RATE_NONE) {
            ret_val = samp_rate;
            // TODO: Remember name of this node so we can make the throw message more descriptive.
            continue;
        }
        if (samp_rate != ret_val) {
            throw uhd::runtime_error(
                str(
                    // TODO add node names
                    boost::format("Conflicting sampling rates: One upstream block wants %d Msps, another wants %d Msps.")
                    % samp_rate % ret_val
                )
            );
        }
    }
    return ret_val;
}
