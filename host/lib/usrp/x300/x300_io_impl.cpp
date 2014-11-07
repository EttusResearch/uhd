//
// Copyright 2013-2014 Ettus Research LLC
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

#include "x300_regs.hpp"
#include "x300_impl.hpp"
#include "validate_subdev_spec.hpp"
#include "../../transport/super_recv_packet_handler.hpp"
#include "../../transport/super_send_packet_handler.hpp"
#include <uhd/transport/nirio_zero_copy.hpp>
#include "async_packet_handler.hpp"
#include <uhd/transport/bounded_buffer.hpp>
#include <boost/bind.hpp>
#include <uhd/utils/tasks.hpp>
#include <uhd/utils/log.hpp>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>
#include <uhd/usrp/rfnoc/rx_block_ctrl_base.hpp>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;

/***********************************************************************
 * update streamer rates
 **********************************************************************/
// TODO: Move to device3?
void x300_impl::update_tick_rate(mboard_members_t &mb, const double rate)
{
    BOOST_FOREACH(const std::string &block_id, mb.rx_streamers.keys()) {
        UHD_MSG(status) << "setting rx streamer " << block_id << " rate to " << rate << std::endl;
        boost::shared_ptr<sph::recv_packet_streamer> my_streamer =
            boost::dynamic_pointer_cast<sph::recv_packet_streamer>(mb.rx_streamers[block_id].lock());
        if (my_streamer) {
            my_streamer->set_tick_rate(rate);
            my_streamer->set_samp_rate(rate);
        }
    }
    BOOST_FOREACH(const std::string &block_id, mb.tx_streamers.keys()) {
        UHD_MSG(status) << "setting tx streamer " << block_id << " rate to " << rate << std::endl;
        boost::shared_ptr<sph::send_packet_streamer> my_streamer =
            boost::dynamic_pointer_cast<sph::send_packet_streamer>(mb.tx_streamers[block_id].lock());
        if (my_streamer) {
            my_streamer->set_tick_rate(rate);
            my_streamer->set_samp_rate(rate);
        }
    }
}

// TODO: Move to device3?
void x300_impl::update_rx_samp_rate(mboard_members_t &mb, const size_t dspno, const double rate)
{
    std::string radio_block_id = str(boost::format("Radio_%d") % dspno);
    if (not mb.rx_streamers.has_key(radio_block_id)) return;
    boost::shared_ptr<sph::recv_packet_streamer> my_streamer =
        boost::dynamic_pointer_cast<sph::recv_packet_streamer>(mb.rx_streamers[radio_block_id].lock());
    if (not my_streamer) return;
    my_streamer->set_samp_rate(rate);
    // TODO move these details to radio_ctrl
    const double adj = mb.radio_perifs[dspno].ddc->get_scaling_adjustment();
    my_streamer->set_scale_factor(adj);
}

// TODO: Move to device3?
void x300_impl::update_tx_samp_rate(mboard_members_t &mb, const size_t dspno, const double rate)
{
    std::string radio_block_id = str(boost::format("Radio_%d") % dspno);
    if (not mb.tx_streamers.has_key(radio_block_id)) return;
    boost::shared_ptr<sph::send_packet_streamer> my_streamer =
        boost::dynamic_pointer_cast<sph::send_packet_streamer>(mb.tx_streamers[radio_block_id].lock());
    if (not my_streamer) return;
    my_streamer->set_samp_rate(rate);
    // TODO move these details to radio_ctrl
    const double adj = mb.radio_perifs[dspno].duc->get_scaling_adjustment();
    my_streamer->set_scale_factor(adj);
}

/***********************************************************************
 * Setup dboard muxing for IQ
 **********************************************************************/
