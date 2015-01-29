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
#include <uhd/usrp/rfnoc/source_block_ctrl_base.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/usrp/rfnoc/constants.hpp>

using namespace uhd;
using namespace uhd::rfnoc;

/***********************************************************************
 * Streaming operations
 **********************************************************************/
void source_block_ctrl_base::issue_stream_cmd(
        const uhd::stream_cmd_t &stream_cmd
) {
    UHD_RFNOC_BLOCK_TRACE() << "source_block_ctrl_base::issue_stream_cmd()" << std::endl;
    if (_upstream_nodes.empty()) {
        UHD_MSG(warning) << "issue_stream_cmd() not implemented for " << get_block_id() << std::endl;
        return;
    }

    BOOST_FOREACH(const node_ctrl_base::node_map_pair_t upstream_node, _upstream_nodes) {
        source_node_ctrl::sptr this_upstream_block_ctrl =
            boost::dynamic_pointer_cast<source_node_ctrl>(upstream_node.second.lock());
        this_upstream_block_ctrl->issue_stream_cmd(stream_cmd);
    }
}

// non-virtual
void source_block_ctrl_base::setup_rx_streamer(uhd::stream_args_t &args)
{
    UHD_RFNOC_BLOCK_TRACE() << "source_block_ctrl_base::setup_rx_streamer()" << std::endl;

    // 0. Check if args collides with our own _args
    // and merge them
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

    // 1. Call our own init_rx() function
    // This should modify "args" if necessary.
    _init_rx(args);

    // 2. Check if we're the last block
    if (_is_final_rx_block()) {
        UHD_MSG(status) << "source_block_ctrl_base::setup_rx_streamer(): Final block, returning. " << std::endl;
        return;
    }

    // 3. Call all upstream blocks
    BOOST_FOREACH(const node_ctrl_base::node_map_pair_t upstream_node, _upstream_nodes) {
        // Make a copy so that modifications upstream aren't propagated downstream
        uhd::stream_args_t new_args = args;
        sptr this_upstream_block_ctrl =
            boost::dynamic_pointer_cast<source_block_ctrl_base>(upstream_node.second.lock());
        if (this_upstream_block_ctrl) {
            this_upstream_block_ctrl->setup_rx_streamer(new_args);
        }
    }
}

/***********************************************************************
 * Stream signatures
 **********************************************************************/
stream_sig_t source_block_ctrl_base::get_output_signature(size_t block_port) const
{
    UHD_RFNOC_BLOCK_TRACE() << "source_block_ctrl_base::get_output_signature() " << std::endl;
    if (not _tree->exists(_root_path / "output_sig" / str(boost::format("%d") % block_port))) {
        throw uhd::runtime_error(str(
            boost::format("Can't query output signature on block %s: Port %d is not defined.")
            % get_block_id().to_string() % block_port
        ));
    }
    return _tree->access<stream_sig_t>(_root_path / "output_sig" / str(boost::format("%d") % block_port)).get();
}

bool source_block_ctrl_base::set_output_signature(const stream_sig_t &sig, size_t block_port)
{
    UHD_RFNOC_BLOCK_TRACE() << "source_block_ctrl_base::set_output_signature() " << sig << " " << block_port << std::endl;

    /// Check if valid block port:
    if (not _tree->exists(_root_path / "output_sig" / block_port)) {
        throw uhd::runtime_error(str(
            boost::format("Can't modify output signature on block %s: Port %d is not defined.")
            % get_block_id().to_string() % block_port
        ));
    }

    // TODO more and better rules, check block definition
    //if (out_sig.packet_size % BYTES_PER_LINE) {
        //return false;
    //}

    _tree->access<stream_sig_t>(_root_path / "output_sig" / block_port).set(sig);
    return true;
}

/***********************************************************************
 * FPGA Configuration
 **********************************************************************/
void source_block_ctrl_base::set_destination(
        boost::uint32_t next_address,
        size_t output_block_port
) {
    UHD_RFNOC_BLOCK_TRACE() << "source_block_ctrl_base::set_destination() " << uhd::sid_t(next_address) << std::endl;
    sid_t new_sid(next_address);
    new_sid.set_src_addr(get_ctrl_sid().get_dst_addr());
    new_sid.set_src_endpoint(get_ctrl_sid().get_dst_endpoint() + output_block_port);
    UHD_MSG(status) << "  Setting SID: " << new_sid << std::endl << "  ";
    sr_write(SR_NEXT_DST_BASE+output_block_port, (1<<16) | next_address);
}

void source_block_ctrl_base::configure_flow_control_out(
            size_t buf_size_pkts,
            size_t block_port,
            UHD_UNUSED(const uhd::sid_t &sid)
) {
    UHD_RFNOC_BLOCK_TRACE() << "source_block_ctrl_base::configure_flow_control_out() buf_size_pkts==" << buf_size_pkts << std::endl;
    // This actually takes counts between acks. So if the buffer size is 1 packet, we
    // set this to zero.
    sr_write(SR_FLOW_CTRL_WINDOW_SIZE_BASE + block_port, (buf_size_pkts == 0) ? 0 : buf_size_pkts-1);
    sr_write(SR_FLOW_CTRL_WINDOW_EN_BASE + block_port, (buf_size_pkts != 0));
}

/***********************************************************************
 * Hooks
 **********************************************************************/
size_t source_block_ctrl_base::_request_output_port(
        const size_t suggested_port,
        const uhd::device_addr_t &
) const {
    const std::set<size_t> valid_output_ports = utils::str_list_to_set<size_t>(_tree->list(_root_path / "output_sig"));
    return utils::node_map_find_first_free(_downstream_nodes, suggested_port, valid_output_ports);
}
// vim: sw=4 et:
