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

#include "e300_regs.hpp"
#include "e300_impl.hpp"
#include "e300_fpga_defs.hpp"
#include "validate_subdev_spec.hpp"
#include "../../transport/super_recv_packet_handler.hpp"
#include "../../transport/super_send_packet_handler.hpp"
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

namespace uhd { namespace usrp { namespace e300 {

static const boost::uint32_t HW_SEQ_NUM_MASK = 0xfff;

/***********************************************************************
 * update streamer rates
 **********************************************************************/
void e300_impl::_check_tick_rate_with_current_streamers(const double rate)
{
    size_t max_tx_chan_count = 0, max_rx_chan_count = 0;
    BOOST_FOREACH(radio_perifs_t &perif, _radio_perifs)
    {
        {
            boost::shared_ptr<sph::recv_packet_streamer> rx_streamer =
                boost::dynamic_pointer_cast<sph::recv_packet_streamer>(
                    perif.rx_streamer.lock());
            if (rx_streamer)
                max_rx_chan_count = std::max(
                    max_rx_chan_count,
                    rx_streamer->get_num_channels());
        }

        {
            boost::shared_ptr<sph::send_packet_streamer> tx_streamer =
                boost::dynamic_pointer_cast<sph::send_packet_streamer>(
                    perif.tx_streamer.lock());
            if (tx_streamer)
                max_tx_chan_count = std::max(
                    max_tx_chan_count,
                    tx_streamer->get_num_channels());
        }
    }
    _enforce_tick_rate_limits(max_rx_chan_count, rate, "RX");
    _enforce_tick_rate_limits(max_tx_chan_count, rate, "TX");
}

void e300_impl::_update_tick_rate(const double rate)
{
    _check_tick_rate_with_current_streamers(rate);

    BOOST_FOREACH(radio_perifs_t &perif, _radio_perifs)
    {
        boost::shared_ptr<sph::recv_packet_streamer> my_streamer =
            boost::dynamic_pointer_cast<sph::recv_packet_streamer>(perif.rx_streamer.lock());
        if (my_streamer)
            my_streamer->set_tick_rate(rate);
        perif.framer->set_tick_rate(_tick_rate);
    }
    BOOST_FOREACH(radio_perifs_t &perif, _radio_perifs)
    {
        boost::shared_ptr<sph::send_packet_streamer> my_streamer =
            boost::dynamic_pointer_cast<sph::send_packet_streamer>(perif.tx_streamer.lock());
        if (my_streamer)
            my_streamer->set_tick_rate(rate);
        perif.deframer->set_tick_rate(_tick_rate);
    }
}

void e300_impl::_update_rx_samp_rate(const size_t dspno, const double rate)
{
    boost::shared_ptr<sph::recv_packet_streamer> my_streamer =
        boost::dynamic_pointer_cast<sph::recv_packet_streamer>(_radio_perifs[dspno].rx_streamer.lock());
    if (my_streamer)
        my_streamer->set_samp_rate(rate);
    _codec_mgr->check_bandwidth(rate, "Rx");
}

void e300_impl::_update_tx_samp_rate(const size_t dspno, const double rate)
{
    boost::shared_ptr<sph::send_packet_streamer> my_streamer =
        boost::dynamic_pointer_cast<sph::send_packet_streamer>(_radio_perifs[dspno].tx_streamer.lock());
    if (my_streamer)
        my_streamer->set_samp_rate(rate);
    _codec_mgr->check_bandwidth(rate, "Tx");
}

/***********************************************************************
 * frontend selection
 **********************************************************************/
void e300_impl::_update_subdev_spec(
        const std::string &txrx,
        const uhd::usrp::subdev_spec_t &spec)
{
    //sanity checking
    if (spec.size())
        validate_subdev_spec(_tree, spec, "rx");

    UHD_ASSERT_THROW(spec.size() <= fpga::NUM_RADIOS);

    if (spec.size() >= 1)
    {
        UHD_ASSERT_THROW(spec[0].db_name == "A");
        UHD_ASSERT_THROW(spec[0].sd_name == "A" or spec[0].sd_name == "B");
    }
    if (spec.size() == 2)
    {
        UHD_ASSERT_THROW(spec[1].db_name == "A");
        UHD_ASSERT_THROW(
            (spec[0].sd_name == "A" and spec[1].sd_name == "B") or
            (spec[0].sd_name == "B" and spec[1].sd_name == "A")
        );
    }

    std::vector<size_t> chan_to_dsp_map(spec.size(), 0);
    for (size_t i = 0; i < spec.size(); i++)
        chan_to_dsp_map[i] = (spec[i].sd_name == "A") ? 0 : 1;
    _tree->access<std::vector<size_t> >("/mboards/0" / (txrx + "_chan_dsp_mapping")).set(chan_to_dsp_map);

    const fs_path mb_path = "/mboards/0";

    if (txrx == "tx") {
        for (size_t i = 0; i < spec.size(); i++)
        {
            const std::string conn = _tree->access<std::string>(
                mb_path / "dboards" / spec[i].db_name /
                ("tx_frontends") / spec[i].sd_name / "connection").get();
            _radio_perifs[i].tx_fe->set_mux(conn);
        }

    } else {
        for (size_t i = 0; i < spec.size(); i++)
        {
            const std::string conn = _tree->access<std::string>(
                mb_path / "dboards" / spec[i].db_name /
                ("rx_frontends") / spec[i].sd_name / "connection").get();

            const bool fe_swapped = (conn == "QI" or conn == "Q");
            _radio_perifs[i].ddc->set_mux(conn, fe_swapped);
            _radio_perifs[i].rx_fe->set_mux(fe_swapped);
        }
    }

    this->_update_enables();
}

/***********************************************************************
 * VITA stuff
 **********************************************************************/
static void e300_if_hdr_unpack_le(
    const boost::uint32_t *packet_buff,
    vrt::if_packet_info_t &if_packet_info
){
    if_packet_info.link_type = vrt::if_packet_info_t::LINK_TYPE_CHDR;
    return vrt::if_hdr_unpack_le(packet_buff, if_packet_info);
}

static void e300_if_hdr_pack_le(
    boost::uint32_t *packet_buff,
    vrt::if_packet_info_t &if_packet_info
){
    if_packet_info.link_type = vrt::if_packet_info_t::LINK_TYPE_CHDR;
    return vrt::if_hdr_pack_le(packet_buff, if_packet_info);
}

/***********************************************************************
 * RX flow control handler
 **********************************************************************/
struct e300_rx_fc_cache_t
{
    e300_rx_fc_cache_t():
        last_seq_in(0){}
    size_t last_seq_in;
};

void e300_impl::_handle_overflow(
    radio_perifs_t &perif,
    boost::weak_ptr<uhd::rx_streamer> streamer)
{
    boost::shared_ptr<sph::recv_packet_streamer> my_streamer =
            boost::dynamic_pointer_cast<sph::recv_packet_streamer>(streamer.lock());

    //If the rx_streamer has expired then overflow handling makes no sense.
    if (not my_streamer)
        return;

    if (my_streamer->get_num_channels() == 1) {
        perif.framer->handle_overflow();
        return;
    }

    // MIMO overflow recovery time
    // find out if we were in continuous mode before stopping
    const bool in_continuous_streaming_mode = perif.framer->in_continuous_streaming_mode();
    // stop streaming
    my_streamer->issue_stream_cmd(stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
    // flush transports
    my_streamer->flush_all(0.001);
    // restart streaming
    if (in_continuous_streaming_mode) {
        stream_cmd_t stream_cmd(stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
        stream_cmd.stream_now = false;
        stream_cmd.time_spec = perif.time64->get_time_now() + time_spec_t(0.01);
        my_streamer->issue_stream_cmd(stream_cmd);
    }
}

static size_t get_rx_flow_control_window(size_t frame_size, size_t sw_buff_size, double fullness_factor)
{
    if (fullness_factor < 0.01 || fullness_factor > 1) {
        throw uhd::value_error("recv_buff_fullness must be between 0.01 and 1 inclusive (1% to 100%)");
    }

    size_t window_in_pkts = (static_cast<size_t>(sw_buff_size * fullness_factor) / frame_size);
    if (window_in_pkts == 0) {
        throw uhd::value_error("recv_buff_size must be larger than the recv_frame_size.");
    }
    return window_in_pkts;
}

static void handle_rx_flowctrl(
    const boost::uint32_t sid,
    zero_copy_if::sptr xport,
    boost::shared_ptr<e300_rx_fc_cache_t> fc_cache,
    const size_t last_seq)
{
    static const size_t RXFC_PACKET_LEN_IN_WORDS    = 2;
    static const size_t RXFC_CMD_CODE_OFFSET        = 0;
    static const size_t RXFC_SEQ_NUM_OFFSET         = 1;

    managed_send_buffer::sptr buff = xport->get_send_buff(1.0);
    if (not buff)
    {
        throw uhd::runtime_error("handle_rx_flowctrl timed out getting a send buffer");
    }
    boost::uint32_t *pkt = buff->cast<boost::uint32_t *>();

    //recover seq32
    size_t& seq_sw = fc_cache->last_seq_in;
    const size_t seq_hw = seq_sw & HW_SEQ_NUM_MASK;
    if (last_seq < seq_hw)
        seq_sw += (HW_SEQ_NUM_MASK + 1);
    seq_sw &= ~HW_SEQ_NUM_MASK;
    seq_sw |= last_seq;

    //load packet info
    vrt::if_packet_info_t packet_info;
    packet_info.packet_type = vrt::if_packet_info_t::PACKET_TYPE_CONTEXT;
    packet_info.num_payload_words32 = RXFC_PACKET_LEN_IN_WORDS;
    packet_info.num_payload_bytes = packet_info.num_payload_words32*sizeof(boost::uint32_t);
    packet_info.packet_count = seq_sw;
    packet_info.sob = false;
    packet_info.eob = false;
    packet_info.sid = sid;
    packet_info.has_sid = true;
    packet_info.has_cid = false;
    packet_info.has_tsi = false;
    packet_info.has_tsf = false;
    packet_info.has_tlr = false;

    //load header
    e300_if_hdr_pack_le(pkt, packet_info);

    //load payload
    pkt[packet_info.num_header_words32+RXFC_CMD_CODE_OFFSET] = uhd::htowx<boost::uint32_t>(0);
    pkt[packet_info.num_header_words32+RXFC_SEQ_NUM_OFFSET] = uhd::htowx<boost::uint32_t>(seq_sw);

    //send the buffer over the interface
    buff->commit(sizeof(boost::uint32_t)*(packet_info.num_packet_words32));
}


/***********************************************************************
 * TX flow control handler
 **********************************************************************/
struct e300_tx_fc_cache_t
{
    e300_tx_fc_cache_t(void):
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
    boost::shared_ptr<e300_impl::async_md_type> async_queue;
    boost::shared_ptr<e300_impl::async_md_type> old_async_queue;
};

#define E300_ASYNC_EVENT_CODE_FLOW_CTRL 0

typedef boost::function<double(void)> tick_rate_retriever_t;


static void handle_tx_async_msgs(boost::shared_ptr<e300_tx_fc_cache_t> fc_cache,
                                 zero_copy_if::sptr xport,
                                 boost::function<double(void)> get_tick_rate)
{
    managed_recv_buffer::sptr buff = xport->get_recv_buff();
    if (not buff)
        return;

    //extract packet info
    vrt::if_packet_info_t if_packet_info;
    if_packet_info.num_packet_words32 = buff->size()/sizeof(boost::uint32_t);
    const boost::uint32_t *packet_buff = buff->cast<const boost::uint32_t *>();

    //unpacking can fail
    try
    {
        e300_if_hdr_unpack_le(packet_buff, if_packet_info);
    }
    catch(const std::exception &ex)
    {
        UHD_MSG(error) << "Error parsing async message packet: " << ex.what() << std::endl;
        return;
    }

    //catch the flow control packets and react
    if (uhd::wtohx(packet_buff[if_packet_info.num_header_words32+0]) == 0)
    {
        const size_t seq = uhd::wtohx(packet_buff[if_packet_info.num_header_words32+1]);
        fc_cache->seq_queue.push_with_haste(seq);
        return;
    }

    //fill in the async metadata
    async_metadata_t metadata;
    load_metadata_from_buff(uhd::wtohx<boost::uint32_t>,
                            metadata, if_packet_info, packet_buff,
                            get_tick_rate(), fc_cache->stream_channel);

    //The FC response and the burst ack are two indicators that the radio
    //consumed packets. Use them to update the FC metadata
    if (metadata.event_code == E300_ASYNC_EVENT_CODE_FLOW_CTRL or
        metadata.event_code == async_metadata_t::EVENT_CODE_BURST_ACK
    ) {
        const size_t seq = metadata.user_payload[0];
        fc_cache->seq_queue.push_with_pop_on_full(seq);
    }

    //FC responses don't propagate up to the user so filter them here
    if (metadata.event_code != E300_ASYNC_EVENT_CODE_FLOW_CTRL) {
        fc_cache->async_queue->push_with_pop_on_full(metadata);
        metadata.channel = fc_cache->device_channel;
        fc_cache->old_async_queue->push_with_pop_on_full(metadata);
        standard_async_msg_prints(metadata);
    }
}

static managed_send_buffer::sptr get_tx_buff_with_flowctrl(
    task::sptr /*holds ref*/,
    boost::shared_ptr<e300_tx_fc_cache_t> fc_cache,
    zero_copy_if::sptr xport,
    const size_t fc_window,
    const double timeout
){
    while (true)
    {
        const size_t delta = (fc_cache->last_seq_out & HW_SEQ_NUM_MASK) - (fc_cache->last_seq_ack & HW_SEQ_NUM_MASK);
        if ((delta & HW_SEQ_NUM_MASK) <= fc_window)
            break;

        const bool ok = fc_cache->seq_queue.pop_with_timed_wait(fc_cache->last_seq_ack, timeout);
        if (not ok)
            return managed_send_buffer::sptr(); //timeout waiting for flow control
    }

    managed_send_buffer::sptr buff = xport->get_send_buff(timeout);
    if (buff) {
        fc_cache->last_seq_out++; //update seq, this will actually be a send
    }

    return buff;
}

/***********************************************************************
 * Async Data
 **********************************************************************/
bool e300_impl::recv_async_msg(
    async_metadata_t &async_metadata, double timeout
)
{
    return _async_md->pop_with_timed_wait(async_metadata, timeout);
}

/***********************************************************************
 * Receive streamer
 **********************************************************************/
rx_streamer::sptr e300_impl::get_rx_stream(const uhd::stream_args_t &args_)
{
    boost::mutex::scoped_lock lock(_stream_spawn_mutex);
    stream_args_t args = args_;

    //setup defaults for unspecified values
    if (not args.otw_format.empty() and args.otw_format != "sc16")
    {
        throw uhd::value_error("e300_impl::get_rx_stream only supports otw_format sc16");
    }
    args.otw_format = "sc16";
    args.channels = args.channels.empty()? std::vector<size_t>(1, 0) : args.channels;

    boost::shared_ptr<sph::recv_packet_streamer> my_streamer;
    for (size_t stream_i = 0; stream_i < args.channels.size(); stream_i++)
    {

        const size_t radio_index = _tree->access<std::vector<size_t> >("/mboards/0/rx_chan_dsp_mapping")
                                       .get().at(args.channels[stream_i]);

        radio_perifs_t &perif = _radio_perifs[radio_index];

        // make a transport, grab a sid
        boost::uint32_t data_sid;
        both_xports_t data_xports = _make_transport(
           radio_index ? E300_XB_DST_R1 : E300_XB_DST_R0,
           E300_RADIO_DEST_PREFIX_RX,
           _data_xport_params,
           data_sid);

        //calculate packet size
        static const size_t hdr_size = 0
            + vrt::num_vrl_words32*sizeof(boost::uint32_t)
            + vrt::max_if_hdr_words32*sizeof(boost::uint32_t)
            + sizeof(vrt::if_packet_info_t().tlr) //forced to have trailer
            - sizeof(vrt::if_packet_info_t().cid) //no class id ever used
            - sizeof(vrt::if_packet_info_t().tsi) //no int time ever used
        ;
        const size_t bpp = data_xports.recv->get_recv_frame_size() - hdr_size;
        const size_t bpi = convert::get_bytes_per_item(args.otw_format);
        const size_t spp = unsigned(args.args.cast<double>("spp", bpp/bpi));

        //make the new streamer given the samples per packet
        if (not my_streamer)
            my_streamer = boost::make_shared<sph::recv_packet_streamer>(spp);
        my_streamer->resize(args.channels.size());

        //init some streamer stuff
        my_streamer->set_vrt_unpacker(&e300_if_hdr_unpack_le);

        //set the converter
        uhd::convert::id_type id;
        id.input_format = args.otw_format + "_item32_le";
        id.num_inputs = 1;
        id.output_format = args.cpu_format;
        id.num_outputs = 1;
        my_streamer->set_converter(id);

        perif.framer->clear();
        perif.framer->set_nsamps_per_packet(spp); //seems to be a good place to set this
        perif.framer->set_sid((data_sid << 16) | (data_sid >> 16));
        perif.framer->setup(args);
        perif.ddc->setup(args);

        // flow control setup
        const size_t frame_size = data_xports.recv->get_recv_frame_size();
        const size_t num_frames = data_xports.recv->get_num_recv_frames();
        const size_t fc_window = get_rx_flow_control_window(
            frame_size,num_frames * frame_size,
            E300_RX_SW_BUFF_FULLNESS);
        const size_t fc_handle_window = std::max<size_t>(1, fc_window / E300_RX_FC_REQUEST_FREQ);

        UHD_LOG << "RX Flow Control Window = " << fc_window
                << ", RX Flow Control Handler Window = "
                << fc_handle_window << std::endl;

        perif.framer->configure_flow_control(fc_window);
        boost::shared_ptr<e300_rx_fc_cache_t> fc_cache(new e300_rx_fc_cache_t());

        my_streamer->set_xport_chan_get_buff(stream_i, boost::bind(
            &zero_copy_if::get_recv_buff, data_xports.recv, _1
        ), true /*flush*/);
        my_streamer->set_overflow_handler(stream_i,
            boost::bind(&e300_impl::_handle_overflow, this, boost::ref(perif),
            boost::weak_ptr<uhd::rx_streamer>(my_streamer))
        );

        my_streamer->set_xport_handle_flowctrl(stream_i,
            boost::bind(&handle_rx_flowctrl, data_sid, data_xports.send, fc_cache, _1),
            fc_handle_window, true/*init*/);

        my_streamer->set_issue_stream_cmd(stream_i,
            boost::bind(&rx_vita_core_3000::issue_stream_command, perif.framer, _1)
        );
        perif.rx_streamer = my_streamer; //store weak pointer

        //sets all tick and samp rates on this streamer
        this->_update_tick_rate(this->_get_tick_rate());
        _tree->access<double>(str(boost::format("/mboards/0/rx_dsps/%u/rate/value") % radio_index)).update();

    }
    _update_enables();
    return my_streamer;
}

/***********************************************************************
 * Transmit streamer
 **********************************************************************/
tx_streamer::sptr e300_impl::get_tx_stream(const uhd::stream_args_t &args_)
{
    boost::mutex::scoped_lock lock(_stream_spawn_mutex);
    stream_args_t args = args_;

    //setup defaults for unspecified values
    if (not args.otw_format.empty() and args.otw_format != "sc16")
    {
        throw uhd::value_error("e300_impl::get_tx_stream only supports otw_format sc16");
    }
    args.otw_format = "sc16";
    args.channels = args.channels.empty()? std::vector<size_t>(1, 0) : args.channels;


    //shared async queue for all channels in streamer
    boost::shared_ptr<async_md_type> async_md(new async_md_type(1000/*messages deep*/));

    boost::shared_ptr<sph::send_packet_streamer> my_streamer;

    for (size_t stream_i = 0; stream_i < args.channels.size(); stream_i++)
    {
        const size_t radio_index = _tree->access<std::vector<size_t> >("/mboards/0/tx_chan_dsp_mapping")
                                       .get().at(args.channels[stream_i]);


        radio_perifs_t &perif = _radio_perifs[radio_index];


        // make a transport, grab a sid
        boost::uint32_t data_sid;
        both_xports_t data_xports = _make_transport(
           radio_index ? E300_XB_DST_R1 : E300_XB_DST_R0,
           E300_RADIO_DEST_PREFIX_TX,
           _data_xport_params,
           data_sid);

        //calculate packet size
        static const size_t hdr_size = 0
            + vrt::num_vrl_words32*sizeof(boost::uint32_t)
            + vrt::max_if_hdr_words32*sizeof(boost::uint32_t)
            + sizeof(vrt::if_packet_info_t().tlr) //forced to have trailer
            - sizeof(vrt::if_packet_info_t().cid) //no class id ever used
            - sizeof(vrt::if_packet_info_t().tsi) //no int time ever used
        ;
        const size_t bpp = data_xports.send->get_send_frame_size() - hdr_size;
        const size_t bpi = convert::get_bytes_per_item(args.otw_format);
        const size_t spp = unsigned(args.args.cast<double>("spp", bpp/bpi));

        //make the new streamer given the samples per packet
        if (not my_streamer)
            my_streamer = boost::make_shared<sph::send_packet_streamer>(spp);
        my_streamer->resize(args.channels.size());

        //init some streamer stuff
        my_streamer->set_vrt_packer(&e300_if_hdr_pack_le);

        //set the converter
        uhd::convert::id_type id;
        id.input_format = args.cpu_format;
        id.num_inputs = 1;
        id.output_format = args.otw_format + "_item32_le";
        id.num_outputs = 1;
        my_streamer->set_converter(id);

        perif.deframer->clear();
        perif.deframer->setup(args);
        perif.duc->setup(args);

        //flow control setup
        const size_t fc_window = data_xports.send->get_num_send_frames();
        const size_t fc_handle_window = std::max<size_t>(1, fc_window/E300_TX_FC_RESPONSE_FREQ);

        UHD_LOG << "TX Flow Control Window = " << fc_window
                << ", TX Flow Control Handler Window = "
                << fc_handle_window << std::endl;

        perif.deframer->configure_flow_control(0/*cycs off*/, fc_handle_window/*pkts*/);
        boost::shared_ptr<e300_tx_fc_cache_t> fc_cache(new e300_tx_fc_cache_t());
        fc_cache->stream_channel = stream_i;
        fc_cache->device_channel = args.channels[stream_i];
        fc_cache->async_queue = async_md;
        fc_cache->old_async_queue = _async_md;

        tick_rate_retriever_t get_tick_rate_fn = boost::bind(&e300_impl::_get_tick_rate, this);

        task::sptr task = task::make(boost::bind(&handle_tx_async_msgs,
                                                 fc_cache, data_xports.recv,
                                                 get_tick_rate_fn));

        my_streamer->set_xport_chan_get_buff(
            stream_i,
            boost::bind(&get_tx_buff_with_flowctrl, task, fc_cache, data_xports.send, fc_window, _1)
        );

        my_streamer->set_async_receiver(
            boost::bind(&async_md_type::pop_with_timed_wait, async_md, _1, _2)
        );
        my_streamer->set_xport_chan_sid(stream_i, true, data_sid);
        my_streamer->set_enable_trailer(false); //TODO not implemented trailer support yet
        perif.tx_streamer = my_streamer; //store weak pointer

        //sets all tick and samp rates on this streamer
        this->_update_tick_rate(this->_get_tick_rate());
        _tree->access<double>(str(boost::format("/mboards/0/tx_dsps/%u/rate/value") % radio_index)).update();
    }
    _update_enables();
    return my_streamer;
}
}}} // namespace
