//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "../transport/super_recv_packet_handler.hpp"
#include <uhd/utils/log.hpp>
#include <uhd/rfnoc/source_node_ctrl.hpp>
#include <uhdlib/rfnoc/rx_stream_terminator.hpp>
#include <uhdlib/rfnoc/radio_ctrl_impl.hpp>
#include <boost/format.hpp>

using namespace uhd::rfnoc;

size_t rx_stream_terminator::_count = 0;

rx_stream_terminator::rx_stream_terminator() :
    _term_index(_count),
    _samp_rate(rate_node_ctrl::RATE_UNDEFINED),
    _tick_rate(tick_node_ctrl::RATE_UNDEFINED)
{
    _count++;
}

std::string rx_stream_terminator::unique_id() const
{
    return str(boost::format("RX Terminator %d") % _term_index);
}

void rx_stream_terminator::set_tx_streamer(bool, const size_t)
{
    /* nop */
}

void rx_stream_terminator::set_rx_streamer(bool active, const size_t)
{
    // TODO this is identical to source_node_ctrl::set_rx_streamer() -> factor out
    UHD_RFNOC_BLOCK_TRACE() << "rx_stream_terminator::set_rx_streamer() " << active;
    for(const node_ctrl_base::node_map_pair_t upstream_node:  _upstream_nodes) {
        source_node_ctrl::sptr curr_upstream_block_ctrl =
            boost::dynamic_pointer_cast<source_node_ctrl>(upstream_node.second.lock());
        if (curr_upstream_block_ctrl) {
            curr_upstream_block_ctrl->set_rx_streamer(
                    active,
                    get_upstream_port(upstream_node.first)
            );
        }
        _rx_streamer_active[upstream_node.first] = active;
    }
}

void rx_stream_terminator::handle_overrun(boost::weak_ptr<uhd::rx_streamer> streamer, const size_t)
{
    std::unique_lock<std::mutex> l(_overrun_handler_mutex, std::defer_lock);
    if (!l.try_lock()) {
        // We're already handling overruns, so just stop right there
        return;
    }

    std::vector<boost::shared_ptr<uhd::rfnoc::radio_ctrl_impl> > upstream_radio_nodes =
        find_upstream_node<uhd::rfnoc::radio_ctrl_impl>();
    const size_t n_radios = upstream_radio_nodes.size();
    if (n_radios == 0) {
        return;
    }

    UHD_RFNOC_BLOCK_TRACE() << "rx_stream_terminator::handle_overrun()" ;
    boost::shared_ptr<uhd::transport::sph::recv_packet_streamer> my_streamer =
            boost::dynamic_pointer_cast<uhd::transport::sph::recv_packet_streamer>(streamer.lock());
    if (not my_streamer) return; //If the rx_streamer has expired then overflow handling makes no sense.

    bool in_continuous_streaming_mode = true;
    int num_channels = 0;
    for(const boost::shared_ptr<uhd::rfnoc::radio_ctrl_impl> &node:  upstream_radio_nodes) {
        num_channels += node->get_active_rx_ports().size();
        for(const size_t port:  node->get_active_rx_ports()) {
            in_continuous_streaming_mode = in_continuous_streaming_mode && node->in_continuous_streaming_mode(port);
        }
    }
    if (num_channels == 0) {
        return;
    }

    if (num_channels == 1 and in_continuous_streaming_mode) {
        std::vector<size_t> active_rx_ports = upstream_radio_nodes[0]->get_active_rx_ports();
        if (active_rx_ports.empty()) {
            return;
        }
        const size_t port = active_rx_ports[0];
        upstream_radio_nodes[0]->issue_stream_cmd(stream_cmd_t::STREAM_MODE_START_CONTINUOUS, port);
        return;
    }

    /////////////////////////////////////////////////////////////
    // MIMO overflow recovery time
    /////////////////////////////////////////////////////////////
    for(const boost::shared_ptr<uhd::rfnoc::radio_ctrl_impl> &node:  upstream_radio_nodes) {
        for(const size_t port:  node->get_active_rx_ports()) {
            // check all the ports on all the radios
            node->rx_ctrl_clear_cmds(port);
            node->issue_stream_cmd(stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS, port);
        }
    }
    //flush transports
    my_streamer->flush_all(0.001); // TODO flushing will probably have to go away.
    //restart streaming on all channels
    if (in_continuous_streaming_mode) {
        stream_cmd_t stream_cmd(stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
        stream_cmd.stream_now = false;
        stream_cmd.time_spec = upstream_radio_nodes[0]->get_time_now() + time_spec_t(0.05);

        for(const boost::shared_ptr<uhd::rfnoc::radio_ctrl_impl> &node:  upstream_radio_nodes) {
            for(const size_t port:  node->get_active_rx_ports()) {
                node->issue_stream_cmd(stream_cmd, port);
            }
        }
    }
}

rx_stream_terminator::~rx_stream_terminator()
{
    UHD_RFNOC_BLOCK_TRACE() << "rx_stream_terminator::~rx_stream_terminator() " ;
    set_rx_streamer(false, 0);
}

