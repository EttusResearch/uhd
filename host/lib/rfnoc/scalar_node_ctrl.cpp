//
// Copyright 2014-2015 Ettus Research LLC
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

#include <uhd/rfnoc/scalar_node_ctrl.hpp>
#include <boost/bind.hpp>

using namespace uhd::rfnoc;

const double scalar_node_ctrl::SCALE_UNDEFINED = -1.0;

static double _get_input_factor(scalar_node_ctrl::sptr node, size_t port)
{
    return node->get_input_scale_factor(port);
}

static double _get_output_factor(scalar_node_ctrl::sptr node, size_t port)
{
    return node->get_output_scale_factor(port);
}

// FIXME add recursion limiters (i.e. list of explored nodes)
double scalar_node_ctrl::get_input_scale_factor(
        size_t /* port */
) {
    try {
        return find_downstream_unique_property<scalar_node_ctrl, double>(
                boost::bind(_get_input_factor, _1, _2),
                SCALE_UNDEFINED
        );
    } catch (const uhd::runtime_error &ex) {
        throw uhd::runtime_error(str(
            boost::format("Multiple scaling factors rates downstream of %s: %s.")
            % unique_id() % ex.what()
        ));
    }
}

double scalar_node_ctrl::get_output_scale_factor(
        size_t /* port */
) {
    try {
        return find_upstream_unique_property<scalar_node_ctrl, double>(
                boost::bind(_get_output_factor, _1, _2),
                SCALE_UNDEFINED
        );
    } catch (const uhd::runtime_error &ex) {
        throw uhd::runtime_error(str(
            boost::format("Multiple scaling factors rates upstream of %s: %s.")
            % unique_id() % ex.what()
        ));
    }
}
