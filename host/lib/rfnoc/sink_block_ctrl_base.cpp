//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/rfnoc/sink_block_ctrl_base.hpp>
#include <uhd/rfnoc/constants.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/rfnoc/utils.hpp>

using namespace uhd;
using namespace uhd::rfnoc;


/***********************************************************************
 * Stream signatures
 **********************************************************************/
stream_sig_t sink_block_ctrl_base::get_input_signature(size_t block_port) const
{
    if (not _tree->exists(_root_path / "ports" / "in" / block_port)) {
        throw uhd::runtime_error(str(
                boost::format("Invalid port number %d for block %s")
                % block_port % unique_id()
        ));
    }

    return _resolve_port_def(
            _tree->access<blockdef::port_t>(_root_path / "ports" / "in" / block_port).get()
    );
}

std::vector<size_t> sink_block_ctrl_base::get_input_ports() const
{
    std::vector<size_t> input_ports;
    input_ports.reserve(_tree->list(_root_path / "ports" / "in").size());
    for(const std::string port:  _tree->list(_root_path / "ports" / "in")) {
        input_ports.push_back(boost::lexical_cast<size_t>(port));
    }
    return input_ports;
}

/***********************************************************************
 * FPGA Configuration
 **********************************************************************/
size_t sink_block_ctrl_base::get_fifo_size(size_t block_port) const {
    if (_tree->exists(_root_path / "input_buffer_size" / str(boost::format("%d") % block_port))) {
        return _tree->access<size_t>(_root_path / "input_buffer_size" / str(boost::format("%d") % block_port)).get();
    }
    return 0;
}

void sink_block_ctrl_base::configure_flow_control_in(
        size_t cycles,
        size_t packets,
        size_t block_port
) {
    UHD_RFNOC_BLOCK_TRACE() << boost::format("sink_block_ctrl_base::configure_flow_control_in(cycles=%d, packets=%d)") % cycles % packets ;
    uint32_t cycles_word = 0;
    if (cycles) {
        cycles_word = (1<<31) | cycles;
    }
    sr_write(SR_FLOW_CTRL_CYCS_PER_ACK, cycles_word, block_port);

    uint32_t packets_word = 0;
    if (packets) {
        packets_word = (1<<31) | packets;
    }
    sr_write(SR_FLOW_CTRL_PKTS_PER_ACK, packets_word, block_port);
}

void sink_block_ctrl_base::set_error_policy(
    const std::string &policy
) {
    if (policy == "next_packet")
    {
        sr_write(SR_ERROR_POLICY, (1 << 2) | 1);
    }
    else if (policy == "next_burst")
    {
        sr_write(SR_ERROR_POLICY, (1 << 3) | 1);
    }
    else if (policy == "continue")
    {
        sr_write(SR_ERROR_POLICY, (1 << 1) | 1);
    }
    else if (policy == "wait")
    {
        sr_write(SR_ERROR_POLICY, 1);
    }
    else throw uhd::value_error("Block input cannot handle requested error policy: " + policy);
}

/***********************************************************************
 * Hooks
 **********************************************************************/
size_t sink_block_ctrl_base::_request_input_port(
        const size_t suggested_port,
        const uhd::device_addr_t &
) const {
    const std::set<size_t> valid_input_ports = utils::str_list_to_set<size_t>(_tree->list(_root_path / "ports" / "in"));
    return utils::node_map_find_first_free(_upstream_nodes, suggested_port, valid_input_ports);
}
// vim: sw=4 et:
