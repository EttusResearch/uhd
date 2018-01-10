//
// Copyright 2014-2016 Ettus Research LLC
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
#include <uhd/rfnoc/constants.hpp>
#include <uhd/rfnoc/source_block_ctrl_base.hpp>
#include <uhd/rfnoc/sink_block_ctrl_base.hpp>
#include <uhd/utils/byteswap.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/msg.hpp>
#include "../common/async_packet_handler.hpp"
#include "../../transport/super_recv_packet_handler.hpp"
#include "../../transport/super_send_packet_handler.hpp"
#include "../../rfnoc/rx_stream_terminator.hpp"
#include "../../rfnoc/tx_stream_terminator.hpp"
#include <uhd/rfnoc/rate_node_ctrl.hpp>
#include <uhd/rfnoc/radio_ctrl.hpp>
#include <uhd/transport/zero_copy_flow_ctrl.hpp>
#include <boost/atomic.hpp>

#define UHD_STREAMER_LOG() UHD_LOGV(never)

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;

//! CHDR uses 12-Bit sequence numbers
static const uint32_t HW_SEQ_NUM_MASK = 0xfff;


/***********************************************************************
 * Helper functions for get_?x_stream()
 **********************************************************************/
static uhd::stream_args_t sanitize_stream_args(const uhd::stream_args_t &args_)
{
    uhd::stream_args_t args = args_;
    if (args.channels.empty()) {
        args.channels = std::vector<size_t>(1, 0);
    }

    return args;
}

static void check_stream_sig_compatible(const rfnoc::stream_sig_t &stream_sig, stream_args_t &args, const std::string &tx_rx)
{
    if (args.otw_format.empty()) {
        if (stream_sig.item_type.empty()) {
            throw uhd::runtime_error(str(
                    boost::format("[%s Streamer] No otw_format defined!") % tx_rx
            ));
        } else {
            args.otw_format = stream_sig.item_type;
        }
    } else if (not stream_sig.item_type.empty() and stream_sig.item_type != args.otw_format) {
        throw uhd::runtime_error(str(
                boost::format("[%s Streamer] Conflicting OTW types defined: args.otw_format = '%s' <=> stream_sig.item_type = '%s'")
                % tx_rx % args.otw_format % stream_sig.item_type
        ));
    }
    const size_t bpi = convert::get_bytes_per_item(args.otw_format); // bytes per item
    if (stream_sig.packet_size) {
        if (args.args.has_key("spp")) {
            size_t args_spp = args.args.cast<size_t>("spp", 0);
            if (args_spp * bpi != stream_sig.packet_size) {
                throw uhd::runtime_error(str(
                        boost::format("[%s Streamer] Conflicting packet sizes defined: args yields %d bytes but stream_sig.packet_size is %d bytes")
                        % tx_rx % (args_spp * bpi) % stream_sig.packet_size
                ));
            }
        } else {
            args.args["spp"] = str(boost::format("%d") % (stream_sig.packet_size / bpi));
        }
    }
}

/*! \brief Returns a list of rx or tx channels for a streamer.
 *
 * If the given stream args contain instructions to set up channels,
 * those are used. Otherwise, the current device's channel definition
 * is consulted.
 *
 * \param args_ Stream args.
 * \param[out] chan_list The list of channels in the correct order.
 * \param[out] chan_args Channel args for every channel. `chan_args.size() == chan_list.size()`
 */
void generate_channel_list(
        const uhd::stream_args_t &args_,
        std::vector<uhd::rfnoc::block_id_t> &chan_list,
        std::vector<device_addr_t> &chan_args
) {
    uhd::stream_args_t args = args_;
    std::vector<uhd::rfnoc::block_id_t> chan_list_(args.channels.size());
    std::vector<device_addr_t> chan_args_(args.channels.size());

    for (size_t i = 0; i < args.channels.size(); i++)
    {
        // Extract block ID
        size_t chan_idx = args.channels[i];
        std::string key = str(boost::format("block_id%d") % chan_idx);
        if (args.args.has_key(key)) {
            chan_list_[i] = args.args.pop(key);
        } else if (args.args.has_key("block_id")) {
            chan_list_[i] = args.args["block_id"];
        } else {
            throw uhd::runtime_error(str(
                boost::format("Cannot create streamers: No block_id specified for channel %d.")
                % chan_idx
            ));
        }

        // Split off known channel specific args
        key = str(boost::format("block_port%d") % chan_idx);
        if (args.args.has_key(key)) {
            chan_args_[i]["block_port"] = args.args.pop(key);
        }
        key = str(boost::format("radio_id%d") % chan_idx);
        if (args.args.has_key(key)) {
            chan_args_[i]["radio_id"] = args.args.pop(key);
        }
        key = str(boost::format("radio_port%d") % chan_idx);
        if (args.args.has_key(key)) {
            chan_args_[i]["radio_port"] = args.args.pop(key);
        }
    }

    // Add all remaining args to all channel args
    BOOST_FOREACH(device_addr_t &chan_arg, chan_args_) {
        chan_arg = chan_arg.to_string() + "," + args.args.to_string();
    }

    chan_list = chan_list_;
    chan_args = chan_args_;
}