void x300_impl::update_subdev_spec(const std::string &tx_rx, const size_t mb_i, const subdev_spec_t &spec)
{
    UHD_ASSERT_THROW(tx_rx == "tx" or tx_rx == "rx");
    UHD_ASSERT_THROW(mb_i < _mb.size());
    const std::string mb_name = boost::lexical_cast<std::string>(mb_i);
    fs_path mb_root = "/mboards/" + mb_name;

    //sanity checking
    validate_subdev_spec(_tree, spec, tx_rx, mb_name);
    UHD_ASSERT_THROW(spec.size() <= 2);
    if (spec.size() == 1) {
        UHD_ASSERT_THROW(spec[0].db_name == "A" || spec[0].db_name == "B");
    }
    else if (spec.size() == 2) {
        UHD_ASSERT_THROW(
            (spec[0].db_name == "A" && spec[1].db_name == "B") ||
            (spec[0].db_name == "B" && spec[1].db_name == "A")
        );
    }

    std::vector<size_t> chan_to_dsp_map(spec.size(), 0);
    // setup mux for this spec
    for (size_t i = 0; i < spec.size(); i++)
    {
        const int radio_idx = _mb[mb_i].get_radio_index(spec[i].db_name);
        chan_to_dsp_map[i] = radio_idx;

        //extract connection
        const std::string conn = _tree->access<std::string>(mb_root / "dboards" / spec[i].db_name / (tx_rx + "_frontends") / spec[i].sd_name / "connection").get();

        if (tx_rx == "tx") {
            //swap condition
            _mb[mb_i].radio_perifs[radio_idx].tx_fe->set_mux(conn);
        } else {
            //swap condition
            const bool fe_swapped = (conn == "QI" or conn == "Q");
            _mb[mb_i].radio_perifs[radio_idx].ddc->set_mux(conn, fe_swapped);
            //see usrp/io_impl.cpp if multiple DSPs share the frontend:
            _mb[mb_i].radio_perifs[radio_idx].rx_fe->set_mux(fe_swapped);
        }
    }

    _tree->access<std::vector<size_t> >(mb_root / (tx_rx + "_chan_dsp_mapping")).set(chan_to_dsp_map);
}


/***********************************************************************
 * VITA stuff
 **********************************************************************/
static void x300_if_hdr_unpack_be(
    const boost::uint32_t *packet_buff,
    vrt::if_packet_info_t &if_packet_info
){
    if_packet_info.link_type = vrt::if_packet_info_t::LINK_TYPE_CHDR;
    return vrt::if_hdr_unpack_be(packet_buff, if_packet_info);
}

static void x300_if_hdr_pack_be(
    boost::uint32_t *packet_buff,
    vrt::if_packet_info_t &if_packet_info
){
    if_packet_info.link_type = vrt::if_packet_info_t::LINK_TYPE_CHDR;
    return vrt::if_hdr_pack_be(packet_buff, if_packet_info);
}

static void x300_if_hdr_unpack_le(
    const boost::uint32_t *packet_buff,
    vrt::if_packet_info_t &if_packet_info
){
    if_packet_info.link_type = vrt::if_packet_info_t::LINK_TYPE_CHDR;
    return vrt::if_hdr_unpack_le(packet_buff, if_packet_info);
}

static void x300_if_hdr_pack_le(
    boost::uint32_t *packet_buff,
    vrt::if_packet_info_t &if_packet_info
){
    if_packet_info.link_type = vrt::if_packet_info_t::LINK_TYPE_CHDR;
    return vrt::if_hdr_pack_le(packet_buff, if_packet_info);
}

/***********************************************************************
 * RX flow control handler
 **********************************************************************/
// TODO move to rx_block_ctrl_base
static size_t get_rx_flow_control_window(size_t frame_size, size_t sw_buff_size, const device_addr_t& rx_args)
{
    double fullness_factor = rx_args.cast<double>("recv_buff_fullness", X300_RX_SW_BUFF_FULL_FACTOR);

    if (fullness_factor < 0.01 || fullness_factor > 1) {
        throw uhd::value_error("recv_buff_fullness must be between 0.01 and 1 inclusive (1% to 100%)");
    }

    size_t window_in_pkts = (static_cast<size_t>(sw_buff_size * fullness_factor) / frame_size);
    if (window_in_pkts == 0) {
        throw uhd::value_error("recv_buff_size must be larger than the recv_frame_size.");
    }
    return window_in_pkts;
}

// TODO: Definitely move to rx_block_ctrl_base
static void handle_rx_flowctrl(const boost::uint32_t sid, zero_copy_if::sptr xport, bool big_endian, boost::shared_ptr<boost::uint32_t> seq32_state, const size_t last_seq)
{
    // TODO remove this counter once the debug msg is gone
    static size_t fc_pkt_count = 0;
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

    //UHD_MSG(status) << "sending flow ctrl packet " << fc_pkt_count++ << ", acking " << str(boost::format("%04d\tseq32==0x%08x") % last_seq % seq32) << std::endl;

    //load packet info
    vrt::if_packet_info_t packet_info;
    packet_info.packet_type = vrt::if_packet_info_t::PACKET_TYPE_FC; // FC!
    packet_info.num_payload_words32 = 2;
    packet_info.num_payload_bytes = packet_info.num_payload_words32*sizeof(boost::uint32_t);
    packet_info.packet_count = seq32;
    packet_info.sob = false;
    packet_info.eob = false;
    packet_info.sid = sid;
    packet_info.has_sid = true;
    packet_info.has_cid = false;
    packet_info.has_tsi = false;
    packet_info.has_tsf = false;
    packet_info.has_tlr = false;

    //load header
    if (big_endian)
        x300_if_hdr_pack_be(pkt, packet_info);
    else
        x300_if_hdr_pack_le(pkt, packet_info);

    //load payload
    pkt[packet_info.num_header_words32+0] = uhd::htonx<boost::uint32_t>(0);
    pkt[packet_info.num_header_words32+1] = uhd::htonx<boost::uint32_t>(seq32);

    // hardcode bits
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
 * TX flow control handler
 **********************************************************************/
struct x300_tx_fc_guts_t
{
    x300_tx_fc_guts_t(void):
        stream_channel(0),
        device_channel(0),
        last_seq_out(0),
        last_seq_ack(0),
        seq_queue(1){}
    size_t stream_channel;
    size_t device_channel;
    size_t last_seq_out;
    size_t last_seq_ack;
    bounded_buffer<size_t> seq_queue;
    boost::shared_ptr<x300_impl::async_md_type> async_queue;
    boost::shared_ptr<x300_impl::async_md_type> old_async_queue;
};

#define X300_ASYNC_EVENT_CODE_FLOW_CTRL 0

static size_t get_tx_flow_control_window(size_t frame_size, const device_addr_t& tx_args)
{
    double hw_buff_size = tx_args.cast<double>("send_buff_size", X300_TX_HW_BUFF_SIZE);
    size_t window_in_pkts = (static_cast<size_t>(hw_buff_size) / frame_size);
    if (window_in_pkts == 0) {
        throw uhd::value_error("send_buff_size must be larger than the send_frame_size.");
    }
    return window_in_pkts;
}

static void handle_tx_async_msgs(boost::shared_ptr<x300_tx_fc_guts_t> guts, zero_copy_if::sptr xport, bool big_endian, x300_clock_ctrl::sptr clock)
{
    managed_recv_buffer::sptr buff = xport->get_recv_buff();
    if (not buff) return;

    //extract packet info
    vrt::if_packet_info_t if_packet_info;
    if_packet_info.num_packet_words32 = buff->size()/sizeof(boost::uint32_t);
    const boost::uint32_t *packet_buff = buff->cast<const boost::uint32_t *>();

    //unpacking can fail
    boost::uint32_t (*endian_conv)(boost::uint32_t) = uhd::ntohx;
    try
    {
        if (big_endian)
        {
            x300_if_hdr_unpack_be(packet_buff, if_packet_info);
            endian_conv = uhd::ntohx;
        }
        else
        {
            x300_if_hdr_unpack_le(packet_buff, if_packet_info);
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
        endian_conv, metadata, if_packet_info, packet_buff,
        clock->get_master_clock_rate(), guts->stream_channel);

    //The FC response and the burst ack are two indicators that the radio
    //consumed packets. Use them to update the FC metadata
    if (metadata.event_code == X300_ASYNC_EVENT_CODE_FLOW_CTRL or
        metadata.event_code == async_metadata_t::EVENT_CODE_BURST_ACK
    ) {
        const size_t seq = metadata.user_payload[0];
        guts->seq_queue.push_with_pop_on_full(seq);
    }

    //FC responses don't propagate up to the user so filter them here
    if (metadata.event_code != X300_ASYNC_EVENT_CODE_FLOW_CTRL) {
        guts->async_queue->push_with_pop_on_full(metadata);
        metadata.channel = guts->device_channel;
        guts->old_async_queue->push_with_pop_on_full(metadata);
        standard_async_msg_prints(metadata);
    }
}

static managed_send_buffer::sptr get_tx_buff_with_flowctrl(
    task::sptr /*holds ref*/,
    boost::shared_ptr<x300_tx_fc_guts_t> guts,
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
bool x300_impl::recv_async_msg(
    async_metadata_t &async_metadata, double timeout
){
    return _async_md->pop_with_timed_wait(async_metadata, timeout);
}


/***********************************************************************
 * Helper functions for get_?x_stream()
 * TODO: Move these up, they are generic
 **********************************************************************/
uhd::stream_args_t sanitize_stream_args(const uhd::stream_args_t &args_)
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

void x300_impl::generate_channel_list(
        const uhd::stream_args_t &args_,
        std::vector<uhd::rfnoc::block_id_t> &chan_list,
        std::vector<device_addr_t> &chan_args
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
            fs_path chan_root = str(boost::format("/channels/%d") % chan_idx);
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
 * Receive streamer
 **********************************************************************/
rx_streamer::sptr x300_impl::get_rx_stream(const uhd::stream_args_t &args_)
{
    boost::mutex::scoped_lock lock(_transport_setup_mutex);
    stream_args_t args = sanitize_stream_args(args_);

    // I. Generate the channel list
    std::vector<uhd::rfnoc::block_id_t> chan_list;
    std::vector<device_addr_t> chan_args;
    generate_channel_list(args, chan_list, chan_args);

    // II. Iterate over all channels
    boost::shared_ptr<sph::recv_packet_streamer> my_streamer;
    for (size_t stream_i = 0; stream_i < chan_list.size(); stream_i++) {
        // Get block ID and mb index
        uhd::rfnoc::block_id_t block_id = chan_list[stream_i];
        size_t mb_index = block_id.get_device_no();
        UHD_ASSERT_THROW(mb_index < _mb.size());
        size_t block_port = chan_args[stream_i].cast<size_t>("block_port", 0);
        UHD_ASSERT_THROW(block_port < 16); // TODO replace with a check against the actual block definition

        // Access to this channel's mboard and block control
        mboard_members_t &mb = _mb[mb_index];
        uhd::rfnoc::rx_block_ctrl_base::sptr ce_ctrl =
            boost::dynamic_pointer_cast<uhd::rfnoc::rx_block_ctrl_base>(get_block_ctrl(block_id));

        // Setup the DSP transport hints (default to a large recv buff)
        device_addr_t device_addr = mb.recv_args;
        if (not device_addr.has_key("recv_buff_size"))
        {
            if (mb.xport_path != "nirio") {
                //For the ethernet transport, the buffer has to be set before creating
                //the transport because it is independent of the frame size and # frames
                //For nirio, the buffer size is not configurable by the user
                #if defined(UHD_PLATFORM_MACOS) || defined(UHD_PLATFORM_BSD)
                    //limit buffer resize on macos or it will error
                    device_addr["recv_buff_size"] = boost::lexical_cast<std::string>(X300_RX_SW_BUFF_SIZE_ETH_MACOS);
                #elif defined(UHD_PLATFORM_LINUX) || defined(UHD_PLATFORM_WIN32)
                    //set to half-a-second of buffering at max rate
                    device_addr["recv_buff_size"] = boost::lexical_cast<std::string>(X300_RX_SW_BUFF_SIZE_ETH);
                #endif
            }
        }

        //allocate sid and create transport
        uhd::sid_t stream_address = ce_ctrl->get_address(block_port);
        UHD_MSG(status) << "creating rx stream " << device_addr.to_string() << std::endl;
        both_xports_t xport = this->make_transport(stream_address, device_addr);
        UHD_MSG(status) << std::hex << "data_sid = " << xport.send_sid << std::dec << " actual recv_buff_size = " << xport.recv_buff_size << std::endl;

        // To calculate the max number of samples per packet, we assume the maximum header length
        // to avoid fragmentation should the entire header be used.
        const size_t bpp = xport.recv->get_recv_frame_size() - X300_RX_MAX_HDR_LEN; // bytes per packet
        const size_t bpi = convert::get_bytes_per_item(args.otw_format); // bytes per item
        const size_t spp = std::min(args.args.cast<size_t>("spp", bpp/bpi), bpp/bpi); // samples per packet

        //make the new streamer given the samples per packet
        if (not my_streamer) my_streamer = boost::make_shared<sph::recv_packet_streamer>(spp);
        my_streamer->resize(args.channels.size());

        //init some streamer stuff
        std::string conv_endianness;
        if (mb.if_pkt_is_big_endian) {
            my_streamer->set_vrt_unpacker(&x300_if_hdr_unpack_be);
            conv_endianness = "be";
        } else {
            my_streamer->set_vrt_unpacker(&x300_if_hdr_unpack_le);
            conv_endianness = "le";
        }

        //set the converter
        uhd::convert::id_type id;
        id.input_format = args.otw_format + "_item32_" + conv_endianness;
        id.num_inputs = 1;
        id.output_format = args.cpu_format;
        id.num_outputs = 1;
        my_streamer->set_converter(id);

        // Configure the block
        ce_ctrl->setup_rx_streamer(args);
        ce_ctrl->set_destination(xport.send_sid.get_src(), block_port);

        //flow control setup
        const size_t pkt_size = spp * bpi + X300_RX_MAX_HDR_LEN;
        const size_t fc_window = get_rx_flow_control_window(pkt_size, xport.recv_buff_size, device_addr);
        const size_t fc_handle_window = std::max<size_t>(1, fc_window / X300_RX_FC_REQUEST_FREQ);
        UHD_LOG << "RX Flow Control Window = " << fc_window << ", RX Flow Control Handler Window = " << fc_handle_window << std::endl;
        ce_ctrl->configure_flow_control_out(fc_window, block_port);

        //Give the streamer a functor to get the recv_buffer
        //bind requires a zero_copy_if::sptr to add a streamer->xport lifetime dependency
        my_streamer->set_xport_chan_get_buff(
            stream_i,
            boost::bind(&zero_copy_if::get_recv_buff, xport.recv, _1),
            true /*flush*/
        );

        //Give the streamer a functor to handle overruns
        //bind requires a weak_ptr to break the a streamer->streamer circular dependency
        //Using "this" is OK because we know that x300_impl will outlive the streamer
        my_streamer->set_overflow_handler(
            stream_i,
            boost::bind(
                &uhd::rfnoc::rx_block_ctrl_base::handle_overrun, ce_ctrl,
                boost::weak_ptr<uhd::rx_streamer>(my_streamer)
            )
        );

        //Give the streamer a functor to send flow control messages
        //handle_rx_flowctrl is static and has no lifetime issues
        boost::shared_ptr<boost::uint32_t> seq32(new boost::uint32_t(0));
        my_streamer->set_xport_handle_flowctrl(
            stream_i, boost::bind(&handle_rx_flowctrl, xport.send_sid, xport.send, mb.if_pkt_is_big_endian, seq32, _1),
            fc_handle_window,
            true/*init*/
        );

        //Give the streamer a functor issue stream cmd
        //bind requires a shared pointer to add a streamer->framer lifetime dependency
        my_streamer->set_issue_stream_cmd(
            stream_i, boost::bind(&uhd::rfnoc::rx_block_ctrl_base::issue_stream_cmd, ce_ctrl, _1)
        );

        // Tell the streamer which SID is valid for this channel
        my_streamer->set_xport_chan_sid(stream_i, true, xport.send_sid);

        // Store a weak pointer to prevent a streamer->x300_impl->streamer circular dependency
        mb.rx_streamers[ce_ctrl->get_block_id().get()] = boost::weak_ptr<sph::recv_packet_streamer>(my_streamer);

        //sets all tick and samp rates on this streamer
        const fs_path mb_path = "/mboards/"+boost::lexical_cast<std::string>(mb_index);
        _tree->access<double>(mb_path / "tick_rate").update();
        // TODO this is specific to radios and thus should be done by radio_ctrl
        if (ce_ctrl->get_block_id().get_block_name() == "Radio") {
            UHD_MSG(status) << "This is a radio, thus updating sample rate" << std::endl;
            _tree->access<double>(mb_path / "rx_dsps" / boost::lexical_cast<std::string>(ce_ctrl->get_block_id().get_block_count()) / "rate" / "value").update();
        }
    }

    return my_streamer;
}

/***********************************************************************
 * Transmit streamer
 **********************************************************************/
tx_streamer::sptr x300_impl::get_tx_stream(const uhd::stream_args_t &args_)
{
    boost::mutex::scoped_lock lock(_transport_setup_mutex);
    stream_args_t args = sanitize_stream_args(args_);

    // I. Generate the channel list
    std::vector<uhd::rfnoc::block_id_t> chan_list;
    std::vector<device_addr_t> chan_args;
    generate_channel_list(args, chan_list, chan_args);

    //shared async queue for all channels in streamer
    boost::shared_ptr<async_md_type> async_md(new async_md_type(1000/*messages deep*/));

    // II. Iterate over all channels
    boost::shared_ptr<sph::send_packet_streamer> my_streamer;
    for (size_t stream_i = 0; stream_i < chan_list.size(); stream_i++) {
        // Get block ID and mb index
        uhd::rfnoc::block_id_t block_id = chan_list[stream_i];
        size_t mb_index = block_id.get_device_no();
        UHD_ASSERT_THROW(mb_index < _mb.size());
        size_t block_port = chan_args[stream_i].cast<size_t>("block_port", 0);
        UHD_ASSERT_THROW(block_port < 16);

        // Access to this channel's mboard and block control
        mboard_members_t &mb = _mb[mb_index];
        uhd::rfnoc::tx_block_ctrl_base::sptr ce_ctrl =
            boost::dynamic_pointer_cast<uhd::rfnoc::tx_block_ctrl_base>(get_block_ctrl(block_id));

        // Setup the dsp transport hints
        device_addr_t device_tx_args = mb.send_args;

        //allocate sid and create transport
        uhd::sid_t stream_address = ce_ctrl->get_address(block_port);
        UHD_MSG(status) << "creating tx stream " << device_tx_args.to_string() << std::endl;
        both_xports_t xport = this->make_transport(stream_address, device_tx_args);
        UHD_MSG(status) << std::hex << "data_sid = " << xport.send_sid << std::dec << std::endl;
        UHD_VAR(xport.send_buff_size);

        // To calculate the max number of samples per packet, we assume the maximum header length
        // to avoid fragmentation should the entire header be used.
        const size_t bpp = xport.send->get_send_frame_size() - X300_TX_MAX_HDR_LEN;
        const size_t bpi = convert::get_bytes_per_item(args.otw_format);
        const size_t spp = std::min(args.args.cast<size_t>("spp", bpp/bpi), bpp/bpi); // samples per packet

        //make the new streamer given the samples per packet
        if (not my_streamer) my_streamer = boost::make_shared<sph::send_packet_streamer>(spp);
        my_streamer->resize(args.channels.size());

        //init some streamer stuff
        std::string conv_endianness;
        if (mb.if_pkt_is_big_endian) {
            my_streamer->set_vrt_packer(&x300_if_hdr_pack_be);
            conv_endianness = "be";
        } else {
            my_streamer->set_vrt_packer(&x300_if_hdr_pack_le);
            conv_endianness = "le";
        }

        //set the converter
        uhd::convert::id_type id;
        id.input_format = args.cpu_format;
        id.num_inputs = 1;
        id.output_format = args.otw_format + "_item32_" + conv_endianness;
        id.num_outputs = 1;
        my_streamer->set_converter(id);

        // Configure the block
        ce_ctrl->setup_tx_streamer(args);

        //flow control setup
        const size_t pkt_size = spp * bpi + X300_TX_MAX_HDR_LEN;
        UHD_VAR(pkt_size);
        // For flow control, this value is used to determine the window size
        device_tx_args["send_buff_size"] = boost::lexical_cast<std::string>(ce_ctrl->get_fifo_size(block_port));
        size_t fc_window = get_tx_flow_control_window(pkt_size, device_tx_args);  //In packets
        const size_t fc_handle_window = std::max<size_t>(1, fc_window/X300_TX_FC_RESPONSE_FREQ);
        UHD_MSG(status) << "TX Flow Control Window = " << fc_window << ", TX Flow Control Handler Window = " << fc_handle_window << std::endl;
        ce_ctrl->configure_flow_control_in(0/*cycs off*/, fc_handle_window, block_port);

        boost::shared_ptr<x300_tx_fc_guts_t> guts(new x300_tx_fc_guts_t());
        guts->stream_channel = stream_i;
        // TODO: Do we really need to distinguish devices here?
        guts->device_channel = stream_i;
        guts->async_queue = async_md;
        guts->old_async_queue = _async_md;
        task::sptr task = task::make(boost::bind(&handle_tx_async_msgs, guts, xport.recv, mb.if_pkt_is_big_endian, mb.clock));

        //Give the streamer a functor to get the send buffer
        //get_tx_buff_with_flowctrl is static so bind has no lifetime issues
        //xport.send (sptr) is required to add streamer->data-transport lifetime dependency
        //task (sptr) is required to add  a streamer->async-handler lifetime dependency
        my_streamer->set_xport_chan_get_buff(
            stream_i,
            boost::bind(&get_tx_buff_with_flowctrl, task, guts, xport.send, fc_window, _1)
        );
        //Give the streamer a functor handled received async messages
        my_streamer->set_async_receiver(
            boost::bind(&async_md_type::pop_with_timed_wait, async_md, _1, _2)
        );
        my_streamer->set_xport_chan_sid(stream_i, true, xport.send_sid);
        my_streamer->set_enable_trailer(false); //TODO not implemented trailer support yet

        //Store a weak pointer to prevent a streamer->x300_impl->streamer circular dependency
        mb.tx_streamers[ce_ctrl->get_block_id().get()] = boost::weak_ptr<sph::send_packet_streamer>(my_streamer);

        //sets all tick and samp rates on this streamer
        const fs_path mb_path = "/mboards/"+boost::lexical_cast<std::string>(mb_index);
        _tree->access<double>(mb_path / "tick_rate").update();
        if (ce_ctrl->get_block_id().get_block_name() == "Radio") {
            // TODO this is specific to radios and thus should be done by radio_ctrl
            UHD_MSG(status) << "This is a radio, thus updating sample rate" << std::endl;
            _tree->access<double>(mb_path / "tx_dsps" / boost::lexical_cast<std::string>(ce_ctrl->get_block_id().get_block_count()) / "rate" / "value").update();
        }
    }

    return my_streamer;
}
// vim: sw=4 expandtab:
