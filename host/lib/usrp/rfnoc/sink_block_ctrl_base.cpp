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

#include "utils.hpp"
#include <uhd/usrp/rfnoc/sink_block_ctrl_base.hpp>
#include <uhd/usrp/rfnoc/constants.hpp>
#include <uhd/utils/msg.hpp>

using namespace uhd;
using namespace uhd::rfnoc;

/***********************************************************************
 * Streaming operations
 **********************************************************************/
void sink_block_ctrl_base::setup_tx_streamer(uhd::stream_args_t &args)
{
    UHD_RFNOC_BLOCK_TRACE() << "sink_block_ctrl_base::setup_tx_streamer()" << std::endl;

    // 0. Check if args collides with our own options
    BOOST_FOREACH(const std::string key, _args.keys()) {
        if (args.args.has_key(key) and _args[key] != args.args[key]) {
            throw uhd::runtime_error(
                str(boost::format(
                        "Conflicting options for block %s: Block options require '%s' == '%s',\n"
                        "but streamer requests '%s' == '%s'."
                        ) % get_block_id().get() % key % _args[key] % key % args.args[key]
                )
            );
        }
        args.args[key] = _args[key];
    }

    // 1. Call our own init_tx() function
    // This should modify "args" if necessary.
    _init_tx(args);

    // 2. Check if we're the last block
    if (_is_final_tx_block()) {
        UHD_RFNOC_BLOCK_TRACE() << "sink_block_ctrl_base::setup_tx_streamer(): Final block, returning. " << std::endl;
        return;
    }

    // 3. Call all downstream blocks
    BOOST_FOREACH(const node_ctrl_base::node_map_pair_t downstream_node, _downstream_nodes) {
        // Make a copy so that modifications downstream aren't propagated upstream
        uhd::stream_args_t new_args = args;
        sptr this_downstream_block_ctrl =
            boost::dynamic_pointer_cast<sink_block_ctrl_base>(downstream_node.second.lock());
        if (this_downstream_block_ctrl) {
            this_downstream_block_ctrl->setup_tx_streamer(new_args);
        }
    }
}

/***********************************************************************
 * Stream signatures
 **********************************************************************/
stream_sig_t sink_block_ctrl_base::get_input_signature(size_t block_port) const
{
    UHD_RFNOC_BLOCK_TRACE() << "block_ctrl_base::get_input_signature() " << std::endl;
    if (not _tree->exists(_root_path / "input_sig" / str(boost::format("%d") % block_port))) {
        throw uhd::runtime_error(str(
            boost::format("Can't query input signature on block %s: Port %d is not defined.")
            % get_block_id().to_string() % block_port
        ));
    }
    return _tree->access<stream_sig_t>(_root_path / "input_sig" / str(boost::format("%d") % block_port)).get();
}

bool sink_block_ctrl_base::set_input_signature(const stream_sig_t &sig, size_t block_port)
{
    UHD_RFNOC_BLOCK_TRACE() << "block_ctrl_base::set_input_signature() " << sig << " " << block_port << std::endl;

    /// Check if valid block port:
    if (not _tree->exists(_root_path / "input_sig" / block_port)) {
        throw uhd::runtime_error(str(
            boost::format("Can't modify input signature on block %s: Port %d is not defined.")
            % get_block_id().to_string() % block_port
        ));
    }

    //if (not
        //_tree->access<stream_sig_t>(_root_path / "input_sig" / str(boost::format("%d") % block_port))
        //.get().is_compatible(in_sig)
    //) {
        //return false;
    //}

    //// TODO more and better rules, check block definition
    //if (in_sig.packet_size % BYTES_PER_LINE) {
        //return false;
    //}

    _tree->access<stream_sig_t>(_root_path / "input_sig" / block_port).set(sig);
    // FIXME figure out good rules to propagate the signature
    _tree->access<stream_sig_t>(_root_path / "output_sig" / block_port).set(sig);
    return true;
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
    UHD_RFNOC_BLOCK_TRACE() << boost::format("sink_block_ctrl_base::configure_flow_control_in(cycles=%d, packets=%d)") % cycles % packets << std::endl;
    boost::uint32_t cycles_word = 0;
    if (cycles) {
        cycles_word = (1<<31) | cycles;
    }
    sr_write(SR_FLOW_CTRL_CYCS_PER_ACK_BASE + block_port, cycles_word);

    boost::uint32_t packets_word = 0;
    if (packets) {
        packets_word = (1<<31) | packets;
    }
    sr_write(SR_FLOW_CTRL_PKTS_PER_ACK_BASE + block_port, packets_word);
}


/***********************************************************************
 * Hooks
 **********************************************************************/
size_t sink_block_ctrl_base::_request_input_port(
        const size_t suggested_port,
        const uhd::device_addr_t &
) const {
    const std::set<size_t> valid_input_ports = utils::str_list_to_set<size_t>(_tree->list(_root_path / "input_sig"));
    return utils::node_map_find_first_free(_upstream_nodes, suggested_port, valid_input_ports);
}
// vim: sw=4 et:
