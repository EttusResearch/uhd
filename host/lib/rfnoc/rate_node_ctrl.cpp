//
// Copyright 2014-2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/rfnoc/rate_node_ctrl.hpp>
#include <boost/bind.hpp>

using namespace uhd::rfnoc;

const double rate_node_ctrl::RATE_UNDEFINED = -1.0;

static double _get_input_samp_rate(rate_node_ctrl::sptr node, size_t port)
{
    return node->get_input_samp_rate(port);
}

static double _get_output_samp_rate(rate_node_ctrl::sptr node, size_t port)
{
    return node->get_output_samp_rate(port);
}


// FIXME add recursion limiters (i.e. list of explored nodes)
double rate_node_ctrl::get_input_samp_rate(
        size_t /* port */
) {
    try {
        return find_downstream_unique_property<rate_node_ctrl, double>(
                boost::bind(_get_input_samp_rate, _1, _2),
                RATE_UNDEFINED
        );
    } catch (const uhd::runtime_error &ex) {
        throw uhd::runtime_error(str(
            boost::format("Multiple sampling rates downstream of %s: %s.")
            % unique_id() % ex.what()
        ));
    }
}

double rate_node_ctrl::get_output_samp_rate(
        size_t /* port */
) {
    try {
        return find_upstream_unique_property<rate_node_ctrl, double>(
                boost::bind(_get_output_samp_rate, _1, _2),
                RATE_UNDEFINED
        );
    } catch (const uhd::runtime_error &ex) {
        throw uhd::runtime_error(str(
            boost::format("Multiple sampling rates upstream of %s: %s.")
            % unique_id() % ex.what()
        ));
    }
}
