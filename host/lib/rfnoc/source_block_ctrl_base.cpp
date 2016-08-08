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
#include <uhd/rfnoc/source_block_ctrl_base.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/rfnoc/constants.hpp>

using namespace uhd;
using namespace uhd::rfnoc;

/***********************************************************************
 * Streaming operations
 **********************************************************************/
void source_block_ctrl_base::issue_stream_cmd(
        const uhd::stream_cmd_t &stream_cmd,
        const size_t chan
) {
    UHD_RFNOC_BLOCK_TRACE() << "source_block_ctrl_base::issue_stream_cmd()" << std::endl;
    if (_upstream_nodes.empty()) {
        UHD_MSG(warning) << "issue_stream_cmd() not implemented for " << get_block_id() << std::endl;
        return;
    }

    BOOST_FOREACH(const node_ctrl_base::node_map_pair_t upstream_node, _upstream_nodes) {
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
    BOOST_FOREACH(const std::string port, _tree->list(_root_path / "ports" / "out")) {
        output_ports.push_back(boost::lexical_cast<size_t>(port));
    }
    return output_ports;
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
    new_sid.set_src(get_address(output_block_port));
    UHD_RFNOC_BLOCK_TRACE() << "  Setting SID: " << new_sid << std::endl << "  ";
    sr_write(SR_NEXT_DST_SID, (1<<16) | next_address, output_block_port);
}

void source_block_ctrl_base::configure_flow_control_out(
            size_t buf_size_pkts,
            size_t block_port,
            UHD_UNUSED(const uhd::sid_t &sid)
) {
    UHD_RFNOC_BLOCK_TRACE() << "source_block_ctrl_base::configure_flow_control_out() buf_size_pkts==" << buf_size_pkts << std::endl;
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
    boost::this_thread::sleep(boost::posix_time::milliseconds(1));

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
