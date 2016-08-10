//
// Copyright 2016 Ettus Research LLC
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

#include "graph_impl.hpp"
#include <uhd/rfnoc/source_block_ctrl_base.hpp>
#include <uhd/rfnoc/sink_block_ctrl_base.hpp>
#include <uhd/utils/msg.hpp>

using namespace uhd::rfnoc;

/****************************************************************************
 * Structors
 ***************************************************************************/
graph_impl::graph_impl(
            const std::string &name,
            boost::weak_ptr<uhd::device3> device_ptr
            //async_msg_handler::sptr msg_handler
) : _name(name)
  , _device_ptr(device_ptr)
{

}


/****************************************************************************
 * Connection API
 ***************************************************************************/
void graph_impl::connect(
        const block_id_t &src_block,
        size_t src_block_port,
        const block_id_t &dst_block,
        size_t dst_block_port,
        const size_t pkt_size_
) {
    device3::sptr device_ptr = _device_ptr.lock();
    if (not device_ptr) {
        throw uhd::runtime_error("Invalid device");
    }

    uhd::rfnoc::source_block_ctrl_base::sptr src = device_ptr->get_block_ctrl<rfnoc::source_block_ctrl_base>(src_block);
    uhd::rfnoc::sink_block_ctrl_base::sptr dst = device_ptr->get_block_ctrl<rfnoc::sink_block_ctrl_base>(dst_block);

    /********************************************************************
     * 1. Draw the edges (logically connect the nodes)
     ********************************************************************/
    size_t actual_src_block_port = src->connect_downstream(
            boost::dynamic_pointer_cast<uhd::rfnoc::node_ctrl_base>(dst),
            src_block_port
    );
    if (src_block_port == uhd::rfnoc::ANY_PORT) {
        src_block_port = actual_src_block_port;
    } else if (src_block_port != actual_src_block_port) {
        throw uhd::runtime_error(str(
            boost::format("Can't connect to port %d on block %s.")
            % src_block_port % src->unique_id()
        ));
    }
    size_t actual_dst_block_port = dst->connect_upstream(
            boost::dynamic_pointer_cast<uhd::rfnoc::node_ctrl_base>(src),
            dst_block_port
    );
    if (dst_block_port == uhd::rfnoc::ANY_PORT) {
        dst_block_port = actual_dst_block_port;
    } else if (dst_block_port != actual_dst_block_port) {
        throw uhd::runtime_error(str(
            boost::format("Can't connect to port %d on block %s.")
            % dst_block_port % dst->unique_id()
        ));
    }
    src->set_downstream_port(actual_src_block_port, actual_dst_block_port);
    dst->set_upstream_port(actual_dst_block_port, actual_src_block_port);
    // At this point, ports are locked and no one else can simply connect
    // into them.
    //UHD_MSG(status)
        //<< "[" << _name << "] Connecting "
        //<< src_block << ":" << actual_src_block_port << " --> "
        //<< dst_block << ":" << actual_dst_block_port << std::endl;

    /********************************************************************
     * 2. Check IO signatures match
     ********************************************************************/
    if (not rfnoc::stream_sig_t::is_compatible(
                src->get_output_signature(actual_src_block_port),
                dst->get_input_signature(actual_dst_block_port)
        )) {
        throw uhd::runtime_error(str(
            boost::format("Can't connect block %s to %s: IO signature mismatch\n(%s is incompatible with %s).")
            % src->get_block_id().get() % dst->get_block_id().get()
            % src->get_output_signature(actual_src_block_port)
            % dst->get_input_signature(actual_dst_block_port)
        ));
    }

    /********************************************************************
     * 3. Configure the source block's destination
     ********************************************************************/
    // Calculate SID
    sid_t sid = dst->get_address(dst_block_port);
    sid.set_src(src->get_address(src_block_port));

    // Set SID on source block
    src->set_destination(sid.get(), src_block_port);

    /********************************************************************
     * 4. Configure flow control
     ********************************************************************/
    size_t pkt_size = (pkt_size_ != 0) ? pkt_size_ : src->get_output_signature(src_block_port).packet_size;
    if (pkt_size == 0) { // Unspecified packet rate. Assume max packet size.
        UHD_MSG(status) << "Assuming max packet size for " << src->get_block_id() << std::endl;
        pkt_size = uhd::rfnoc::MAX_PACKET_SIZE;
    }
    // FC window (in packets) depends on FIFO size...          ...and packet size.
    size_t buf_size_pkts = dst->get_fifo_size(dst_block_port) / pkt_size;
    if (buf_size_pkts == 0) {
        throw uhd::runtime_error(str(
            boost::format("Input FIFO for block %s is too small (%d kiB) for packets of size %d kiB\n"
                          "coming from block %s.")
            % dst->get_block_id().get() % (dst->get_fifo_size(dst_block_port) / 1024)
            % (pkt_size / 1024) % src->get_block_id().get()
        ));
    }
    src->configure_flow_control_out(buf_size_pkts, src_block_port);
    // On the same crossbar, use lots of FC packets
    size_t pkts_per_ack = std::min(
            uhd::rfnoc::DEFAULT_FC_XBAR_PKTS_PER_ACK,
            buf_size_pkts - 1
    );
    // Over the network, use less or we'd flood the transport
    if (sid.get_src_addr() != sid.get_dst_addr()) {
        pkts_per_ack = std::max<size_t>(buf_size_pkts / uhd::rfnoc::DEFAULT_FC_TX_RESPONSE_FREQ, 1);
    }
    dst->configure_flow_control_in(
            0, // Default to not use cycles
            pkts_per_ack,
            dst_block_port
    );

    /********************************************************************
     * 5. Configure error policy
     ********************************************************************/
    dst->set_error_policy("next_burst");
}

void graph_impl::connect(
        const block_id_t &src_block,
        const block_id_t &dst_block
) {
    connect(src_block, ANY_PORT, dst_block, ANY_PORT);
}

