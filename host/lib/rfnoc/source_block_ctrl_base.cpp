//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/rfnoc/constants.hpp>
#include <uhd/rfnoc/source_block_ctrl_base.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/rfnoc/utils.hpp>
#include <chrono>
#include <thread>

using namespace uhd;
using namespace uhd::rfnoc;

/***********************************************************************
 * Streaming operations
 **********************************************************************/
void source_block_ctrl_base::issue_stream_cmd(
    const uhd::stream_cmd_t& stream_cmd, const size_t chan)
{
    UHD_RFNOC_BLOCK_TRACE() << "source_block_ctrl_base::issue_stream_cmd()";
    if (_upstream_nodes.empty()) {
        UHD_LOGGER_WARNING("RFNOC")
            << "issue_stream_cmd() not implemented for " << get_block_id();
        return;
    }

    for (const node_ctrl_base::node_map_pair_t upstream_node : _upstream_nodes) {
        // FIXME:  Need proper mapping from input port to output port
        // The code below assumes the input port and output port are the same
        // if the number of upstream and downstream ports are the same.
        // The stream command is limited to only that port to prevent issuing
        // it on the wrong block and port.
        if (_num_input_ports == _num_output_ports
            and upstream_node.first != chan) {
            continue;
        }
        source_node_ctrl::sptr this_upstream_block_ctrl =
            boost::dynamic_pointer_cast<source_node_ctrl>(upstream_node.second.lock());
        if (this_upstream_block_ctrl) {
            this_upstream_block_ctrl->issue_stream_cmd(
                stream_cmd, get_upstream_port(upstream_node.first));
        }
    }
}

/***********************************************************************
 * Stream signatures
 **********************************************************************/
stream_sig_t source_block_ctrl_base::get_output_signature(size_t block_port) const
{
    if (not _tree->exists(_root_path / "ports" / "out" / block_port)) {
        throw uhd::runtime_error(str(boost::format("Invalid port number %d for block %s")
                                     % block_port % unique_id()));
    }

    return _resolve_port_def(
        _tree->access<blockdef::port_t>(_root_path / "ports" / "out" / block_port).get());
}

std::vector<size_t> source_block_ctrl_base::get_output_ports() const
{
    std::vector<size_t> output_ports;
    output_ports.reserve(_tree->list(_root_path / "ports" / "out").size());
    for (const std::string port : _tree->list(_root_path / "ports" / "out")) {
        output_ports.push_back(boost::lexical_cast<size_t>(port));
    }
    return output_ports;
}

/***********************************************************************
 * FPGA Configuration
 **********************************************************************/
void source_block_ctrl_base::set_destination(
    uint32_t next_address, size_t output_block_port)
{
    UHD_RFNOC_BLOCK_TRACE() << "source_block_ctrl_base::set_destination() "
                            << uhd::sid_t(next_address);
    sid_t new_sid(next_address);
    new_sid.set_src(get_address(output_block_port));
    UHD_RFNOC_BLOCK_TRACE() << "  Setting SID: " << new_sid << "  ";
    sr_write(SR_NEXT_DST_SID, (1 << 16) | next_address, output_block_port);
}

void source_block_ctrl_base::configure_flow_control_out(const bool enable_fc_output,
    const bool lossless_link,
    const size_t buf_size_bytes,
    const size_t pkt_limit,
    const size_t block_port,
    UHD_UNUSED(const uhd::sid_t& sid))
{
    UHD_RFNOC_BLOCK_TRACE()
        << "source_block_ctrl_base::configure_flow_control_out() buf_size_bytes=="
        << buf_size_bytes;
    if (buf_size_bytes == 0) {
        throw uhd::runtime_error(
            str(boost::format(
                    "Invalid window size %d for block %s. Window size cannot be 0 bytes.")
                % buf_size_bytes % unique_id()));
    }

    // Enable source flow control module and conditionally enable byte based and/or packet
    // count based flow control
    const bool enable_byte_fc    = (buf_size_bytes != 0);
    const bool enable_pkt_cnt_fc = (pkt_limit != 0);
    const uint32_t config = (enable_fc_output ? 1 : 0) | ((enable_byte_fc ? 1 : 0) << 1)
                            | ((enable_pkt_cnt_fc ? 1 : 0) << 2)
                            | ((lossless_link ? 1 : 0) << 3);

    // Resize the FC window.
    // Precondition: No data can be buffered upstream.
    if (enable_byte_fc) {
        sr_write(SR_FLOW_CTRL_WINDOW_SIZE, buf_size_bytes, block_port);
    }
    if (enable_pkt_cnt_fc) {
        sr_write(SR_FLOW_CTRL_PKT_LIMIT, pkt_limit, block_port);
    }

    // Enable the FC window.
    // Precondition: The window size and/or packet limit must be set.
    sr_write(SR_FLOW_CTRL_EN, config, block_port);
}

size_t source_block_ctrl_base::get_mtu(size_t block_port) const
{
    if (_tree->exists(_root_path / "mtu" / std::to_string(block_port))) {
        return _tree->access<size_t>(_root_path / "mtu" / std::to_string(block_port))
            .get();
    }
    return 0;
}


/***********************************************************************
 * Hooks
 **********************************************************************/
size_t source_block_ctrl_base::_request_output_port(
    const size_t suggested_port, const uhd::device_addr_t&) const
{
    const std::set<size_t> valid_output_ports =
        utils::str_list_to_set<size_t>(_tree->list(_root_path / "ports" / "out"));
    return utils::node_map_find_first_free(
        _downstream_nodes, suggested_port, valid_output_ports);
}
// vim: sw=4 et:
