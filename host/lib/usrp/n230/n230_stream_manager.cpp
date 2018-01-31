//
// Copyright 2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "n230_stream_manager.hpp"

#include "../../transport/super_recv_packet_handler.hpp"
#include "../../transport/super_send_packet_handler.hpp"
#include <uhdlib/usrp/common/async_packet_handler.hpp>
#include <uhd/transport/bounded_buffer.hpp>
#include <uhd/utils/tasks.hpp>
#include <uhd/utils/log.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

static const double N230_RX_SW_BUFF_FULL_FACTOR   = 0.90;     //Buffer should ideally be 90% full.
static const size_t N230_RX_FC_REQUEST_FREQ       = 32;       //per flow-control window
static const size_t N230_TX_MAX_ASYNC_MESSAGES    = 1000;
static const size_t N230_TX_MAX_SPP               = 4092;
static const size_t N230_TX_FC_RESPONSE_FREQ      = 10;       //per flow-control window

static const uint32_t N230_EVENT_CODE_FLOW_CTRL = 0;

namespace uhd { namespace usrp { namespace n230 {

using namespace uhd::transport;

n230_stream_manager::~n230_stream_manager()
{
}

/***********************************************************************
 * Receive streamer
 **********************************************************************/
n230_stream_manager::n230_stream_manager(
    const n230_device_args_t& dev_args,
    boost::shared_ptr<n230_resource_manager> resource_mgr,
    boost::weak_ptr<property_tree> prop_tree
) :
    _dev_args(dev_args),
    _resource_mgr(resource_mgr),
    _tree(prop_tree)
{
    _async_md_queue.reset(new async_md_queue_t(N230_TX_MAX_ASYNC_MESSAGES));
}

/***********************************************************************
 * Receive streamer
 **********************************************************************/
rx_streamer::sptr n230_stream_manager::get_rx_stream(const uhd::stream_args_t &args_)
{
    boost::mutex::scoped_lock lock(_stream_setup_mutex);

    stream_args_t args = args_;

    //setup defaults for unspecified values
    if (args.otw_format.empty()) args.otw_format = "sc16";
    args.channels = args.channels.empty()? std::vector<size_t>(1, 0) : args.channels;

    boost::shared_ptr<sph::recv_packet_streamer> my_streamer;
    for (size_t stream_i = 0; stream_i < args.channels.size(); stream_i++)
    {
        const size_t chan = args.channels[stream_i];
        radio_resource_t& perif = _resource_mgr->get_radio(chan);

        //setup transport hints (default to a large recv buff)
        //TODO: Propagate the device_args class into streamer in the future
        device_addr_t device_addr = args.args;
        if (not device_addr.has_key("recv_buff_size")) {
            device_addr["recv_buff_size"] = std::to_string(_dev_args.get_recv_buff_size());
        }
        if (not device_addr.has_key("recv_frame_size")) {
            device_addr["recv_frame_size"] = std::to_string(_dev_args.get_recv_frame_size());
        }
        if (not device_addr.has_key("num_recv_frames")) {
            device_addr["num_recv_frames"] = std::to_string(_dev_args.get_num_recv_frames());
        }

        transport::udp_zero_copy::buff_params buff_params_out;
        sid_t sid;
        zero_copy_if::sptr xport = _resource_mgr->create_transport(
            RX_DATA, chan, device_addr, sid, buff_params_out);

        //calculate packet size
        static const size_t hdr_size = 0
            + vrt::max_if_hdr_words32*sizeof(uint32_t)
            //+ sizeof(vrt::if_packet_info_t().tlr) //no longer using trailer
            - sizeof(vrt::if_packet_info_t().cid) //no class id ever used
            - sizeof(vrt::if_packet_info_t().tsi) //no int time ever used
        ;
        const size_t bpp = xport->get_recv_frame_size() - hdr_size;
        const size_t bpi = convert::get_bytes_per_item(args.otw_format);
        size_t spp = unsigned(args.args.cast<double>("spp", bpp/bpi));
        spp = std::min<size_t>(N230_TX_MAX_SPP, spp); //FPGA FIFO maximum for framing at full rate

        //make the new streamer given the samples per packet
        if (not my_streamer) my_streamer = boost::make_shared<sph::recv_packet_streamer>(spp);
        my_streamer->resize(args.channels.size());

        //init some streamer stuff
        my_streamer->set_vrt_unpacker(&n230_stream_manager::_cvita_hdr_unpack);

        //set the converter
        uhd::convert::id_type id;
        id.input_format = args.otw_format + "_item32_be";
        id.num_inputs = 1;
        id.output_format = args.cpu_format;
        id.num_outputs = 1;
        my_streamer->set_converter(id);

        perif.framer->clear();
        perif.framer->set_nsamps_per_packet(spp);
        perif.framer->set_sid(sid.reversed().get());
        perif.framer->setup(args);
        perif.ddc->setup(args);

        //Give the streamer a functor to get the recv_buffer
        //bind requires a zero_copy_if::sptr to add a streamer->xport lifetime dependency
        my_streamer->set_xport_chan_get_buff(
            stream_i,
            boost::bind(&zero_copy_if::get_recv_buff, xport, _1),
            true /*flush*/
        );

        my_streamer->set_overflow_handler(stream_i, boost::bind(
            &n230_stream_manager::_handle_overflow, this, chan
        ));

        my_streamer->set_issue_stream_cmd(stream_i, boost::bind(
            &rx_vita_core_3000::issue_stream_command, perif.framer, _1
        ));

        const size_t fc_window = _get_rx_flow_control_window(
            xport->get_recv_frame_size(), buff_params_out.recv_buff_size);
        const size_t fc_handle_window = std::max<size_t>(1, fc_window / N230_RX_FC_REQUEST_FREQ);

        perif.framer->configure_flow_control(fc_window);

        //Give the streamer a functor to send flow control messages
        //handle_rx_flowctrl is static and has no lifetime issues
        boost::shared_ptr<rx_fc_cache_t> fc_cache(new rx_fc_cache_t());
        my_streamer->set_xport_handle_flowctrl(
            stream_i, boost::bind(&n230_stream_manager::_handle_rx_flowctrl, sid.get(), xport, fc_cache, _1),
            fc_handle_window,
            true/*init*/
        );

        //Store a weak pointer to prevent a streamer->manager->streamer circular dependency
        _rx_streamers[chan] = my_streamer; //store weak pointer
        _rx_stream_cached_args[chan] = args;

        //Sets tick and samp rates on all streamer
        update_tick_rate(_get_tick_rate());

        //TODO: Find a way to remove this dependency
        property_tree::sptr prop_tree = _tree.lock();
        if (prop_tree) {
            //TODO: Update this to support multiple motherboards
            const fs_path mb_path = "/mboards/0";
            prop_tree->access<double>(mb_path / "rx_dsps" / std::to_string(chan) / "rate" / "value").update();
        }
    }
    update_stream_states();

    return my_streamer;
}

/***********************************************************************
 * Transmit streamer
 **********************************************************************/
tx_streamer::sptr n230_stream_manager::get_tx_stream(const uhd::stream_args_t &args_)
{
    boost::mutex::scoped_lock lock(_stream_setup_mutex);

    uhd::stream_args_t args = args_;

    //setup defaults for unspecified values
    if (not args.otw_format.empty() and args.otw_format != "sc16") {
        throw uhd::value_error("n230_impl::get_tx_stream only supports otw_format sc16");
    }
    args.otw_format = "sc16";
    args.channels = args.channels.empty()? std::vector<size_t>(1, 0) : args.channels;

    //shared async queue for all channels in streamer
    boost::shared_ptr<async_md_queue_t> async_md(new async_md_queue_t(N230_TX_MAX_ASYNC_MESSAGES));

    boost::shared_ptr<sph::send_packet_streamer> my_streamer;
    for (size_t stream_i = 0; stream_i < args.channels.size(); stream_i++)
    {
        const size_t chan = args.channels[stream_i];
        radio_resource_t& perif = _resource_mgr->get_radio(chan);

        //setup transport hints (default to a large recv buff)
        //TODO: Propagate the device_args class into streamer in the future
        device_addr_t device_addr = args.args;
        if (not device_addr.has_key("send_buff_size")) {
            device_addr["send_buff_size"] = std::to_string(_dev_args.get_send_buff_size());
        }
        if (not device_addr.has_key("send_frame_size")) {
            device_addr["send_frame_size"] = std::to_string(_dev_args.get_send_frame_size());
        }
        if (not device_addr.has_key("num_send_frames")) {
            device_addr["num_send_frames"] = std::to_string(_dev_args.get_num_send_frames());
        }

        transport::udp_zero_copy::buff_params buff_params_out;
        sid_t sid;
        zero_copy_if::sptr xport = _resource_mgr->create_transport(
            TX_DATA, chan, device_addr, sid, buff_params_out);

        //calculate packet size
        static const size_t hdr_size = 0
            + vrt::num_vrl_words32*sizeof(uint32_t)
            + vrt::max_if_hdr_words32*sizeof(uint32_t)
            //+ sizeof(vrt::if_packet_info_t().tlr) //forced to have trailer
            - sizeof(vrt::if_packet_info_t().cid) //no class id ever used
            - sizeof(vrt::if_packet_info_t().tsi) //no int time ever used
        ;
        const size_t bpp = xport->get_send_frame_size() - hdr_size;
        const size_t bpi = convert::get_bytes_per_item(args.otw_format);
        const size_t spp = unsigned(args.args.cast<double>("spp", bpp/bpi));

        //make the new streamer given the samples per packet
        if (not my_streamer) my_streamer = boost::make_shared<sph::send_packet_streamer>(spp);
        my_streamer->resize(args.channels.size());
        my_streamer->set_vrt_packer(&n230_stream_manager::_cvita_hdr_pack);

        //set the converter
        uhd::convert::id_type id;
        id.input_format = args.cpu_format;
        id.num_inputs = 1;
        id.output_format = args.otw_format + "_item32_be";
        id.num_outputs = 1;
        my_streamer->set_converter(id);

        perif.deframer->clear();
        perif.deframer->setup(args);
        perif.duc->setup(args);

        //flow control setup
        size_t fc_window = _get_tx_flow_control_window(
            bpp, device_addr.cast<size_t>("send_buff_size", _dev_args.get_send_buff_size()));
        //In packets
        const size_t fc_handle_window = (fc_window / N230_TX_FC_RESPONSE_FREQ);

        perif.deframer->configure_flow_control(0/*cycs off*/, fc_handle_window);
        boost::shared_ptr<tx_fc_cache_t> fc_cache(new tx_fc_cache_t());
        fc_cache->stream_channel = stream_i;
        fc_cache->device_channel = chan;
        fc_cache->async_queue = async_md;
        fc_cache->old_async_queue = _async_md_queue;

        tick_rate_retriever_t get_tick_rate_fn = boost::bind(&n230_stream_manager::_get_tick_rate, this);
        task::sptr task = task::make(
            boost::bind(&n230_stream_manager::_handle_tx_async_msgs,
                fc_cache, xport, get_tick_rate_fn));

        //Give the streamer a functor to get the send buffer
        //get_tx_buff_with_flowctrl is static so bind has no lifetime issues
        //xport.send (sptr) is required to add streamer->data-transport lifetime dependency
        //task (sptr) is required to add  a streamer->async-handler lifetime dependency
        my_streamer->set_xport_chan_get_buff(
            stream_i,
            boost::bind(&n230_stream_manager::_get_tx_buff_with_flowctrl, task, fc_cache, xport, fc_window, _1)
        );
        //Give the streamer a functor handled received async messages
        my_streamer->set_async_receiver(
            boost::bind(&async_md_queue_t::pop_with_timed_wait, async_md, _1, _2)
        );
        my_streamer->set_xport_chan_sid(stream_i, true, sid.get());
        my_streamer->set_enable_trailer(false); //TODO not implemented trailer support yet

        //Store a weak pointer to prevent a streamer->manager->streamer circular dependency
        _tx_streamers[chan] = boost::weak_ptr<sph::send_packet_streamer>(my_streamer);
        _tx_stream_cached_args[chan] = args;

        //Sets tick and samp rates on all streamer
        update_tick_rate(_get_tick_rate());

        //TODO: Find a way to remove this dependency
        property_tree::sptr prop_tree = _tree.lock();
        if (prop_tree) {
            //TODO: Update this to support multiple motherboards
            const fs_path mb_path = "/mboards/0";
            prop_tree->access<double>(mb_path / "tx_dsps" / std::to_string(chan) / "rate" / "value").update();
        }
    }
    update_stream_states();

    return my_streamer;
}

/***********************************************************************
 * Async Message Receiver
 **********************************************************************/
bool n230_stream_manager::recv_async_msg(async_metadata_t &async_metadata, double timeout)
{
    return _async_md_queue->pop_with_timed_wait(async_metadata, timeout);
}

/***********************************************************************
 * Sample Rate Updaters
 **********************************************************************/
void n230_stream_manager::update_rx_samp_rate(const size_t dspno, const double rate)
{
    boost::shared_ptr<sph::recv_packet_streamer> my_streamer =
        boost::dynamic_pointer_cast<sph::recv_packet_streamer>(_rx_streamers[dspno].lock());
    if (not my_streamer) return;
    my_streamer->set_samp_rate(rate);
    const double adj = _resource_mgr->get_radio(dspno).ddc->get_scaling_adjustment();
    my_streamer->set_scale_factor(adj);
}

void n230_stream_manager::update_tx_samp_rate(const size_t dspno, const double rate)
{
    boost::shared_ptr<sph::send_packet_streamer> my_streamer =
        boost::dynamic_pointer_cast<sph::send_packet_streamer>(_tx_streamers[dspno].lock());
    if (not my_streamer) return;
    my_streamer->set_samp_rate(rate);
    const double adj = _resource_mgr->get_radio(dspno).duc->get_scaling_adjustment();
    my_streamer->set_scale_factor(adj);
}

/***********************************************************************
 * Tick Rate Updater
 **********************************************************************/
void n230_stream_manager::update_tick_rate(const double rate)
{
    for (size_t i = 0; i < fpga::NUM_RADIOS; i++) {
        radio_resource_t& perif = _resource_mgr->get_radio(i);

        boost::shared_ptr<sph::recv_packet_streamer> my_rx_streamer =
            boost::dynamic_pointer_cast<sph::recv_packet_streamer>(_rx_streamers[i].lock());
        if (my_rx_streamer) my_rx_streamer->set_tick_rate(rate);
        perif.framer->set_tick_rate(rate);

        boost::shared_ptr<sph::send_packet_streamer> my_tx_streamer =
            boost::dynamic_pointer_cast<sph::send_packet_streamer>(_tx_streamers[i].lock());
        if (my_tx_streamer) my_tx_streamer->set_tick_rate(rate);
    }
}

/***********************************************************************
 * Stream State Updater
 **********************************************************************/
void n230_stream_manager::update_stream_states()
{
    //extract settings from state variables
    const bool enb_tx0 = bool(_tx_streamers[0].lock());
    const bool enb_rx0 = bool(_rx_streamers[0].lock());
    const bool enb_tx1 = bool(_tx_streamers[1].lock());
    const bool enb_rx1 = bool(_rx_streamers[1].lock());

    fe_state_t fe0_state = NONE_STREAMING;
    if (enb_tx0 && enb_rx0) fe0_state = TXRX_STREAMING;
    else if (enb_tx0)       fe0_state = TX_STREAMING;
    else if (enb_rx0)       fe0_state = RX_STREAMING;

    fe_state_t fe1_state = NONE_STREAMING;
    if (enb_tx1 && enb_rx1) fe1_state = TXRX_STREAMING;
    else if (enb_tx1)       fe1_state = TX_STREAMING;
    else if (enb_rx1)       fe1_state = RX_STREAMING;

    _resource_mgr->get_frontend_ctrl().set_stream_state(fe0_state, fe1_state);
}

size_t n230_stream_manager::_get_rx_flow_control_window(size_t frame_size, size_t sw_buff_size)
{
    double sw_buff_max = sw_buff_size * N230_RX_SW_BUFF_FULL_FACTOR;
    size_t window_in_pkts = (static_cast<size_t>(sw_buff_max) / frame_size);
    if (window_in_pkts == 0) {
        throw uhd::value_error("recv_buff_size must be larger than the recv_frame_size.");
    }
    return window_in_pkts;
}

void n230_stream_manager::_handle_overflow(const size_t i)
{
    boost::shared_ptr<sph::recv_packet_streamer> my_streamer =
            boost::dynamic_pointer_cast<sph::recv_packet_streamer>(_rx_streamers[i].lock());
    if (my_streamer->get_num_channels() == 2) {
        //MIMO
        //find out if we were in continuous mode before stopping
        const bool in_continuous_streaming_mode = _resource_mgr->get_radio(i).framer->in_continuous_streaming_mode();
        //stop streaming
        my_streamer->issue_stream_cmd(stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
        //restart streaming
        if (in_continuous_streaming_mode) {
            stream_cmd_t stream_cmd(stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
            stream_cmd.stream_now = false;
            stream_cmd.time_spec = _resource_mgr->get_radio(i).time->get_time_now() + time_spec_t(0.01);
            my_streamer->issue_stream_cmd(stream_cmd);
        }
    } else {
        _resource_mgr->get_radio(i).framer->handle_overflow();
    }
}

void n230_stream_manager::_handle_rx_flowctrl(
    const sid_t& sid,
    zero_copy_if::sptr xport,
    boost::shared_ptr<rx_fc_cache_t> fc_cache,
    const size_t last_seq)
{
    static const size_t RXFC_PACKET_LEN_IN_WORDS    = 2;
    static const size_t RXFC_CMD_CODE_OFFSET        = 0;
    static const size_t RXFC_SEQ_NUM_OFFSET         = 1;

    managed_send_buffer::sptr buff = xport->get_send_buff(0.0);
    if (not buff) {
        throw uhd::runtime_error("handle_rx_flowctrl timed out getting a send buffer");
    }
    uint32_t *pkt = buff->cast<uint32_t *>();

    //recover seq32
    size_t& seq_sw = fc_cache->last_seq_in;
    const size_t seq_hw = seq_sw & HW_SEQ_NUM_MASK;
    if (last_seq < seq_hw) seq_sw += (HW_SEQ_NUM_MASK + 1);
    seq_sw &= ~HW_SEQ_NUM_MASK;
    seq_sw |= last_seq;

    //load packet info
    vrt::if_packet_info_t packet_info;
    packet_info.packet_type = vrt::if_packet_info_t::PACKET_TYPE_CONTEXT;
    packet_info.num_payload_words32 = RXFC_PACKET_LEN_IN_WORDS;
    packet_info.num_payload_bytes = packet_info.num_payload_words32*sizeof(uint32_t);
    packet_info.packet_count = seq_sw;
    packet_info.sob = false;
    packet_info.eob = false;
    packet_info.sid = sid.get();
    packet_info.has_sid = true;
    packet_info.has_cid = false;
    packet_info.has_tsi = false;
    packet_info.has_tsf = false;
    packet_info.has_tlr = false;

    //load header
    _cvita_hdr_pack(pkt, packet_info);

    //load payload
    pkt[packet_info.num_header_words32 + RXFC_CMD_CODE_OFFSET] = uhd::htonx<uint32_t>(N230_EVENT_CODE_FLOW_CTRL);
    pkt[packet_info.num_header_words32 + RXFC_SEQ_NUM_OFFSET] = uhd::htonx<uint32_t>(seq_sw);

    //send the buffer over the interface
    buff->commit(sizeof(uint32_t)*(packet_info.num_packet_words32));
}

void n230_stream_manager::_handle_tx_async_msgs(
    boost::shared_ptr<tx_fc_cache_t> fc_cache,
    zero_copy_if::sptr xport,
    tick_rate_retriever_t get_tick_rate)
{
    managed_recv_buffer::sptr buff = xport->get_recv_buff();
    if (not buff) return;

    //extract packet info
    vrt::if_packet_info_t if_packet_info;
    if_packet_info.num_packet_words32 = buff->size()/sizeof(uint32_t);
    const uint32_t *packet_buff = buff->cast<const uint32_t *>();

    //unpacking can fail
    uint32_t (*endian_conv)(uint32_t) = uhd::ntohx;
    try {
        _cvita_hdr_unpack(packet_buff, if_packet_info);
        endian_conv = uhd::ntohx;
    } catch(const std::exception &ex) {
        UHD_LOGGER_ERROR("N230") << "Error parsing async message packet: " << ex.what() ;
        return;
    }

    //fill in the async metadata
    async_metadata_t metadata;
    load_metadata_from_buff(
        endian_conv, metadata, if_packet_info, packet_buff,
        get_tick_rate(), fc_cache->stream_channel);

    //The FC response and the burst ack are two indicators that the radio
    //consumed packets. Use them to update the FC metadata
    if (metadata.event_code == N230_EVENT_CODE_FLOW_CTRL or
        metadata.event_code == async_metadata_t::EVENT_CODE_BURST_ACK
    ) {
        const size_t seq = metadata.user_payload[0];
        fc_cache->seq_queue.push_with_pop_on_full(seq);
    }

    //FC responses don't propagate up to the user so filter them here
    if (metadata.event_code != N230_EVENT_CODE_FLOW_CTRL) {
        fc_cache->async_queue->push_with_pop_on_full(metadata);
        metadata.channel = fc_cache->device_channel;
        fc_cache->old_async_queue->push_with_pop_on_full(metadata);
        standard_async_msg_prints(metadata);
    }
}

managed_send_buffer::sptr n230_stream_manager::_get_tx_buff_with_flowctrl(
    task::sptr /*holds ref*/,
    boost::shared_ptr<tx_fc_cache_t> fc_cache,
    zero_copy_if::sptr xport,
    size_t fc_pkt_window,
    const double timeout)
{
    while (true)
    {
        const size_t delta = (fc_cache->last_seq_out & HW_SEQ_NUM_MASK) - (fc_cache->last_seq_ack & HW_SEQ_NUM_MASK);
        if ((delta & HW_SEQ_NUM_MASK) <= fc_pkt_window) break;

        const bool ok = fc_cache->seq_queue.pop_with_timed_wait(fc_cache->last_seq_ack, timeout);
        if (not ok) return managed_send_buffer::sptr(); //timeout waiting for flow control
    }

    managed_send_buffer::sptr buff = xport->get_send_buff(timeout);
    if (buff) fc_cache->last_seq_out++; //update seq, this will actually be a send
    return buff;
}

size_t n230_stream_manager::_get_tx_flow_control_window(
    size_t payload_size,
    size_t hw_buff_size)
{
    size_t window_in_pkts = hw_buff_size / payload_size;
    if (window_in_pkts == 0) {
        throw uhd::value_error("send_buff_size must be larger than the send_frame_size.");
    }
    return window_in_pkts;
}

double n230_stream_manager::_get_tick_rate()
{
    return _resource_mgr->get_clk_pps_ctrl().get_tick_rate();
}

void n230_stream_manager::_cvita_hdr_unpack(
    const uint32_t *packet_buff,
    vrt::if_packet_info_t &if_packet_info)
{
    if_packet_info.link_type = vrt::if_packet_info_t::LINK_TYPE_CHDR;
    return vrt::if_hdr_unpack_be(packet_buff, if_packet_info);
}

void n230_stream_manager::_cvita_hdr_pack(
    uint32_t *packet_buff,
    vrt::if_packet_info_t &if_packet_info)
{
    if_packet_info.link_type = vrt::if_packet_info_t::LINK_TYPE_CHDR;
    return vrt::if_hdr_pack_be(packet_buff, if_packet_info);
}

}}} //namespace