/***********************************************************************
 * RX Flow Control Functions
 **********************************************************************/
//! Stores the state of RX flow control
struct rx_fc_cache_t
{
    rx_fc_cache_t():
        last_seq_in(0){}
    size_t last_seq_in;
};

/*! Determine the size of the flow control window in number of packets.
 *
 * This value depends on three things:
 * - The packet size (in bytes), P
 * - The size of the software buffer (in bytes), B
 * - The desired buffer fullness, F
 *
 * The FC window size is thus X = floor(B*F/P).
 *
 * \param pkt_size The maximum packet size in bytes
 * \param sw_buff_size Software buffer size in bytes
 * \param rx_args If this has a key 'recv_buff_fullness', this value will
 *                be used for said fullness. Must be between 0.01 and 1.
 *
 *  \returns The size of the flow control window in number of packets
 */
static size_t get_rx_flow_control_window(
        size_t pkt_size,
        size_t sw_buff_size,
        const device_addr_t& rx_args
) {
    double fullness_factor = rx_args.cast<double>(
            "recv_buff_fullness",
            uhd::rfnoc::DEFAULT_FC_RX_SW_BUFF_FULL_FACTOR
    );

    if (fullness_factor < 0.01 || fullness_factor > 1) {
        throw uhd::value_error("recv_buff_fullness must be in [0.01, 1] inclusive (1% to 100%)");
    }

    size_t window_in_pkts = (static_cast<size_t>(sw_buff_size * fullness_factor) / pkt_size);
    if (rx_args.has_key("max_recv_window")) {
        window_in_pkts = std::min(
            window_in_pkts,
            rx_args.cast<size_t>("max_recv_window", window_in_pkts)
        );
    }
    if (window_in_pkts == 0) {
        throw uhd::value_error("recv_buff_size must be larger than the recv_frame_size.");
    }
    UHD_ASSERT_THROW(size_t(sw_buff_size * fullness_factor) >= pkt_size * window_in_pkts);
    return window_in_pkts;
}


/*! Send out RX flow control packets.
 *
 * For an rx stream, this function takes care of sending back
 * a flow control packet to the source telling it which
 * packets have been consumed.
 *
 * This function should only be called by the function handling
 * the rx stream, usually recv() in super_recv_packet_handler.
 *
 * \param sid The SID that goes into this packet. This is the reversed()
 *            version of the data stream's SID.
 * \param xport A transport object over which to send the data
 * \param big_endian Endianness of the transport
 * \param seq32_state Pointer to a variable that saves the 32-Bit state
 *                    of the sequence numbers, since we only have 12 Bit
 *                    sequence numbers in CHDR.
 * \param last_seq The value to send: The last consumed packet's sequence number.
 */
static void handle_rx_flowctrl(
        const sid_t &sid,
        zero_copy_if::sptr xport,
        endianness_t endianness,
        boost::shared_ptr<rx_fc_cache_t> fc_cache,
        const size_t last_seq
) {
    static const size_t RXFC_PACKET_LEN_IN_WORDS    = 2;
    static const size_t RXFC_CMD_CODE_OFFSET        = 0;
    static const size_t RXFC_SEQ_NUM_OFFSET         = 1;

    managed_send_buffer::sptr buff = xport->get_send_buff(0.0);
    if (not buff) {
        throw uhd::runtime_error("handle_rx_flowctrl timed out getting a send buffer");
    }
    uint32_t *pkt = buff->cast<uint32_t *>();

    // Recover sequence number. The sequence numbers handled by the streamers
    // are 12 Bits, but we want to know the 32-Bit sequence number.
    size_t &seq32 = fc_cache->last_seq_in;
    const size_t seq12 = seq32 & HW_SEQ_NUM_MASK;
    if (last_seq < seq12)
        seq32 += (HW_SEQ_NUM_MASK + 1);
    seq32 &= ~HW_SEQ_NUM_MASK;
    seq32 |= last_seq;

    // Super-verbose mode:
    //static size_t fc_pkt_count = 0;
    //UHD_MSG(status) << "sending flow ctrl packet " << fc_pkt_count++ << ", acking " << str(boost::format("%04d\tseq_sw==0x%08x") % last_seq % seq32) << std::endl;

    //load packet info
    vrt::if_packet_info_t packet_info;
    packet_info.packet_type = vrt::if_packet_info_t::PACKET_TYPE_FC;
    packet_info.num_payload_words32 = RXFC_PACKET_LEN_IN_WORDS;
    packet_info.num_payload_bytes = packet_info.num_payload_words32*sizeof(uint32_t);
    packet_info.packet_count = seq32;
    packet_info.sob = false;
    packet_info.eob = false;
    packet_info.sid = sid.get();
    packet_info.has_sid = true;
    packet_info.has_cid = false;
    packet_info.has_tsi = false;
    packet_info.has_tsf = false;
    packet_info.has_tlr = false;

    if (endianness == ENDIANNESS_BIG) {
        // Load Header:
        vrt::chdr::if_hdr_pack_be(pkt, packet_info);
        // Load Payload: (the sequence number)
        pkt[packet_info.num_header_words32+RXFC_CMD_CODE_OFFSET] = uhd::htonx<uint32_t>(0);
        pkt[packet_info.num_header_words32+RXFC_SEQ_NUM_OFFSET]  = uhd::htonx<uint32_t>(seq32);
    } else {
        // Load Header:
        vrt::chdr::if_hdr_pack_le(pkt, packet_info);
        // Load Payload: (the sequence number)
        pkt[packet_info.num_header_words32+RXFC_CMD_CODE_OFFSET] = uhd::htowx<uint32_t>(0);
        pkt[packet_info.num_header_words32+RXFC_SEQ_NUM_OFFSET]  = uhd::htowx<uint32_t>(seq32);
    }

    //std::cout << "  SID=" << std::hex << sid << " hdr bits=" << packet_info.packet_type << " seq32=" << seq32 << std::endl;
    //std::cout << "num_packet_words32: " << packet_info.num_packet_words32 << std::endl;
    //for (size_t i = 0; i < packet_info.num_packet_words32; i++) {
        //std::cout << str(boost::format("0x%08x") % pkt[i]) << " ";
        //if (i % 2) {
            //std::cout << std::endl;
        //}
    //}

    //send the buffer over the interface
    buff->commit(sizeof(uint32_t)*(packet_info.num_packet_words32));
}

/***********************************************************************
 * TX Flow Control Functions
 **********************************************************************/
#define DEVICE3_ASYNC_EVENT_CODE_FLOW_CTRL 0

//! Stores the state of TX flow control
struct tx_fc_cache_t
{
    tx_fc_cache_t(size_t capacity):
        last_seq_ack(0),
        space(capacity) {}

    size_t last_seq_ack;
    size_t space;
};

/*! Return the size of the flow control window in packets.
 *
 * If the return value of this function is F, the last tx'd packet
 * has index N and the last ack'd packet has index M, the amount of
 * FC credit we have is C = F + M - N (i.e. we can send C more packets
 * before getting another ack).
 *
 * Note: If `send_buff_size` is set in \p tx_hints, this will
 * override hw_buff_size_.
 */
static size_t get_tx_flow_control_window(
        size_t pkt_size,
        const double hw_buff_size_,
        const device_addr_t& tx_hints
) {
    double hw_buff_size = tx_hints.cast<double>("send_buff_size", hw_buff_size_);
    size_t window_in_pkts = (static_cast<size_t>(hw_buff_size) / pkt_size);
    if (window_in_pkts == 0) {
        throw uhd::value_error("send_buff_size must be larger than the send_frame_size.");
    }
    return window_in_pkts;
}

static bool tx_flow_ctrl(
    boost::shared_ptr<tx_fc_cache_t> fc_cache,
	zero_copy_if::sptr async_xport,
    uint32_t (*endian_conv)(uint32_t),
    void (*unpack)(const uint32_t *packet_buff, vrt::if_packet_info_t &),
    managed_buffer::sptr
) {
    while (true)
    {
        // If there is space
        if (fc_cache->space)
        {
            // All is good - packet will be sent
            fc_cache->space--;
            return true;
        }

		// Look for a flow control message to update the space available in the buffer.
		managed_recv_buffer::sptr buff = async_xport->get_recv_buff();
		if (buff)
		{
			vrt::if_packet_info_t if_packet_info;
			if_packet_info.num_packet_words32 = buff->size()/sizeof(uint32_t);
			const uint32_t *packet_buff = buff->cast<const uint32_t *>();
			try {
				unpack(packet_buff, if_packet_info);
			}
			catch(const std::exception &ex)
			{
				UHD_MSG(error) << "Error unpacking async flow control packet: " << ex.what() << std::endl;
				continue;
			}

			if (if_packet_info.packet_type != vrt::if_packet_info_t::PACKET_TYPE_FC)
			{
				UHD_MSG(error) << "Unexpected packet type received by flow control handler: " << if_packet_info.packet_type << std::endl;
				continue;
			}

			// update the amount of space
			size_t seq_ack = endian_conv(packet_buff[if_packet_info.num_header_words32+1]);
			fc_cache->space += (seq_ack - fc_cache->last_seq_ack) & HW_SEQ_NUM_MASK;
			fc_cache->last_seq_ack = seq_ack;
		}
    }
    return false;
}

/***********************************************************************
 * TX Async Message Functions
 **********************************************************************/
struct async_tx_info_t
{
    size_t stream_channel;
    size_t device_channel;
    boost::shared_ptr<device3_impl::async_md_type> async_queue;
    boost::shared_ptr<device3_impl::async_md_type> old_async_queue;
};

/*! Handle incoming messages.
 *  Send them to the async message queue for the user to poll.
 *
 * This is run inside a uhd::task as long as this streamer lives.
 */
static void handle_tx_async_msgs(
        boost::shared_ptr<async_tx_info_t> async_info,
        zero_copy_if::sptr xport,
        endianness_t endianness,
        boost::function<double(void)> get_tick_rate
) {
    managed_recv_buffer::sptr buff = xport->get_recv_buff();
    if (not buff)
    {
        return;
    }

    //extract packet info
    vrt::if_packet_info_t if_packet_info;
    if_packet_info.num_packet_words32 = buff->size()/sizeof(uint32_t);
    const uint32_t *packet_buff = buff->cast<const uint32_t *>();

    //unpacking can fail
    uint32_t (*endian_conv)(uint32_t) = uhd::ntohx;
    try
    {
        if (endianness == ENDIANNESS_BIG)
        {
            vrt::chdr::if_hdr_unpack_be(packet_buff, if_packet_info);
            endian_conv = uhd::ntohx;
        }
        else
        {
            vrt::chdr::if_hdr_unpack_le(packet_buff, if_packet_info);
            endian_conv = uhd::wtohx;
        }
    }
    catch(const std::exception &ex)
    {
        UHD_MSG(error) << "Error parsing async message packet: " << ex.what() << std::endl;
        return;
    }

    double tick_rate = get_tick_rate();
    if (tick_rate == rfnoc::tick_node_ctrl::RATE_UNDEFINED) {
        tick_rate = 1;
    }

    //fill in the async metadata
    async_metadata_t metadata;
    load_metadata_from_buff(
            endian_conv,
            metadata,
            if_packet_info,
            packet_buff,
            tick_rate,
            async_info->stream_channel
    );

	// Filter out any flow control messages and cache the rest
    if (metadata.event_code == DEVICE3_ASYNC_EVENT_CODE_FLOW_CTRL)
    {
        UHD_MSG(error) << "Unexpected flow control message found in async message handling" << std::endl;
    } else {
        async_info->async_queue->push_with_pop_on_full(metadata);
        metadata.channel = async_info->device_channel;
        async_info->old_async_queue->push_with_pop_on_full(metadata);
        standard_async_msg_prints(metadata);
    }
}

bool device3_impl::recv_async_msg(
    async_metadata_t &async_metadata, double timeout
)
{
    return _async_md->pop_with_timed_wait(async_metadata, timeout);
}

/***********************************************************************
 * Receive streamer
 **********************************************************************/
void device3_impl::update_rx_streamers(double /* rate */)
{
    BOOST_FOREACH(const std::string &block_id, _rx_streamers.keys()) {
        UHD_STREAMER_LOG() << "[Device3] updating RX streamer to " << block_id << std::endl;
        boost::shared_ptr<sph::recv_packet_streamer> my_streamer =
            boost::dynamic_pointer_cast<sph::recv_packet_streamer>(_rx_streamers[block_id].lock());
        if (my_streamer) {
            double tick_rate = my_streamer->get_terminator()->get_tick_rate();
            if (tick_rate == rfnoc::tick_node_ctrl::RATE_UNDEFINED) {
                tick_rate = 1.0;
            }
            my_streamer->set_tick_rate(tick_rate);
            double samp_rate = my_streamer->get_terminator()->get_output_samp_rate();
            if (samp_rate == rfnoc::rate_node_ctrl::RATE_UNDEFINED) {
                samp_rate = 1.0;
            }
            double scaling = my_streamer->get_terminator()->get_output_scale_factor();
            if (scaling == rfnoc::scalar_node_ctrl::SCALE_UNDEFINED) {
                scaling = 1/32767.;
            }
            UHD_STREAMER_LOG() << "  New tick_rate == " << tick_rate << "  New samp_rate == " << samp_rate << " New scaling == " << scaling << std::endl;

            my_streamer->set_tick_rate(tick_rate);
            my_streamer->set_samp_rate(samp_rate);
            my_streamer->set_scale_factor(scaling);
        }
    }
}

rx_streamer::sptr device3_impl::get_rx_stream(const stream_args_t &args_)
{
    boost::mutex::scoped_lock lock(_transport_setup_mutex);
    stream_args_t args = sanitize_stream_args(args_);

    // I. Generate the channel list
    std::vector<uhd::rfnoc::block_id_t> chan_list;
    std::vector<device_addr_t> chan_args;
    generate_channel_list(args, chan_list, chan_args);
    // Note: All 'args.args' are merged into chan_args now.

    // II. Iterate over all channels
    boost::shared_ptr<sph::recv_packet_streamer> my_streamer;
    // The terminator's lifetime is coupled to the streamer.
    // There is only one terminator. If the streamer has multiple channels,
    // it will be connected to each upstream block.
    rfnoc::rx_stream_terminator::sptr recv_terminator = rfnoc::rx_stream_terminator::make();
    for (size_t stream_i = 0; stream_i < chan_list.size(); stream_i++) {
        // Get block ID and mb index
        uhd::rfnoc::block_id_t block_id = chan_list[stream_i];
        UHD_STREAMER_LOG() << "[RX Streamer] chan " << stream_i << " connecting to " << block_id << std::endl;
        // Update args so args.args is always valid for this particular channel:
        args.args = chan_args[stream_i];
        size_t mb_index = block_id.get_device_no();
        size_t suggested_block_port = args.args.cast<size_t>("block_port", rfnoc::ANY_PORT);

        // Access to this channel's block control
        uhd::rfnoc::source_block_ctrl_base::sptr blk_ctrl =
            boost::dynamic_pointer_cast<uhd::rfnoc::source_block_ctrl_base>(get_block_ctrl(block_id));

        // Connect the terminator with this channel's block.
        size_t block_port = blk_ctrl->connect_downstream(
                recv_terminator,
                suggested_block_port,
                args.args
        );
        const size_t terminator_port = recv_terminator->connect_upstream(blk_ctrl);
        blk_ctrl->set_downstream_port(block_port, terminator_port);
        recv_terminator->set_upstream_port(terminator_port, block_port);

        // Check if the block connection is compatible (spp and item type)
        check_stream_sig_compatible(blk_ctrl->get_output_signature(block_port), args, "RX");

        // Setup the DSP transport hints
        device_addr_t rx_hints = get_rx_hints(mb_index);

        //allocate sid and create transport
        uhd::sid_t stream_address = blk_ctrl->get_address(block_port);
        UHD_STREAMER_LOG() << "[RX Streamer] creating rx stream " << rx_hints.to_string() << std::endl;
        both_xports_t xport = make_transport(stream_address, RX_DATA, rx_hints);
        UHD_STREAMER_LOG() << std::hex << "[RX Streamer] data_sid = " << xport.send_sid << std::dec << " actual recv_buff_size = " << xport.recv_buff_size << std::endl;

        // Configure the block
        blk_ctrl->set_destination(xport.send_sid.get_src(), block_port);

        blk_ctrl->sr_write(uhd::rfnoc::SR_RESP_OUT_DST_SID, xport.send_sid.get_src(), block_port);
        UHD_STREAMER_LOG() << "[RX Streamer] resp_out_dst_sid == " << xport.send_sid.get_src() << std::endl;

        // Find all upstream radio nodes and set their response in SID to the host
        std::vector<boost::shared_ptr<uhd::rfnoc::radio_ctrl> > upstream_radio_nodes = blk_ctrl->find_upstream_node<uhd::rfnoc::radio_ctrl>();
        UHD_STREAMER_LOG() << "[RX Streamer] Number of upstream radio nodes: " << upstream_radio_nodes.size() << std::endl;
        BOOST_FOREACH(const boost::shared_ptr<uhd::rfnoc::radio_ctrl> &node, upstream_radio_nodes) {
            node->sr_write(uhd::rfnoc::SR_RESP_OUT_DST_SID, xport.send_sid.get_src(), block_port);
        }

        // To calculate the max number of samples per packet, we assume the maximum header length
        // to avoid fragmentation should the entire header be used.
        const size_t bpp = xport.recv->get_recv_frame_size() - stream_options.rx_max_len_hdr; // bytes per packet
        const size_t bpi = convert::get_bytes_per_item(args.otw_format); // bytes per item
        const size_t spp = std::min(args.args.cast<size_t>("spp", bpp/bpi), bpp/bpi); // samples per packet
        UHD_STREAMER_LOG() << "[RX Streamer] spp == " << spp << std::endl;

        //make the new streamer given the samples per packet
        if (not my_streamer)
            my_streamer = boost::make_shared<sph::recv_packet_streamer>(spp);
        my_streamer->resize(chan_list.size());

        //init some streamer stuff
        std::string conv_endianness;
        if (get_transport_endianness(mb_index) == ENDIANNESS_BIG) {
            my_streamer->set_vrt_unpacker(&vrt::chdr::if_hdr_unpack_be);
            conv_endianness = "be";
        } else {
            my_streamer->set_vrt_unpacker(&vrt::chdr::if_hdr_unpack_le);
            conv_endianness = "le";
        }

        //set the converter
        uhd::convert::id_type id;
        id.input_format = args.otw_format + "_item32_" + conv_endianness;
        id.num_inputs = 1;
        id.output_format = args.cpu_format;
        id.num_outputs = 1;
        my_streamer->set_converter(id);

        //flow control setup
        const size_t pkt_size = spp * bpi + stream_options.rx_max_len_hdr;
        const size_t fc_window = get_rx_flow_control_window(pkt_size, xport.recv_buff_size, rx_hints);
        const size_t fc_handle_window = std::max<size_t>(1, fc_window / stream_options.rx_fc_request_freq);
        UHD_STREAMER_LOG()<< "[RX Streamer] Flow Control Window (minus one) = " << fc_window-1 << ", Flow Control Handler Window = " << fc_handle_window << std::endl;
        blk_ctrl->configure_flow_control_out(
                fc_window-1, // Leave one space for overrun packets TODO make this obsolete
                block_port
        );

        //Give the streamer a functor to get the recv_buffer
        //bind requires a zero_copy_if::sptr to add a streamer->xport lifetime dependency
        my_streamer->set_xport_chan_get_buff(
            stream_i,
            boost::bind(&zero_copy_if::get_recv_buff, xport.recv, _1),
            true /*flush*/
        );

        //Give the streamer a functor to handle overruns
        //bind requires a weak_ptr to break the a streamer->streamer circular dependency
        //Using "this" is OK because we know that this device3_impl will outlive the streamer
        my_streamer->set_overflow_handler(
              stream_i,
              boost::bind(
                  &uhd::rfnoc::rx_stream_terminator::handle_overrun, recv_terminator,
                  boost::weak_ptr<uhd::rx_streamer>(my_streamer), stream_i
              )
        );

        //Give the streamer a functor to send flow control messages
        //handle_rx_flowctrl is static and has no lifetime issues
        boost::shared_ptr<rx_fc_cache_t> fc_cache(new rx_fc_cache_t());
        my_streamer->set_xport_handle_flowctrl(
            stream_i, boost::bind(
                &handle_rx_flowctrl,
                xport.send_sid,
                xport.send,
                get_transport_endianness(mb_index),
                fc_cache,
                _1
            ),
            fc_handle_window,
            true/*init*/
        );

        //Give the streamer a functor issue stream cmd
        //bind requires a shared pointer to add a streamer->framer lifetime dependency
        my_streamer->set_issue_stream_cmd(
            stream_i,
            boost::bind(&uhd::rfnoc::source_block_ctrl_base::issue_stream_cmd, blk_ctrl, _1, block_port)
        );

        // Tell the streamer which SID is valid for this channel
        my_streamer->set_xport_chan_sid(stream_i, true, xport.send_sid);
    }

    // Connect the terminator to the streamer
    my_streamer->set_terminator(recv_terminator);

    // Notify all blocks in this chain that they are connected to an active streamer
    recv_terminator->set_rx_streamer(true, 0);

    // Store a weak pointer to prevent a streamer->device3_impl->streamer circular dependency.
    // Note that we store the streamer only once, and use its terminator's
    // ID to do so.
    _rx_streamers[recv_terminator->unique_id()] = boost::weak_ptr<sph::recv_packet_streamer>(my_streamer);

    // Sets tick rate, samp rate and scaling on this streamer.
    // A registered terminator is required to do this.
    update_rx_streamers();

    post_streamer_hooks(RX_DIRECTION);
    return my_streamer;
}

/***********************************************************************
 * Transmit streamer
 **********************************************************************/
void device3_impl::update_tx_streamers(double /* rate */)
{
    BOOST_FOREACH(const std::string &block_id, _tx_streamers.keys()) {
        UHD_STREAMER_LOG() << "[Device3] updating TX streamer: " << block_id << std::endl;
        boost::shared_ptr<sph::send_packet_streamer> my_streamer =
            boost::dynamic_pointer_cast<sph::send_packet_streamer>(_tx_streamers[block_id].lock());
        if (my_streamer) {
            double tick_rate = my_streamer->get_terminator()->get_tick_rate();
            if (tick_rate == rfnoc::tick_node_ctrl::RATE_UNDEFINED) {
                tick_rate = 1.0;
            }
            double samp_rate = my_streamer->get_terminator()->get_input_samp_rate();
            if (samp_rate == rfnoc::rate_node_ctrl::RATE_UNDEFINED) {
                samp_rate = 1.0;
            }
            double scaling = my_streamer->get_terminator()->get_input_scale_factor();
            if (scaling == rfnoc::scalar_node_ctrl::SCALE_UNDEFINED) {
                scaling = 32767.;
            }
            UHD_STREAMER_LOG() << "  New tick_rate == " << tick_rate << "  New samp_rate == " << samp_rate << " New scaling == " << scaling << std::endl;
            my_streamer->set_tick_rate(tick_rate);
            my_streamer->set_samp_rate(samp_rate);
            my_streamer->set_scale_factor(scaling);
        }
    }
}

// This class manages the lifetime of the TX async message handler task and transports
class device3_send_packet_streamer : public sph::send_packet_streamer
{
public:
	device3_send_packet_streamer(const size_t max_num_samps) : sph::send_packet_streamer(max_num_samps) {};
	~device3_send_packet_streamer() {
		_tx_async_msg_task.reset();	// Make sure the async task is destroyed before the transports
	};

	both_xports_t _xport;
	both_xports_t _async_xport;
	task::sptr _tx_async_msg_task;
};

