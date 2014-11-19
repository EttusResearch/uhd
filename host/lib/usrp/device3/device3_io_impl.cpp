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

#define DEVICE3_STREAMER // For the super_*_packet_handlers

#include "device3_impl.hpp"
#include <uhd/usrp/rfnoc/constants.hpp>
#include <uhd/usrp/rfnoc/rx_block_ctrl_base.hpp>
#include <uhd/usrp/rfnoc/tx_block_ctrl_base.hpp>
#include <uhd/utils/byteswap.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/msg.hpp>
#include "../common/async_packet_handler.hpp"
#include "../../transport/super_recv_packet_handler.hpp"
#include "../../transport/super_send_packet_handler.hpp"
#include "../rfnoc/terminator_recv.hpp"
#include "../rfnoc/terminator_send.hpp"

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;

//! CVITA uses 12-Bit sequence numbers
static const boost::uint32_t HW_SEQ_NUM_MASK = 0xfff;

/***********************************************************************
 * CHDR packer/unpacker (TODO: Remove once new chdr class is in)
 **********************************************************************/
static void device3_if_hdr_unpack_be(
        const boost::uint32_t *packet_buff,
        vrt::if_packet_info_t &if_packet_info
){
    if_packet_info.link_type = vrt::if_packet_info_t::LINK_TYPE_CHDR;
    return vrt::if_hdr_unpack_be(packet_buff, if_packet_info);
}

static void device3_if_hdr_unpack_le(
    const boost::uint32_t *packet_buff,
    vrt::if_packet_info_t &if_packet_info
){
    if_packet_info.link_type = vrt::if_packet_info_t::LINK_TYPE_CHDR;
    return vrt::if_hdr_unpack_le(packet_buff, if_packet_info);
}

static void device3_if_hdr_pack_be(
    boost::uint32_t *packet_buff,
    vrt::if_packet_info_t &if_packet_info
){
    if_packet_info.link_type = vrt::if_packet_info_t::LINK_TYPE_CHDR;
    return vrt::if_hdr_pack_be(packet_buff, if_packet_info);
}

static void device3_if_hdr_pack_le(
    boost::uint32_t *packet_buff,
    vrt::if_packet_info_t &if_packet_info
){
    if_packet_info.link_type = vrt::if_packet_info_t::LINK_TYPE_CHDR;
    return vrt::if_hdr_pack_le(packet_buff, if_packet_info);
}


/***********************************************************************
 * Helper functions for get_?x_stream()
 **********************************************************************/
static uhd::stream_args_t sanitize_stream_args(const uhd::stream_args_t &args_)
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
 * \param pkt_size Packet size in bytes
 * \param sw_buff_size Software buffer size in bytes
 * \param rx_args If this has a key 'recv_buff_fullness', this value will
 *                be used for said fullness. Must be between 0.01 and 1.
 * \param fullness If specified, this value will override the value in
 *                 \p rx_args.
 */
size_t get_rx_flow_control_window(
        size_t pkt_size,
        size_t sw_buff_size,
        const device_addr_t& rx_args,
        const double fullness_=-1
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
        throw uhd::value_error("recv_buff_size must be larger than the recv_frame_size.");
    }
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
    boost::uint32_t *pkt = buff->cast<boost::uint32_t *>();

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

    if (endianness == ENDIANNESS_BIG) {
        // Load Header:
        device3_if_hdr_pack_be(pkt, packet_info);
        // Load Payload: (the sequence number)
        pkt[packet_info.num_header_words32+RXFC_CMD_CODE_OFFSET] = uhd::htonx<boost::uint32_t>(0);
        pkt[packet_info.num_header_words32+RXFC_SEQ_NUM_OFFSET]  = uhd::htonx<boost::uint32_t>(seq32);
        // hardcode bits TODO remove this when chdr fix is merged
        pkt[0] = (pkt[0] & 0xFFFFFF00) | 0x00000040;
    } else {
        // Load Header:
        device3_if_hdr_pack_le(pkt, packet_info);
        // Load Payload: (the sequence number)
        pkt[packet_info.num_header_words32+RXFC_CMD_CODE_OFFSET] = uhd::htowx<boost::uint32_t>(0);
        pkt[packet_info.num_header_words32+RXFC_SEQ_NUM_OFFSET]  = uhd::htowx<boost::uint32_t>(seq32);
        // hardcode bits TODO remove this when chdr fix is merged
        pkt[0] = (pkt[0] & 0x00FFFFFF) | 0x40000000;
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
    buff->commit(sizeof(boost::uint32_t)*(packet_info.num_packet_words32));
}

/***********************************************************************
 * TX Flow Control Functions
 **********************************************************************/
//! Stores the state of TX flow control
struct tx_fc_cache_t
{
    tx_fc_cache_t(void):
        stream_channel(0),
        device_channel(0),
        last_seq_out(0),
        last_seq_ack(0),
        seq_queue(1){}
    size_t stream_channel;
    size_t device_channel;
    size_t last_seq_out;
    size_t last_seq_ack;
    uhd::transport::bounded_buffer<size_t> seq_queue;
    boost::shared_ptr<device3_impl::async_md_type> async_queue;
    boost::shared_ptr<device3_impl::async_md_type> old_async_queue;
};

/*! Return the size of the flow control window in packets.
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

static managed_send_buffer::sptr get_tx_buff_with_flowctrl(
    task::sptr /*holds ref*/,
    boost::shared_ptr<tx_fc_cache_t> fc_cache,
    zero_copy_if::sptr xport,
    size_t fc_window,
    const double timeout
){
    while (true)
    {
        const size_t delta = (fc_cache->last_seq_out & HW_SEQ_NUM_MASK) - (fc_cache->last_seq_ack & HW_SEQ_NUM_MASK);
        if ((delta & HW_SEQ_NUM_MASK) <= fc_window)
            break;

        if ((delta & 0xfff) <= fc_window)
            break;

        const bool ok = fc_cache->seq_queue.pop_with_timed_wait(fc_cache->last_seq_ack, timeout);
        if (not ok) {
            return managed_send_buffer::sptr(); //timeout waiting for flow control
        }
    }

    managed_send_buffer::sptr buff = xport->get_send_buff(timeout);
    if (buff) {
        fc_cache->last_seq_out++; //update seq, this will actually be a send
    }
    return buff;
}

#define DEVICE3_ASYNC_EVENT_CODE_FLOW_CTRL 0
/*! Handle incoming messages. If they're flow control, update the TX FC cache.
 * Otherwise, send them to the async message queue for the user to poll.
 *
 * This is run inside a uhd::task as long as this streamer lives.
 */
static void handle_tx_async_msgs(
        boost::shared_ptr<tx_fc_cache_t> fc_cache,
        zero_copy_if::sptr xport,
        endianness_t endianness,
        boost::function<double(size_t)> get_tick_rate, size_t mb_index
) {
    managed_recv_buffer::sptr buff = xport->get_recv_buff();
    if (not buff)
        return;

    //extract packet info
    vrt::if_packet_info_t if_packet_info;
    if_packet_info.num_packet_words32 = buff->size()/sizeof(boost::uint32_t);
    const boost::uint32_t *packet_buff = buff->cast<const boost::uint32_t *>();

    //unpacking can fail
    boost::uint32_t (*endian_conv)(boost::uint32_t) = uhd::ntohx;
    try
    {
        if (endianness == ENDIANNESS_BIG)
        {
            device3_if_hdr_unpack_be(packet_buff, if_packet_info);
            endian_conv = uhd::ntohx;
        }
        else
        {
            device3_if_hdr_unpack_le(packet_buff, if_packet_info);
            endian_conv = uhd::wtohx;
        }
    }
    catch(const std::exception &ex)
    {
        UHD_MSG(error) << "Error parsing async message packet: " << ex.what() << std::endl;
        return;
    }

    //fill in the async metadata
    async_metadata_t metadata;
    load_metadata_from_buff(
            endian_conv,
            metadata,
            if_packet_info,
            packet_buff,
            get_tick_rate(mb_index),
            fc_cache->stream_channel
    );

    // TODO: Shouldn't we be polling if_packet_info.packet_type == PACKET_TYPE_FC?
    //       Thing is, on X300, packet_type == 0, so that wouldn't work. But it seems it should.
    //The FC response and the burst ack are two indicators that the radio
    //consumed packets. Use them to update the FC metadata
    if (metadata.event_code == DEVICE3_ASYNC_EVENT_CODE_FLOW_CTRL or
        metadata.event_code == async_metadata_t::EVENT_CODE_BURST_ACK
    ) {
        const size_t seq = metadata.user_payload[0];
        fc_cache->seq_queue.push_with_pop_on_full(seq);
    }

    //FC responses don't propagate up to the user so filter them here
    if (metadata.event_code != DEVICE3_ASYNC_EVENT_CODE_FLOW_CTRL) {
        fc_cache->async_queue->push_with_pop_on_full(metadata);
        metadata.channel = fc_cache->device_channel;
        fc_cache->old_async_queue->push_with_pop_on_full(metadata);
        standard_async_msg_prints(metadata);
    }
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
 * Receive streamer
 **********************************************************************/
rx_streamer::sptr device3_impl::get_rx_stream(const stream_args_t &args_)
{
    boost::mutex::scoped_lock lock(_transport_setup_mutex);
    stream_args_t args = sanitize_stream_args(args_);

    // I. Generate the channel list
    std::vector<uhd::rfnoc::block_id_t> chan_list;
    std::vector<device_addr_t> chan_args;
    generate_channel_list(args, chan_list, chan_args, "rx");

    // II. Iterate over all channels
    boost::shared_ptr<sph::recv_packet_streamer> my_streamer;
    for (size_t stream_i = 0; stream_i < chan_list.size(); stream_i++) {
        // Get block ID and mb index
        uhd::rfnoc::block_id_t block_id = chan_list[stream_i];
        size_t mb_index = block_id.get_device_no();
        // TODO "any port"
        size_t block_port = chan_args[stream_i].cast<size_t>("block_port", 0);
        UHD_ASSERT_THROW(block_port < 16); // TODO replace with a check against the actual block definition

        // Access to this channel's block control
        uhd::rfnoc::rx_block_ctrl_base::sptr blk_ctrl =
            boost::dynamic_pointer_cast<uhd::rfnoc::rx_block_ctrl_base>(get_block_ctrl(block_id));

        // Setup the DSP transport hints
        device_addr_t rx_hints = get_rx_hints(mb_index);

        //allocate sid and create transport
        uhd::sid_t stream_address = blk_ctrl->get_address(block_port);
        UHD_MSG(status) << "creating rx stream " << rx_hints.to_string() << std::endl;
        both_xports_t xport = make_transport(stream_address, RX_DATA, rx_hints);
        UHD_MSG(status) << std::hex << "data_sid = " << xport.send_sid << std::dec << " actual recv_buff_size = " << xport.recv_buff_size << std::endl;

        // Configure the block (this may change args)
        blk_ctrl->setup_rx_streamer(args);
        blk_ctrl->set_destination(xport.send_sid.get_src(), block_port);

        // To calculate the max number of samples per packet, we assume the maximum header length
        // to avoid fragmentation should the entire header be used.
        const size_t bpp = xport.recv->get_recv_frame_size() - stream_options.rx_max_len_hdr; // bytes per packet
        const size_t bpi = convert::get_bytes_per_item(args.otw_format); // bytes per item
        const size_t spp = std::min(args.args.cast<size_t>("spp", bpp/bpi), bpp/bpi); // samples per packet
        // TODO: check this is fine with E300

        //make the new streamer given the samples per packet
        if (not my_streamer)
            my_streamer = boost::make_shared<sph::recv_packet_streamer>(spp);
        my_streamer->resize(args.channels.size());

        //init some streamer stuff
        std::string conv_endianness;
        if (get_transport_endianness(mb_index) == ENDIANNESS_BIG) {
            my_streamer->set_vrt_unpacker(&device3_if_hdr_unpack_be);
            conv_endianness = "be";
        } else {
            my_streamer->set_vrt_unpacker(&device3_if_hdr_unpack_le);
            conv_endianness = "le";
        }

        //set the converter
        uhd::convert::id_type id;
        id.input_format = args.otw_format + "_item32_" + conv_endianness;
        id.num_inputs = 1;
        id.output_format = args.cpu_format;
        id.num_outputs = 1;
        my_streamer->set_converter(id);

        // Add terminator. Its lifetime is coupled to the streamer.
        rfnoc::sink_node_ctrl::sptr recv_terminator = rfnoc::terminator_recv::make();
        blk_ctrl->register_downstream_node(recv_terminator, block_port);
        my_streamer->store_terminator(recv_terminator);

        //flow control setup
        const size_t pkt_size = spp * bpi + stream_options.rx_max_len_hdr;
        const size_t fc_window = get_rx_flow_control_window(pkt_size, xport.recv_buff_size, rx_hints);
        const size_t fc_handle_window = std::max<size_t>(1, fc_window / stream_options.rx_fc_request_freq);
        UHD_MSG(status)<< "RX Flow Control Window = " << fc_window << ", RX Flow Control Handler Window = " << fc_handle_window << std::endl;
        blk_ctrl->configure_flow_control_out(
                fc_window,
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
                &uhd::rfnoc::rx_block_ctrl_base::handle_overrun, blk_ctrl,
                boost::weak_ptr<uhd::rx_streamer>(my_streamer)
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
            stream_i, boost::bind(&uhd::rfnoc::rx_block_ctrl_base::issue_stream_cmd, blk_ctrl, _1)
        );

        // Tell the streamer which SID is valid for this channel
        my_streamer->set_xport_chan_sid(stream_i, true, xport.send_sid);

        // Store a weak pointer to prevent a streamer->device3_impl->streamer circular dependency
        _rx_streamers[blk_ctrl->get_block_id().get()] = boost::weak_ptr<sph::recv_packet_streamer>(my_streamer);

        //sets all tick and samp rates on this streamer
        const fs_path mb_path = "/mboards";
        _tree->access<double>(mb_path / mb_index / "tick_rate").update();
        // TODO this is specific to radios and thus should be done by radio_ctrl
        if (blk_ctrl->get_block_id().get_block_name() == "Radio") {
            UHD_MSG(status) << "This is a radio, thus updating sample rate" << std::endl;
            _tree->access<double>(mb_path / mb_index / "rx_dsps" / blk_ctrl->get_block_id().get_block_count() / "rate" / "value").update();
        }
    }

    post_streamer_hooks(false);
    return my_streamer;

}

/***********************************************************************
 * Transmit streamer
 **********************************************************************/
tx_streamer::sptr device3_impl::get_tx_stream(const uhd::stream_args_t &args_)
{
    boost::mutex::scoped_lock lock(_transport_setup_mutex);
    stream_args_t args = sanitize_stream_args(args_);

    // I. Generate the channel list
    std::vector<uhd::rfnoc::block_id_t> chan_list;
    std::vector<device_addr_t> chan_args;
    generate_channel_list(args, chan_list, chan_args, "tx");

    //shared async queue for all channels in streamer
    boost::shared_ptr<async_md_type> async_md(new async_md_type(1000/*messages deep*/));

    // II. Iterate over all channels
    boost::shared_ptr<sph::send_packet_streamer> my_streamer;
    for (size_t stream_i = 0; stream_i < chan_list.size(); stream_i++) {
        // Get block ID and mb index
        uhd::rfnoc::block_id_t block_id = chan_list[stream_i];
        size_t mb_index = block_id.get_device_no();
        // TODO: "any port"
        size_t block_port = chan_args[stream_i].cast<size_t>("block_port", 0);
        // TODO: more elegant check, see if this block is actually valid
        //       (may go into connect call or something)
        UHD_ASSERT_THROW(block_port < 16);

        // Access to this channel's block control
        uhd::rfnoc::tx_block_ctrl_base::sptr blk_ctrl =
            boost::dynamic_pointer_cast<uhd::rfnoc::tx_block_ctrl_base>(get_block_ctrl(block_id));

        // Setup the dsp transport hints
        device_addr_t tx_hints = get_tx_hints(mb_index);

        //allocate sid and create transport
        uhd::sid_t stream_address = blk_ctrl->get_address(block_port);
        UHD_MSG(status) << "creating tx stream " << tx_hints.to_string() << std::endl;
        both_xports_t xport = make_transport(stream_address, TX_DATA, tx_hints);
        UHD_MSG(status) << std::hex << "data_sid = " << xport.send_sid << std::dec << std::endl;

        // Configure the block (this may change args)
        blk_ctrl->setup_tx_streamer(args);

        // To calculate the max number of samples per packet, we assume the maximum header length
        // to avoid fragmentation should the entire header be used.
        const size_t bpp = xport.send->get_send_frame_size() - stream_options.tx_max_len_hdr;
        const size_t bpi = convert::get_bytes_per_item(args.otw_format); // bytes per item
        const size_t spp = std::min(args.args.cast<size_t>("spp", bpp/bpi), bpp/bpi); // samples per packet

        //make the new streamer given the samples per packet
        if (not my_streamer)
            my_streamer = boost::make_shared<sph::send_packet_streamer>(spp);
        my_streamer->resize(args.channels.size());

        //init some streamer stuff
        std::string conv_endianness;
        if (get_transport_endianness(mb_index) == ENDIANNESS_BIG) {
            my_streamer->set_vrt_packer(&device3_if_hdr_pack_be);
            conv_endianness = "be";
        } else {
            my_streamer->set_vrt_packer(&device3_if_hdr_pack_le);
            conv_endianness = "le";
        }

        //set the converter
        uhd::convert::id_type id;
        id.input_format = args.cpu_format;
        id.num_inputs = 1;
        id.output_format = args.otw_format + "_item32_" + conv_endianness;
        id.num_outputs = 1;
        my_streamer->set_converter(id);

        // Add terminator. Its lifetime is coupled to the streamer.
        rfnoc::source_node_ctrl::sptr send_terminator = rfnoc::terminator_send::make();
        blk_ctrl->register_upstream_node(send_terminator, block_port);
        my_streamer->store_terminator(send_terminator);

        //flow control setup
        const size_t pkt_size = spp * bpi + stream_options.tx_max_len_hdr;
        // For flow control, this value is used to determine the window size in *packets*
        size_t fc_window = get_tx_flow_control_window(
                pkt_size, // This is the maximum packet size
                blk_ctrl->get_fifo_size(block_port),
                tx_hints // This can override the value reported by the block!
        );
        const size_t fc_handle_window = std::max<size_t>(1, fc_window / stream_options.tx_fc_response_freq);
        UHD_MSG(status) << "TX Flow Control Window = " << fc_window << ", TX Flow Control Handler Window = " << fc_handle_window << std::endl;
        blk_ctrl->configure_flow_control_in(
                stream_options.tx_fc_response_cycles,
                fc_handle_window, /*pkts*/
                block_port
        );

        boost::shared_ptr<tx_fc_cache_t> fc_cache(new tx_fc_cache_t());
        fc_cache->stream_channel = stream_i;
        fc_cache->device_channel = mb_index;
        fc_cache->async_queue = async_md;
        fc_cache->old_async_queue = _async_md;

        task::sptr task = task::make(
                boost::bind(
                    &handle_tx_async_msgs,
                    fc_cache,
                    xport.recv,
                    get_transport_endianness(mb_index),
                    _tick_rate_retriever, mb_index
                )
        );

        //Give the streamer a functor to get the send buffer
        //get_tx_buff_with_flowctrl is static so bind has no lifetime issues
        //xport.send (sptr) is required to add streamer->data-transport lifetime dependency
        //task (sptr) is required to add  a streamer->async-handler lifetime dependency
        my_streamer->set_xport_chan_get_buff(
            stream_i,
            boost::bind(&get_tx_buff_with_flowctrl, task, fc_cache, xport.send, fc_window, _1)
        );
        //Give the streamer a functor handled received async messages
        my_streamer->set_async_receiver(
            boost::bind(&async_md_type::pop_with_timed_wait, async_md, _1, _2)
        );
        my_streamer->set_xport_chan_sid(stream_i, true, xport.send_sid);
        my_streamer->set_enable_trailer(false); //TODO not implemented trailer support yet

        //Store a weak pointer to prevent a streamer->device3_impl->streamer circular dependency
        _tx_streamers[blk_ctrl->get_block_id().get()] = boost::weak_ptr<sph::send_packet_streamer>(my_streamer);

        //sets all tick and samp rates on this streamer
        const fs_path mb_path = "/mboards";
        _tree->access<double>(mb_path / mb_index / "tick_rate").update();
        // TODO this is specific to radios and thus should be done by radio_ctrl
        if (blk_ctrl->get_block_id().get_block_name() == "Radio") {
            UHD_MSG(status) << "This is a radio, thus updating sample rate" << std::endl;
            _tree->access<double>(mb_path / mb_index / "tx_dsps" / blk_ctrl->get_block_id().get_block_count() / "rate" / "value").update();
        }
    }

    post_streamer_hooks(true);
    return my_streamer;
}
