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

// Provides streaming-related functions which are used by device3 objects.

#include "device3_impl.hpp"
#include <uhd/usrp/rfnoc/constants.hpp>
#include <uhd/utils/byteswap.hpp>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;

/***********************************************************************
 * Helper functions for get_?x_stream()
 **********************************************************************/
uhd::stream_args_t device3_impl::sanitize_stream_args(const uhd::stream_args_t &args_)
{
    uhd::stream_args_t args = args_;
    if (args.otw_format.empty()) {
        args.otw_format = "sc16";
    }
    if (args.channels.empty()) {
        args.channels = std::vector<size_t>(1, 0);
    }

    return args;
}

void device3_impl::generate_channel_list(
        const uhd::stream_args_t &args_,
        std::vector<uhd::rfnoc::block_id_t> &chan_list,
        std::vector<device_addr_t> &chan_args,
        const std::string &xx
) {
    uhd::stream_args_t args = args_;
    if (args.args.has_key("block_id")) { // Override channel settings
        // TODO: Figure out how to put in more than one block ID in the stream args args
        // For now, the assumption is that all chans go to the same block,
        // and that the channel index is actually the block port index
        // Block ID is removed from the actual args args
        uhd::rfnoc::block_id_t blockid = args.args.pop("block_id");
        BOOST_FOREACH(const size_t chan_idx, args.channels) {
            chan_list.push_back(uhd::rfnoc::block_id_t(blockid));
            // Add block port to chan args
            args.args["block_port"] = str(boost::format("%d") % chan_idx);
            chan_args.push_back(args.args);
        }
    } else {
        BOOST_FOREACH(const size_t chan_idx, args.channels) {
            fs_path chan_root = str(boost::format("/channels/%s/%d") % xx % chan_idx);
            if (not _tree->exists(chan_root)) {
                throw uhd::runtime_error("No channel definition for " + chan_root);
            }
            device_addr_t this_chan_args;
            if (_tree->exists(chan_root / "args")) {
                this_chan_args = _tree->access<device_addr_t>(chan_root / "args").get();
            }
            chan_list.push_back(_tree->access<uhd::rfnoc::block_id_t>(chan_root).get());
            chan_args.push_back(this_chan_args);
        }
    }
}


/***********************************************************************
 * RX Flow Control Functions
 **********************************************************************/
size_t device3_impl::get_rx_flow_control_window(
        size_t pkt_size,
        size_t sw_buff_size,
        const device_addr_t& rx_args,
        const double fullness_
) {
    double fullness_factor = fullness_;
    if (fullness_factor == -1) {
        fullness_factor = rx_args.cast<double>("recv_buff_fullness", uhd::rfnoc::DEFAULT_FC_RX_SW_BUFF_FULL_FACTOR);
    }

    if (fullness_factor < 0.01 || fullness_factor > 1) {
        throw uhd::value_error("recv_buff_fullness must be between 0.01 and 1 inclusive (1% to 100%)");
    }

    size_t window_in_pkts = (static_cast<size_t>(sw_buff_size * fullness_factor) / pkt_size);
    if (window_in_pkts == 0) {
        throw uhd::value_error("recv_buff_size must be larger than the recv_pkt_size.");
    }
    return window_in_pkts;
}


void device3_impl::handle_rx_flowctrl(
        const sid_t &sid,
        zero_copy_if::sptr xport,
        bool big_endian,
        boost::shared_ptr<boost::uint32_t> seq32_state,
        const size_t last_seq
) {
    managed_send_buffer::sptr buff = xport->get_send_buff(0.0);
    if (not buff)
    {
        throw uhd::runtime_error("handle_rx_flowctrl timed out getting a send buffer");
    }
    boost::uint32_t *pkt = buff->cast<boost::uint32_t *>();


    //recover seq32
    boost::uint32_t &seq32 = *seq32_state;
    const size_t seq12 = seq32 & 0xfff;
    if (last_seq < seq12) seq32 += (1 << 12);
    seq32 &= ~0xfff;
    seq32 |= last_seq;

    // Super-verbose mode:
    //static size_t fc_pkt_count = 0;
    //UHD_MSG(status) << "sending flow ctrl packet " << fc_pkt_count++ << ", acking " << str(boost::format("%04d\tseq32==0x%08x") % last_seq % seq32) << std::endl;

    //load packet info
    vrt::if_packet_info_t packet_info;
    packet_info.packet_type = vrt::if_packet_info_t::PACKET_TYPE_FC;
    packet_info.num_payload_words32 = 2;
    packet_info.num_payload_bytes = packet_info.num_payload_words32*sizeof(boost::uint32_t);
    packet_info.packet_count = seq32;
    packet_info.sob = false;
    packet_info.eob = false;
    packet_info.sid = sid.get();
    packet_info.has_sid = true;
    packet_info.has_cid = false;
    packet_info.has_tsi = false;
    packet_info.has_tsf = false;
    packet_info.has_tlr = false;

    //load header
    if (big_endian)
        if_hdr_pack_be(pkt, packet_info);
    else
        if_hdr_pack_le(pkt, packet_info);

    //load payload
    pkt[packet_info.num_header_words32+0] = uhd::htonx<boost::uint32_t>(0);
    pkt[packet_info.num_header_words32+1] = uhd::htonx<boost::uint32_t>(seq32);

    // hardcode bits TODO remove this when chdr fix is merged
    pkt[0] = (pkt[0] & 0xFFFFFF00) | 0x00000040;

    //std::cout << "  SID=" << std::hex << sid << " hdr bits=" << packet_info.packet_type << " seq32=" << seq32 << std::endl;
    //std::cout << "num_packet_words32: " << packet_info.num_packet_words32 << std::endl;
    //for (size_t i = 0; i < packet_info.num_packet_words32; i++) {
        //std::cout << str(boost::format("0x%08x") % pkt[i]) << " ";
        //if (i % 2) {
            //std::cout << std::endl;
        //}
    //}

    //send the buffer over the interface
    buff->commit(sizeof(boost::uint32_t)*(packet_info.num_packet_words32));
}

/***********************************************************************
 * TX Flow Control Functions
 **********************************************************************/
size_t device3_impl::get_tx_flow_control_window(
        size_t pkt_size,
        const double hw_buff_size_,
        const device_addr_t& tx_args
) {
    double hw_buff_size = tx_args.cast<double>("send_buff_size", hw_buff_size_);
    size_t window_in_pkts = (static_cast<size_t>(hw_buff_size) / pkt_size);
    if (window_in_pkts == 0) {
        throw uhd::value_error("send_buff_size must be larger than the send_pkt_size.");
    }
    return window_in_pkts;
}

managed_send_buffer::sptr device3_impl::get_tx_buff_with_flowctrl(
    task::sptr /*holds ref*/,
    boost::shared_ptr<device3_impl::tx_fc_cache_t> guts,
    zero_copy_if::sptr xport,
    size_t fc_pkt_window,
    const double timeout
){
    while (true)
    {
        const size_t delta = (guts->last_seq_out & 0xfff) - (guts->last_seq_ack & 0xfff);
        if ((delta & 0xfff) <= fc_pkt_window) break;

        const bool ok = guts->seq_queue.pop_with_timed_wait(guts->last_seq_ack, timeout);
        if (not ok) return managed_send_buffer::sptr(); //timeout waiting for flow control
    }

    managed_send_buffer::sptr buff = xport->get_send_buff(timeout);
    if (buff) {
        guts->last_seq_out++; //update seq, this will actually be a send
    }
    return buff;
}


/***********************************************************************
 * Async Data
 **********************************************************************/
bool device3_impl::recv_async_msg(
    async_metadata_t &async_metadata, double timeout
)
{
    return _async_md->pop_with_timed_wait(async_metadata, timeout);
}

/***********************************************************************
 * CHDR packer/unpacker (TODO: Remove once new chdr class is in)
 **********************************************************************/
void device3_impl::if_hdr_unpack_be(
        const boost::uint32_t *packet_buff,
        vrt::if_packet_info_t &if_packet_info
){
    if_packet_info.link_type = vrt::if_packet_info_t::LINK_TYPE_CHDR;
    return vrt::if_hdr_unpack_be(packet_buff, if_packet_info);
}

void device3_impl::if_hdr_unpack_le(
    const boost::uint32_t *packet_buff,
    vrt::if_packet_info_t &if_packet_info
){
    if_packet_info.link_type = vrt::if_packet_info_t::LINK_TYPE_CHDR;
    return vrt::if_hdr_unpack_le(packet_buff, if_packet_info);
}

void device3_impl::if_hdr_pack_be(
    boost::uint32_t *packet_buff,
    vrt::if_packet_info_t &if_packet_info
){
    if_packet_info.link_type = vrt::if_packet_info_t::LINK_TYPE_CHDR;
    return vrt::if_hdr_pack_be(packet_buff, if_packet_info);
}

void device3_impl::if_hdr_pack_le(
    boost::uint32_t *packet_buff,
    vrt::if_packet_info_t &if_packet_info
){
    if_packet_info.link_type = vrt::if_packet_info_t::LINK_TYPE_CHDR;
    return vrt::if_hdr_pack_le(packet_buff, if_packet_info);
}

