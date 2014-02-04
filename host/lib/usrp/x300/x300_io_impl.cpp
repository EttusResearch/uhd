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

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;

/***********************************************************************
 * update streamer rates
 **********************************************************************/
void x300_impl::update_tick_rate(mboard_members_t &mb, const double rate)
{
    BOOST_FOREACH(const size_t &dspno, mb.rx_streamers.keys())
    {
        boost::shared_ptr<sph::recv_packet_streamer> my_streamer =
            boost::dynamic_pointer_cast<sph::recv_packet_streamer>(mb.rx_streamers[dspno].lock());
        if (my_streamer) my_streamer->set_tick_rate(rate);
    }
    BOOST_FOREACH(const size_t &dspno, mb.tx_streamers.keys())
    {
        boost::shared_ptr<sph::send_packet_streamer> my_streamer =
            boost::dynamic_pointer_cast<sph::send_packet_streamer>(mb.tx_streamers[dspno].lock());
        if (my_streamer) my_streamer->set_tick_rate(rate);
    }
}

void x300_impl::update_rx_samp_rate(mboard_members_t &mb, const size_t dspno, const double rate)
{
    if (not mb.rx_streamers.has_key(dspno)) return;
    boost::shared_ptr<sph::recv_packet_streamer> my_streamer =
        boost::dynamic_pointer_cast<sph::recv_packet_streamer>(mb.rx_streamers[dspno].lock());
    if (not my_streamer) return;
    my_streamer->set_samp_rate(rate);
    const double adj = mb.radio_perifs[dspno].ddc->get_scaling_adjustment();
    my_streamer->set_scale_factor(adj);
}

void x300_impl::update_tx_samp_rate(mboard_members_t &mb, const size_t dspno, const double rate)
{
    if (not mb.tx_streamers.has_key(dspno)) return;
    boost::shared_ptr<sph::send_packet_streamer> my_streamer =
        boost::dynamic_pointer_cast<sph::send_packet_streamer>(mb.tx_streamers[dspno].lock());
    if (not my_streamer) return;
    my_streamer->set_samp_rate(rate);
    const double adj = mb.radio_perifs[dspno].duc->get_scaling_adjustment();
    my_streamer->set_scale_factor(adj);
}

/***********************************************************************
 * Setup dboard muxing for IQ
 **********************************************************************/
void x300_impl::update_rx_subdev_spec(const size_t mb_i, const subdev_spec_t &spec)
{
    const std::string mb_name = boost::lexical_cast<std::string>(mb_i);
    fs_path root = "/mboards/"+mb_name+"/dboards";

    //sanity checking
    validate_subdev_spec(_tree, spec, "rx", mb_name);
    UHD_ASSERT_THROW(spec.size() <= 2);
    if (spec.size() > 0) UHD_ASSERT_THROW(spec[0].db_name == "A");
    if (spec.size() > 1) UHD_ASSERT_THROW(spec[1].db_name == "B");

    //setup mux for this spec
    for (size_t i = 0; i < 2; i++)
    {
        //extract db name
        const std::string db_name = (i == 0)? "A" : "B";
        if (i < spec.size()) UHD_ASSERT_THROW(spec[i].db_name == db_name);

        //extract fe name
        std::string fe_name;
        if (i < spec.size()) fe_name = spec[i].sd_name;
        else fe_name = _tree->list(root / db_name / "rx_frontends").front();

        //extract connection
        const std::string conn = _tree->access<std::string>(root / db_name / "rx_frontends" / fe_name / "connection").get();

        //swap condition
        const bool fe_swapped = (conn == "QI" or conn == "Q");
        _mb[mb_i].radio_perifs[i].ddc->set_mux(conn, fe_swapped);
        //see usrp/io_impl.cpp if multiple DSPs share the frontend:
        _mb[mb_i].radio_perifs[i].rx_fe->set_mux(fe_swapped);
    }

    _mb[mb_i].rx_fe_map = spec;
}

void x300_impl::update_tx_subdev_spec(const size_t mb_i, const subdev_spec_t &spec)
{
    const std::string mb_name = boost::lexical_cast<std::string>(mb_i);
    fs_path root = "/mboards/"+mb_name+"/dboards";

    //sanity checking
    validate_subdev_spec(_tree, spec, "tx", mb_name);
    UHD_ASSERT_THROW(spec.size() <= 2);
    if (spec.size() > 0) UHD_ASSERT_THROW(spec[0].db_name == "A");
    if (spec.size() > 1) UHD_ASSERT_THROW(spec[1].db_name == "B");

    //set the mux for this spec
    for (size_t i = 0; i < 2; i++)
    {
        //extract db name
        const std::string db_name = (i == 0)? "A" : "B";
        if (i < spec.size()) UHD_ASSERT_THROW(spec[i].db_name == db_name);

        //extract fe name
        std::string fe_name;
        if (i < spec.size()) fe_name = spec[i].sd_name;
        else fe_name = _tree->list(root / db_name / "tx_frontends").front();

        //extract connection
        const std::string conn = _tree->access<std::string>(root / db_name / "tx_frontends" / fe_name / "connection").get();

	//swap condition
        _mb[mb_i].radio_perifs[i].tx_fe->set_mux(conn);

    }

    _mb[mb_i].tx_fe_map = spec;
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

static void handle_rx_flowctrl(const boost::uint32_t sid, zero_copy_if::sptr xport, bool big_endian, boost::shared_ptr<boost::uint32_t> seq32_state, const size_t last_seq)
{
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

    //load packet info
    vrt::if_packet_info_t packet_info;
    packet_info.packet_type = vrt::if_packet_info_t::PACKET_TYPE_CONTEXT;
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
    uint32_t (*endian_conv)(uint32_t) = uhd::ntohx;
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

    //catch the flow control packets and react
    if (endian_conv(packet_buff[if_packet_info.num_header_words32+0]) == 0)
    {
        const size_t seq = endian_conv(packet_buff[if_packet_info.num_header_words32+1]);
        guts->seq_queue.push_with_haste(seq);
        return;
    }

    //fill in the async metadata
    async_metadata_t metadata;
    load_metadata_from_buff(
        endian_conv, metadata, if_packet_info, packet_buff,
        clock->get_master_clock_rate(), guts->stream_channel);
    guts->async_queue->push_with_pop_on_full(metadata);
    metadata.channel = guts->device_channel;
    guts->old_async_queue->push_with_pop_on_full(metadata);
    standard_async_msg_prints(metadata);
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
    if (buff) guts->last_seq_out++; //update seq, this will actually be a send
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
 * Receive streamer
 **********************************************************************/
rx_streamer::sptr x300_impl::get_rx_stream(const uhd::stream_args_t &args_)
{
    boost::mutex::scoped_lock lock(_transport_setup_mutex);
    stream_args_t args = args_;

    //setup defaults for unspecified values
    if (not args.otw_format.empty() and args.otw_format != "sc16")
    {
        throw uhd::value_error("x300_impl::get_rx_stream only supports otw_format sc16");
    }
    args.otw_format = "sc16";
    args.channels = args.channels.empty()? std::vector<size_t>(1, 0) : args.channels;

    boost::shared_ptr<sph::recv_packet_streamer> my_streamer;
    for (size_t stream_i = 0; stream_i < args.channels.size(); stream_i++)
    {
        const size_t chan = args.channels[stream_i];
        size_t mb_chan = chan, mb_index = 0;
        BOOST_FOREACH(mboard_members_t &mb, _mb)
        {
            if (mb_chan < mb.rx_fe_map.size()) break;
            else mb_chan -= mb.rx_fe_map.size();
            mb_index++;
        }
        mboard_members_t &mb = _mb[mb_index];
        radio_perifs_t &perif = mb.radio_perifs[mb_chan];

        //setup the dsp transport hints (default to a large recv buff)
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
        uint8_t dest = (mb_chan == 0)? X300_XB_DST_R0 : X300_XB_DST_R1;
        boost::uint32_t data_sid;
        UHD_LOG << "creating rx stream " << device_addr.to_string() << std::endl;
        both_xports_t xport = this->make_transport(mb_index, dest, X300_RADIO_DEST_PREFIX_RX, device_addr, data_sid);
        UHD_LOG << boost::format("data_sid = 0x%08x, actual recv_buff_size = %d\n") % data_sid % xport.recv_buff_size << std::endl;

        //calculate packet size
        static const size_t hdr_size = 0
            + vrt::num_vrl_words32*sizeof(boost::uint32_t)
            + vrt::max_if_hdr_words32*sizeof(boost::uint32_t)
            + sizeof(vrt::if_packet_info_t().tlr) //forced to have trailer
            - sizeof(vrt::if_packet_info_t().cid) //no class id ever used
            - sizeof(vrt::if_packet_info_t().tsi) //no int time ever used
        ;
        const size_t bpp = xport.recv->get_recv_frame_size() - hdr_size;
        const size_t bpi = convert::get_bytes_per_item(args.otw_format);
        const size_t spp = unsigned(args.args.cast<double>("spp", bpp/bpi));

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

        perif.framer->clear();
        perif.framer->set_nsamps_per_packet(spp); //seems to be a good place to set this
        perif.framer->set_sid((data_sid << 16) | (data_sid >> 16));
        perif.framer->setup(args);
        perif.ddc->setup(args);

        //flow control setup
        const size_t fc_window = get_rx_flow_control_window(xport.recv->get_recv_frame_size(), xport.recv_buff_size, device_addr);
        const size_t fc_handle_window = std::max<size_t>(1, fc_window / X300_RX_FC_REQUEST_FREQ);

        UHD_LOG << "RX Flow Control Window = " << fc_window << ", RX Flow Control Handler Window = " << fc_handle_window << std::endl;

        perif.framer->configure_flow_control(fc_window);

        boost::shared_ptr<boost::uint32_t> seq32(new boost::uint32_t(0));
        //Give the streamer a functor to get the recv_buffer
        //bind requires a zero_copy_if::sptr to add a streamer->xport lifetime dependency
        my_streamer->set_xport_chan_get_buff(
            stream_i,
            boost::bind(&zero_copy_if::get_recv_buff, xport.recv, _1),
            true /*flush*/
        );
        //Give the streamer a functor to handle overflows
        //bind requires a weak_ptr to break the a streamer->streamer circular dependency
        //Using "this" is OK because we know that x300_impl will outlive the streamer
        my_streamer->set_overflow_handler(
            stream_i,
            boost::bind(&x300_impl::handle_overflow, this, boost::ref(perif), boost::weak_ptr<uhd::rx_streamer>(my_streamer))
        );
        //Give the streamer a functor to send flow control messages
        //handle_rx_flowctrl is static and has no lifetime issues
        my_streamer->set_xport_handle_flowctrl(
            stream_i, boost::bind(&handle_rx_flowctrl, data_sid, xport.send, mb.if_pkt_is_big_endian, seq32, _1),
            fc_handle_window,
            true/*init*/
        );
        //Give the streamer a functor issue stream cmd
        //bind requires a rx_vita_core_3000::sptr to add a streamer->framer lifetime dependency
        my_streamer->set_issue_stream_cmd(
            stream_i, boost::bind(&rx_vita_core_3000::issue_stream_command, perif.framer, _1)
        );

        //Store a weak pointer to prevent a streamer->x300_impl->streamer circular dependency
        mb.rx_streamers[mb_chan] = boost::weak_ptr<sph::recv_packet_streamer>(my_streamer);

        //sets all tick and samp rates on this streamer
        const fs_path mb_path = "/mboards/"+boost::lexical_cast<std::string>(mb_index);
        _tree->access<double>(mb_path / "tick_rate").update();
        _tree->access<double>(mb_path / "rx_dsps" / boost::lexical_cast<std::string>(mb_chan) / "rate" / "value").update();
    }

    return my_streamer;
}

void x300_impl::handle_overflow(x300_impl::radio_perifs_t &perif, boost::weak_ptr<uhd::rx_streamer> streamer)
{
    boost::shared_ptr<sph::recv_packet_streamer> my_streamer =
            boost::dynamic_pointer_cast<sph::recv_packet_streamer>(streamer.lock());
    if (not my_streamer) return; //If the rx_streamer has expired then overflow handling makes no sense.

    if (my_streamer->get_num_channels() == 1)
    {
        perif.framer->handle_overflow();
        return;
    }

    /////////////////////////////////////////////////////////////
    // MIMO overflow recovery time
    /////////////////////////////////////////////////////////////
    //find out if we were in continuous mode before stopping
    const bool in_continuous_streaming_mode = perif.framer->in_continuous_streaming_mode();
    //stop streaming
    my_streamer->issue_stream_cmd(stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
    //flush transports
    my_streamer->flush_all(0.001);
    //restart streaming
    if (in_continuous_streaming_mode)
    {
        stream_cmd_t stream_cmd(stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
        stream_cmd.stream_now = false;
        stream_cmd.time_spec = perif.time64->get_time_now() + time_spec_t(0.01);
        my_streamer->issue_stream_cmd(stream_cmd);
    }
}

/***********************************************************************
 * Transmit streamer
 **********************************************************************/
tx_streamer::sptr x300_impl::get_tx_stream(const uhd::stream_args_t &args_)
{
    boost::mutex::scoped_lock lock(_transport_setup_mutex);
    stream_args_t args = args_;

    //setup defaults for unspecified values
    if (not args.otw_format.empty() and args.otw_format != "sc16")
    {
        throw uhd::value_error("x300_impl::get_rx_stream only supports otw_format sc16");
    }
    args.otw_format = "sc16";
    args.channels = args.channels.empty()? std::vector<size_t>(1, 0) : args.channels;

    //shared async queue for all channels in streamer
    boost::shared_ptr<async_md_type> async_md(new async_md_type(1000/*messages deep*/));

    boost::shared_ptr<sph::send_packet_streamer> my_streamer;
    for (size_t stream_i = 0; stream_i < args.channels.size(); stream_i++)
    {
        const size_t chan = args.channels[stream_i];
        size_t mb_chan = chan, mb_index = 0;
        BOOST_FOREACH(mboard_members_t &mb, _mb)
        {
            if (mb_chan < mb.tx_fe_map.size()) break;
            else mb_chan -= mb.tx_fe_map.size();
            mb_index++;
        }
        mboard_members_t &mb = _mb[mb_index];
        radio_perifs_t &perif = mb.radio_perifs[mb_chan];

        //setup the dsp transport hints (TODO)
        device_addr_t device_addr = mb.send_args;

        //allocate sid and create transport
        uint8_t dest = (mb_chan == 0)? X300_XB_DST_R0 : X300_XB_DST_R1;
        boost::uint32_t data_sid;
        UHD_LOG << "creating tx stream " << device_addr.to_string() << std::endl;
        both_xports_t xport = this->make_transport(mb_index, dest, X300_RADIO_DEST_PREFIX_TX, device_addr, data_sid);
        UHD_LOG << boost::format("data_sid = 0x%08x\n") % data_sid << std::endl;

        //calculate packet size
        static const size_t hdr_size = 0
            + vrt::num_vrl_words32*sizeof(boost::uint32_t)
            + vrt::max_if_hdr_words32*sizeof(boost::uint32_t)
            //+ sizeof(vrt::if_packet_info_t().tlr) //forced to have trailer
            - sizeof(vrt::if_packet_info_t().cid) //no class id ever used
            - sizeof(vrt::if_packet_info_t().tsi) //no int time ever used
        ;
        const size_t bpp = xport.send->get_send_frame_size() - hdr_size;
        const size_t bpi = convert::get_bytes_per_item(args.otw_format);
        const size_t spp = unsigned(args.args.cast<double>("spp", bpp/bpi));

        //make the new streamer given the samples per packet
        if (not my_streamer) my_streamer = boost::make_shared<sph::send_packet_streamer>(spp);
        my_streamer->resize(args.channels.size());

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

        perif.deframer->clear();
        perif.deframer->setup(args);
        perif.duc->setup(args);

        //flow control setup
        size_t fc_window = get_tx_flow_control_window(xport.send->get_send_frame_size(), device_addr);  //In packets
        const size_t fc_handle_window = std::max<size_t>(1, fc_window/X300_TX_FC_RESPONSE_FREQ);

        UHD_LOG << "TX Flow Control Window = " << fc_window << ", TX Flow Control Handler Window = " << fc_handle_window << std::endl;

        perif.deframer->configure_flow_control(0/*cycs off*/, fc_handle_window);
        boost::shared_ptr<x300_tx_fc_guts_t> guts(new x300_tx_fc_guts_t());
        guts->stream_channel = stream_i;
        guts->device_channel = chan;
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
        my_streamer->set_xport_chan_sid(stream_i, true, data_sid);
        my_streamer->set_enable_trailer(false); //TODO not implemented trailer support yet

        //Store a weak pointer to prevent a streamer->x300_impl->streamer circular dependency
        mb.tx_streamers[mb_chan] = boost::weak_ptr<sph::send_packet_streamer>(my_streamer);

        //sets all tick and samp rates on this streamer
        const fs_path mb_path = "/mboards/"+boost::lexical_cast<std::string>(mb_index);
        _tree->access<double>(mb_path / "tick_rate").update();
        _tree->access<double>(mb_path / "tx_dsps" / boost::lexical_cast<std::string>(mb_chan) / "rate" / "value").update();
    }

    return my_streamer;
}