tx_streamer::sptr device3_impl::get_tx_stream(const uhd::stream_args_t &args_)
{
    boost::mutex::scoped_lock lock(_transport_setup_mutex);
    stream_args_t args = sanitize_stream_args(args_);

    // I. Generate the channel list
    std::vector<uhd::rfnoc::block_id_t> chan_list;
    std::vector<device_addr_t> chan_args;
    generate_channel_list(args, chan_list, chan_args);
    // Note: All 'args.args' are merged into chan_args now.

    //shared async queue for all channels in streamer
    boost::shared_ptr<async_md_type> async_md(new async_md_type(1000/*messages deep*/));

    // II. Iterate over all channels
    boost::shared_ptr<device3_send_packet_streamer> my_streamer;
    // The terminator's lifetime is coupled to the streamer.
    // There is only one terminator. If the streamer has multiple channels,
    // it will be connected to each downstream block.
    rfnoc::tx_stream_terminator::sptr send_terminator = rfnoc::tx_stream_terminator::make();
    for (size_t stream_i = 0; stream_i < chan_list.size(); stream_i++) {
        // Get block ID and mb index
        uhd::rfnoc::block_id_t block_id = chan_list[stream_i];
        // Update args so args.args is always valid for this particular channel:
        args.args = chan_args[stream_i];
        size_t mb_index = block_id.get_device_no();
        size_t suggested_block_port = args.args.cast<size_t>("block_port", rfnoc::ANY_PORT);
        uhd::endianness_t endianness = get_transport_endianness(mb_index);

        // Access to this channel's block control
        uhd::rfnoc::sink_block_ctrl_base::sptr blk_ctrl =
            boost::dynamic_pointer_cast<uhd::rfnoc::sink_block_ctrl_base>(get_block_ctrl(block_id));

        // Connect the terminator with this channel's block.
        // This will throw if the connection is not possible.
        size_t block_port = blk_ctrl->connect_upstream(
                send_terminator,
                suggested_block_port,
                args.args
        );
        const size_t terminator_port = send_terminator->connect_downstream(blk_ctrl);
        blk_ctrl->set_upstream_port(block_port, terminator_port);
        send_terminator->set_downstream_port(terminator_port, block_port);

        // Check if the block connection is compatible (spp and item type)
        check_stream_sig_compatible(blk_ctrl->get_input_signature(block_port), args, "TX");

        // Setup the dsp transport hints
        device_addr_t tx_hints = get_tx_hints(mb_index);

        //allocate sid and create transport
        uhd::sid_t stream_address = blk_ctrl->get_address(block_port);
        UHD_STREAMER_LOG() << "[TX Streamer] creating tx stream " << tx_hints.to_string() << std::endl;
        both_xports_t xport = make_transport(stream_address, TX_DATA, tx_hints);
		both_xports_t async_xport = make_transport(stream_address, ASYNC_MSG, device_addr_t(""));
        UHD_STREAMER_LOG() << std::hex << "[TX Streamer] data_sid = " << xport.send_sid << std::dec << std::endl;

        // To calculate the max number of samples per packet, we assume the maximum header length
        // to avoid fragmentation should the entire header be used.
        const size_t bpp = tx_hints.cast<size_t>("bpp", xport.send->get_send_frame_size()) - stream_options.tx_max_len_hdr;
        const size_t bpi = convert::get_bytes_per_item(args.otw_format); // bytes per item
        const size_t spp = std::min(args.args.cast<size_t>("spp", bpp/bpi), bpp/bpi); // samples per packet
        UHD_STREAMER_LOG() << "[TX Streamer] spp == " << spp << std::endl;

        //make the new streamer given the samples per packet
        if (not my_streamer)
            my_streamer = boost::make_shared<device3_send_packet_streamer>(spp);
        my_streamer->resize(chan_list.size());
        my_streamer->_xport = xport;
        my_streamer->_async_xport = async_xport;

        //init some streamer stuff
        std::string conv_endianness;
        if (get_transport_endianness(mb_index) == ENDIANNESS_BIG) {
            my_streamer->set_vrt_packer(&vrt::chdr::if_hdr_pack_be);
            conv_endianness = "be";
        } else {
            my_streamer->set_vrt_packer(&vrt::chdr::if_hdr_pack_le);
            conv_endianness = "le";
        }

        //set the converter
        uhd::convert::id_type id;
        id.input_format = args.cpu_format;
        id.num_inputs = 1;
        id.output_format = args.otw_format + "_item32_" + conv_endianness;
        id.num_outputs = 1;
        my_streamer->set_converter(id);

        //flow control setup
        const size_t pkt_size = spp * bpi + stream_options.tx_max_len_hdr;
        // For flow control, this value is used to determine the window size in *packets*
        size_t fc_window = get_tx_flow_control_window(
                pkt_size, // This is the maximum packet size
                blk_ctrl->get_fifo_size(block_port),
                tx_hints // This can override the value reported by the block!
        );
        const size_t fc_handle_window = std::max<size_t>(1, fc_window / stream_options.tx_fc_response_freq);
        UHD_STREAMER_LOG() << "[TX Streamer] Flow Control Window = " << fc_window << ", Flow Control Handler Window = " << fc_handle_window << std::endl;
        blk_ctrl->configure_flow_control_in(
                stream_options.tx_fc_response_cycles,
                fc_handle_window, /*pkts*/
                block_port
        );

        boost::shared_ptr<async_tx_info_t> async_tx_info(new async_tx_info_t());
        async_tx_info->stream_channel = args.channels[stream_i];
        async_tx_info->device_channel = mb_index;
        async_tx_info->async_queue = async_md;
        async_tx_info->old_async_queue = _async_md;

        boost::function<double(void)> tick_rate_retriever = boost::bind(
                &rfnoc::tick_node_ctrl::get_tick_rate,
                send_terminator,
                std::set< rfnoc::node_ctrl_base::sptr >() // Need to specify default args with bind
        );

        my_streamer->_tx_async_msg_task = task::make(
                boost::bind(
                    &handle_tx_async_msgs,
                    async_tx_info,
                    my_streamer->_async_xport.recv,
                    endianness,
                    tick_rate_retriever
                )
        );

        blk_ctrl->sr_write(uhd::rfnoc::SR_CLEAR_RX_FC, 0xc1ea12, block_port);
        blk_ctrl->sr_write(uhd::rfnoc::SR_RESP_IN_DST_SID, my_streamer->_async_xport.recv_sid.get_dst(), block_port);
        UHD_STREAMER_LOG() << "[TX Streamer] resp_in_dst_sid == " << boost::format("0x%04X") % xport.recv_sid.get_dst() << std::endl;

        // FIXME: Once there is a better way to map the radio block and port
        // to the channel or another way to receive asynchronous messages that
        // is not in-band, this should be removed.
        if (args.args.has_key("radio_id") and args.args.has_key("radio_port"))
        {
            // Find downstream radio node and set the response SID to the host
            uhd::rfnoc::block_id_t radio_id(args.args["radio_id"]);
            size_t radio_port = args.args.cast<size_t>("radio_port", 0);
            std::vector<boost::shared_ptr<uhd::rfnoc::radio_ctrl> > downstream_radio_nodes = blk_ctrl->find_downstream_node<uhd::rfnoc::radio_ctrl>();
            UHD_STREAMER_LOG() << "[TX Streamer] Number of downstream radio nodes: " << downstream_radio_nodes.size() << std::endl;
            BOOST_FOREACH(const boost::shared_ptr<uhd::rfnoc::radio_ctrl> &node, downstream_radio_nodes) {
                if (node->get_block_id() == radio_id) {
                    node->sr_write(uhd::rfnoc::SR_RESP_IN_DST_SID, my_streamer->_async_xport.recv_sid.get_dst(), radio_port);
                }
            }
        } else {
            // FIXME:  This block is preserved for legacy behavior where the
            // radio_id and radio_port are not provided.  It fails if more
            // than one radio is visible downstream or the port on the radio
            // is not the same as the block_port.  It should be removed as
            // soon as possible.
            // Find all downstream radio nodes and set their response SID to the host
            std::vector<boost::shared_ptr<uhd::rfnoc::radio_ctrl> > downstream_radio_nodes = blk_ctrl->find_downstream_node<uhd::rfnoc::radio_ctrl>();
            UHD_STREAMER_LOG() << "[TX Streamer] Number of downstream radio nodes: " << downstream_radio_nodes.size() << std::endl;
            BOOST_FOREACH(const boost::shared_ptr<uhd::rfnoc::radio_ctrl> &node, downstream_radio_nodes) {
                node->sr_write(uhd::rfnoc::SR_RESP_IN_DST_SID, my_streamer->_async_xport.recv_sid.get_dst(), block_port);
            }
        }

        // Add flow control
        boost::shared_ptr<tx_fc_cache_t> fc_cache(new tx_fc_cache_t(fc_window));
        my_streamer->_xport.send = zero_copy_flow_ctrl::make(
            my_streamer->_xport.send,
            boost::bind(
				&tx_flow_ctrl,
				fc_cache,
				my_streamer->_xport.recv,
				(endianness == ENDIANNESS_BIG ? uhd::ntohx<uint32_t> : uhd::wtohx<uint32_t>),
				(endianness == ENDIANNESS_BIG ? vrt::chdr::if_hdr_unpack_be : vrt::chdr::if_hdr_unpack_le),
				 _1),
            NULL);

        //Give the streamer a functor to get the send buffer
        my_streamer->set_xport_chan_get_buff(
            stream_i,
            boost::bind(&zero_copy_if::get_send_buff, my_streamer->_xport.send, _1)
        );
        //Give the streamer a functor handled received async messages
        my_streamer->set_async_receiver(
            boost::bind(&async_md_type::pop_with_timed_wait, async_md, _1, _2)
        );
        my_streamer->set_xport_chan_sid(stream_i, true, xport.send_sid);
        // CHDR does not support trailers
        my_streamer->set_enable_trailer(false);
    }

    // Connect the terminator to the streamer
    my_streamer->set_terminator(send_terminator);

    // Notify all blocks in this chain that they are connected to an active streamer
    send_terminator->set_tx_streamer(true, 0);

    // Store a weak pointer to prevent a streamer->device3_impl->streamer circular dependency.
    // Note that we store the streamer only once, and use its terminator's
    // ID to do so.
    _tx_streamers[send_terminator->unique_id()] = boost::weak_ptr<sph::send_packet_streamer>(my_streamer);

    // Sets tick rate, samp rate and scaling on this streamer
    // A registered terminator is required to do this.
    update_tx_streamers();

    post_streamer_hooks(TX_DIRECTION);
    return my_streamer;
}


