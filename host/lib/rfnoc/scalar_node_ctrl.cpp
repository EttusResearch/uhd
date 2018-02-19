//
// Copyright 2014-2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
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
