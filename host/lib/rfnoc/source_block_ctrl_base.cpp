//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/rfnoc/source_block_ctrl_base.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/rfnoc/constants.hpp>
#include <uhdlib/rfnoc/utils.hpp>
#include <chrono>
#include <thread>

using namespace uhd;
using namespace uhd::rfnoc;

/***********************************************************************
 * Streaming operations
 **********************************************************************/
void source_block_ctrl_base::issue_stream_cmd(
        const uhd::stream_cmd_t &stream_cmd,
        const size_t chan
) {
    UHD_RFNOC_BLOCK_TRACE() << "source_block_ctrl_base::issue_stream_cmd()" ;
    if (_upstream_nodes.empty()) {
        UHD_LOGGER_WARNING("RFNOC") << "issue_stream_cmd() not implemented for " << get_block_id() ;
        return;
    }

    for(const node_ctrl_base::node_map_pair_t upstream_node:  _upstream_nodes) {
        source_node_ctrl::sptr this_upstream_block_ctrl =
            boost::dynamic_pointer_cast<source_node_ctrl>(upstream_node.second.lock());
        this_upstream_block_ctrl->issue_stream_cmd(stream_cmd, chan);
    }
}

/***********************************************************************
 * Stream signatures
 **********************************************************************/
stream_sig_t source_block_ctrl_base::get_output_signature(size_t block_port) const
{
    if (not _tree->exists(_root_path / "ports" / "out" / block_port)) {
        throw uhd::runtime_error(str(
                boost::format("Invalid port number %d for block %s")
                % block_port % unique_id()
        ));
    }

    return _resolve_port_def(
            _tree->access<blockdef::port_t>(_root_path / "ports" / "out" / block_port).get()
    );
}

std::vector<size_t> source_block_ctrl_base::get_output_ports() const
{
    std::vector<size_t> output_ports;
    output_ports.reserve(_tree->list(_root_path / "ports" / "out").size());
    for(const std::string port:  _tree->list(_root_path / "ports" / "out")) {
        output_ports.push_back(boost::lexical_cast<size_t>(port));
    }
    return output_ports;
}

/***********************************************************************
 * FPGA Configuration
 **********************************************************************/
void source_block_ctrl_base::set_destination(
        uint32_t next_address,
        size_t output_block_port
) {
    UHD_RFNOC_BLOCK_TRACE() << "source_block_ctrl_base::set_destination() " << uhd::sid_t(next_address) ;
    sid_t new_sid(next_address);
    new_sid.set_src(get_address(output_block_port));
    UHD_RFNOC_BLOCK_TRACE() << "  Setting SID: " << new_sid  << "  ";
    sr_write(SR_NEXT_DST_SID, (1<<16) | next_address, output_block_port);
}

void source_block_ctrl_base::configure_flow_control_out(
            size_t buf_size_pkts,
            size_t block_port,
            UHD_UNUSED(const uhd::sid_t &sid)
) {
    UHD_RFNOC_BLOCK_TRACE() << "source_block_ctrl_base::configure_flow_control_out() buf_size_pkts==" << buf_size_pkts ;
    if (buf_size_pkts < 2) {
      throw uhd::runtime_error(str(
              boost::format("Invalid window size %d for block %s. Window size must at least be 2.")
              % buf_size_pkts % unique_id()
      ));
    }

    //Disable the window and let all upstream data flush out
    //We need to do this every time the window is changed because
    //a) We don't know what state the flow-control module was left in
    //   in the previous run (it should still be enabled)
    //b) Changing the window size where data is buffered upstream may
    //   result in stale packets entering the stream.
    sr_write(SR_FLOW_CTRL_WINDOW_EN, 0, block_port);

    //Wait for data to flush out.
    //In the FPGA we are guaranteed that all buffered packets are more-or-less consecutive.
    //1ms@200MHz = 200,000 cycles of "flush time".
    //200k cycles = 200k * 8 bytes (64 bits) = 1.6MB of data that can be flushed.
    //Typically in the FPGA we have buffering in the order of kilobytes so waiting for 1MB
    //to flush is more than enough time.
    //TODO: Enhancement. We should get feedback from the FPGA about when the source_flow_control
    //      module is done flushing.
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    //Resize the FC window.
    //Precondition: No data can be buffered upstream.
    sr_write(SR_FLOW_CTRL_WINDOW_SIZE, buf_size_pkts, block_port);

    //Enable the FC window.
    //Precondition: The window size must be set.
    sr_write(SR_FLOW_CTRL_WINDOW_EN, (buf_size_pkts != 0), block_port);
}

/***********************************************************************
 * Hooks
 **********************************************************************/
size_t source_block_ctrl_base::_request_output_port(
        const size_t suggested_port,
        const uhd::device_addr_t &
) const {
    const std::set<size_t> valid_output_ports = utils::str_list_to_set<size_t>(_tree->list(_root_path / "ports" / "out"));
    return utils::node_map_find_first_free(_downstream_nodes, suggested_port, valid_output_ports);
}
// vim: sw=4 et:
