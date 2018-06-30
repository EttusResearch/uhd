//
// Copyright 2014-2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/rfnoc/sink_node_ctrl.hpp>
#include <uhdlib/rfnoc/tx_stream_terminator.hpp>
#include <boost/format.hpp>

using namespace uhd::rfnoc;

size_t tx_stream_terminator::_count = 0;

tx_stream_terminator::tx_stream_terminator() :
    _term_index(_count),
    _samp_rate(rate_node_ctrl::RATE_UNDEFINED),
    _tick_rate(tick_node_ctrl::RATE_UNDEFINED)
{
    _count++;
}

std::string tx_stream_terminator::unique_id() const
{
    return str(boost::format("TX Terminator %d") % _term_index);
}

void tx_stream_terminator::set_rx_streamer(bool, const size_t)
{
    /* nop */
}

void tx_stream_terminator::set_tx_streamer(bool active, const size_t /* port */)
{
    // TODO this is identical to sink_node_ctrl::set_tx_streamer() -> factor out
    UHD_RFNOC_BLOCK_TRACE() << "tx_stream_terminator::set_tx_streamer() " << active;
    for(const node_ctrl_base::node_map_pair_t downstream_node:  _downstream_nodes) {
        sink_node_ctrl::sptr curr_downstream_block_ctrl =
            boost::dynamic_pointer_cast<sink_node_ctrl>(downstream_node.second.lock());
        if (curr_downstream_block_ctrl) {
            curr_downstream_block_ctrl->set_tx_streamer(
                    active,
                    get_downstream_port(downstream_node.first)
            );
        }
        _tx_streamer_active[downstream_node.first] = active;
    }

}

tx_stream_terminator::~tx_stream_terminator()
{
    UHD_RFNOC_BLOCK_TRACE() << "tx_stream_terminator::~tx_stream_terminator() " ;
    set_tx_streamer(false, 0);
}

