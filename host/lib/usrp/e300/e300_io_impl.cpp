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
    bool enb_tx1 = false;
    if (_tree->exists("/mboards/0/xbar/Radio_1/tx_active")) {
        enb_tx1 = _tree->access<bool>("/mboards/0/xbar/Radio_1/tx_active").get();
    }

    bool enb_tx2 = false;
    if (_tree->exists("/mboards/0/xbar/Radio_0/tx_active")) {
        enb_tx2 = _tree->access<bool>("/mboards/0/xbar/Radio_0/tx_active").get();
    }

    bool enb_rx1 = false;
    if (_tree->exists("/mboards/0/xbar/Radio_1/rx_active")) {
        enb_rx1 = _tree->access<bool>("/mboards/0/xbar/Radio_1/rx_active").get();
    }

    bool enb_rx2 = false;
    if (_tree->exists("/mboards/0/xbar/Radio_0/tx_active")) {
        enb_rx2 = _tree->access<bool>("/mboards/0/xbar/Radio_0/rx_active").get();
    }

    const size_t max_tx_chan_count = (enb_tx1 ? 1 : 0) + (enb_tx2 ? 1 : 0);
    const size_t max_rx_chan_count = (enb_rx1 ? 1 : 0) + (enb_rx2 ? 1 : 0);
    _enforce_tick_rate_limits(max_rx_chan_count, rate, "RX");
    _enforce_tick_rate_limits(max_tx_chan_count, rate, "TX");
}

void e300_impl::_update_tick_rate(const double rate)
{
    _check_tick_rate_with_current_streamers(rate);

    BOOST_FOREACH(const std::string &block_id, _rx_streamers.keys()) {
        UHD_MSG(status) << "setting rx streamer " << block_id << " rate to " << rate << std::endl;
        boost::shared_ptr<sph::recv_packet_streamer> my_streamer =
            boost::dynamic_pointer_cast<sph::recv_packet_streamer>(_rx_streamers[block_id].lock());
        if (my_streamer) {
            my_streamer->set_tick_rate(rate);
            my_streamer->set_samp_rate(rate);
        }
    }
    BOOST_FOREACH(const std::string &block_id, _tx_streamers.keys()) {
        UHD_MSG(status) << "setting tx streamer " << block_id << " rate to " << rate << std::endl;
        boost::shared_ptr<sph::send_packet_streamer> my_streamer =
            boost::dynamic_pointer_cast<sph::send_packet_streamer>(_tx_streamers[block_id].lock());
        if (my_streamer) {
            my_streamer->set_tick_rate(rate);
            my_streamer->set_samp_rate(rate);
        }
    }
}

void e300_impl::_update_rx_samp_rate(const size_t dspno, const double rate)
{
    const std::string radio_block_id = str(boost::format("Radio_%d") % dspno);
    if (not _rx_streamers.has_key(radio_block_id))
        return;
    boost::shared_ptr<sph::recv_packet_streamer> my_streamer =
        boost::dynamic_pointer_cast<sph::recv_packet_streamer>(_rx_streamers[radio_block_id].lock());
    if (not my_streamer)
        return;
    my_streamer->set_samp_rate(rate);
    // TODO move these details to radio_ctrl
    const double adj = _radio_perifs[dspno].ddc->get_scaling_adjustment();
    my_streamer->set_scale_factor(adj);
}

void e300_impl::_update_tx_samp_rate(const size_t dspno, const double rate)
{
    const std::string radio_block_id = str(boost::format("Radio_%d") % dspno);
    if (not _tx_streamers.has_key(radio_block_id))
        return;
    boost::shared_ptr<sph::send_packet_streamer> my_streamer =
        boost::dynamic_pointer_cast<sph::send_packet_streamer>(_tx_streamers[radio_block_id].lock());
    if (not my_streamer)
        return;
    my_streamer->set_samp_rate(rate);
    // TODO move these details to radio_ctrl
    const double adj = _radio_perifs[dspno].ddc->get_scaling_adjustment();
    my_streamer->set_scale_factor(adj);
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

static size_t get_rx_flow_control_window(
    const size_t frame_size,
    const size_t sw_buff_size)
{
    double fullness_factor = E300_RX_SW_BUFF_FULLNESS;

    if (fullness_factor < 0.01 || fullness_factor > 1) {
        throw uhd::value_error("recv_buff_fullness must be between 0.01 and 1 inclusive (1% to 100%)");
    }

    size_t window_in_pkts = (static_cast<size_t>(sw_buff_size * fullness_factor) / frame_size);
    if (window_in_pkts == 0) {
        throw uhd::value_error("recv_buff_size must be larger than the recv_frame_size.");
    }
    return window_in_pkts;
}

struct e300_rx_fc_cache_t
{
    e300_rx_fc_cache_t():
        last_seq_in(0){}
    size_t last_seq_in;
};


static void handle_rx_flowctrl(
    const boost::uint32_t sid,
    zero_copy_if::sptr xport,
    boost::shared_ptr<e300_rx_fc_cache_t> fc_cache,
    const size_t last_seq)
{
    static const size_t RXFC_PACKET_LEN_IN_WORDS    = 2;
    static const size_t RXFC_CMD_CODE_OFFSET        = 0;
    static const size_t RXFC_SEQ_NUM_OFFSET         = 1;

    managed_send_buffer::sptr buff = xport->get_send_buff(0.0);
    if (not buff)
        throw uhd::runtime_error("handle_rx_flowctrl timed out getting a send buffer");
    boost::uint32_t *pkt = buff->cast<boost::uint32_t *>();

    //recover seq32
    size_t& seq_sw = fc_cache->last_seq_in;
    const size_t seq_hw = seq_sw & HW_SEQ_NUM_MASK;
    if (last_seq < seq_hw)
        seq_sw += (HW_SEQ_NUM_MASK + 1);
    seq_sw &= ~HW_SEQ_NUM_MASK;
    seq_sw |= last_seq;

    //static size_t fc_pkt_count = 0;
    //UHD_MSG(status) << "sending flow ctrl packet " << fc_pkt_count++ << ", acking " << str(boost::format("%04d\tseq_sw==0x%08x - SID = ") % last_seq % seq_sw) << uhd::sid_t(sid) << std::endl;

    //load packet info
    vrt::if_packet_info_t packet_info;
    packet_info.packet_type = vrt::if_packet_info_t::PACKET_TYPE_FC;
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

    // hardcode bits
    pkt[0] = (pkt[0] & 0x00ffffff) | 0x40000000;

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

static size_t get_tx_flow_control_window(
    const size_t frame_size,
    const size_t hw_buff_size)
{
    size_t window_in_pkts = (static_cast<size_t>(hw_buff_size) / frame_size);
    if (window_in_pkts == 0) {
        throw uhd::value_error("send_buff_size must be larger than the send_frame_size.");
    }
    return window_in_pkts;
}

#define E300_ASYNC_EVENT_CODE_FLOW_CTRL 0

typedef boost::function<double(void)> tick_rate_retriever_t;


static void handle_tx_async_msgs(
            boost::shared_ptr<e300_tx_fc_cache_t> fc_cache,
             zero_copy_if::sptr xport,
             boost::function<double(void)> get_tick_rate
)
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
 * Helper functions for get_?x_stream()
 * TODO: Move these up, they are generic
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

void e300_impl::_generate_channel_list(
        const uhd::stream_args_t &args,
        std::vector<uhd::rfnoc::block_id_t> &chan_list,
        std::vector<device_addr_t> &chan_args,
        const std::string &xx)
{
    if (args.args.has_key("block_id")) { // Override channel settings
        // TODO: Figure out how to put in more than one block ID in the stream args args
        UHD_MSG(status) << boost::format("e300_impl::_generate_channel_list() - %s") % args.args["block_id"] << std::endl;

        if (not (args.channels.size() == 1 and args.channels[0] == 0)) {
            throw uhd::runtime_error("When specifying the block ID in stream args, channels must start at 0.");
        }
        chan_list.push_back(uhd::rfnoc::block_id_t(args.args["block_id"]));
        chan_args.push_back(device_addr_t());
    } else {
        BOOST_FOREACH(const size_t chan_idx, args.channels)
        {
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
 * Receive streamer
 **********************************************************************/
rx_streamer::sptr e300_impl::get_rx_stream(const uhd::stream_args_t &args_)
{
    boost::mutex::scoped_lock lock(_stream_spawn_mutex);
    stream_args_t args = sanitize_stream_args(args_);

    // I. Generate the channel list
    std::vector<uhd::rfnoc::block_id_t> chan_list;
    std::vector<device_addr_t> chan_args;
    _generate_channel_list(args, chan_list, chan_args, "rx");

    // II. Iterate over all channels
    boost::shared_ptr<sph::recv_packet_streamer> my_streamer;
    for (size_t stream_i = 0; stream_i < chan_list.size(); stream_i++)
    {
        // Get block ID
        uhd::rfnoc::block_id_t block_id = chan_list[stream_i];
        size_t block_port = chan_args[stream_i].cast<size_t>("block_port", 0);
        UHD_ASSERT_THROW(block_port < 16);

        // Access to this channel's block control
        uhd::rfnoc::rx_block_ctrl_base::sptr ce_ctrl =
            boost::dynamic_pointer_cast<uhd::rfnoc::rx_block_ctrl_base>(get_block_ctrl(block_id));

        // FIXME FIXME: Deal with the transport hints ...


        //allocate sid and create transport
        uhd::sid_t data_sid = ce_ctrl->get_address(block_port);

        UHD_MSG(status) << "creating rx stream " << std::endl;

        both_xports_t xport = _make_transport(data_sid, RX_DATA, device_addr_t());

        UHD_MSG(status) << "data_sid = " << xport.send_sid << std::endl;

        // To calculate the max number of samples per packet, we assume the maximum header length
        // to avoid fragmentation should the entire header be used.
        static const size_t hdr_size = 0
            + vrt::num_vrl_words32*sizeof(boost::uint32_t)
            + vrt::max_if_hdr_words32*sizeof(boost::uint32_t)
            + sizeof(vrt::if_packet_info_t().tlr) //forced to have trailer
            - sizeof(vrt::if_packet_info_t().cid) //no class id ever used
            - sizeof(vrt::if_packet_info_t().tsi) //no int time ever used
        ;

        const size_t bpp = xport.recv->get_recv_frame_size() - hdr_size; // bytes per packet
        const size_t bpi = convert::get_bytes_per_item(args.otw_format); // bytes per item
	// FIXME: Something goes wrong here if we don't hardcode this
//        const size_t spp = std::min(args.args.cast<size_t>("spp", bpp/bpi), bpp/bpi); // samples per packet
	UHD_MSG(status) << "SPP=" << args.args.cast<std::string>("spp", "364") << " " << bpp/bpi << std::endl;
        const size_t spp = bpp/bpi; // samples per packet

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

        // Configure the block
        ce_ctrl->setup_rx_streamer(args);
        ce_ctrl->set_destination(xport.send_sid.get_src(), block_port);

        const size_t pkt_size = spp * bpi + hdr_size;
        const size_t fc_window = get_rx_flow_control_window(pkt_size, xport.recv_buff_size);
        UHD_MSG(status) << "xport.recv_buff_size = " << xport.recv_buff_size << std::endl;
        UHD_MSG(status) << "pkt_size = " << pkt_size << std::endl;
        const size_t fc_handle_window = std::max<size_t>(1, fc_window / E300_RX_FC_REQUEST_FREQ);
        UHD_MSG(status)<< "RX Flow Control Window = " << fc_window << ", RX Flow Control Handler Window = " << fc_handle_window << std::endl;

        //flow control setup
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
        //Using "this" is OK because we know that e300_impl will outlive the streamer
        my_streamer->set_overflow_handler(
            stream_i,
            boost::bind(
                &uhd::rfnoc::rx_block_ctrl_base::handle_overrun, ce_ctrl,
                boost::weak_ptr<uhd::rx_streamer>(my_streamer)
            )
        );

        //Give the streamer a functor to send flow control messages
        //handle_rx_flowctrl is static and has no lifetime issues
        boost::shared_ptr<e300_rx_fc_cache_t> fc_cache(new e300_rx_fc_cache_t());
        my_streamer->set_xport_handle_flowctrl(
            stream_i,
            boost::bind(&handle_rx_flowctrl, xport.send_sid, xport.send, fc_cache, _1),
            fc_handle_window,
            true/*init*/);

        //Give the streamer a functor issue stream cmd
        //bind requires a shared pointer to add a streamer->framer lifetime dependency
        my_streamer->set_issue_stream_cmd(
            stream_i, boost::bind(&uhd::rfnoc::rx_block_ctrl_base::issue_stream_cmd, ce_ctrl, _1)
        );

        // Tell the streamer which SID is valid for this channel
        my_streamer->set_xport_chan_sid(stream_i, true, xport.send_sid);

        // Store a weak pointer to prevent a streamer->e300_impl->streamer circular dependency
        _rx_streamers[ce_ctrl->get_block_id().get()] = boost::weak_ptr<sph::recv_packet_streamer>(my_streamer);
        //sets all tick and samp rates on this streamer
        const fs_path mb_path = "/mboards/0";
        _tree->access<double>(mb_path / "tick_rate").update();
        // TODO this is specific to radios and thus should be done by radio_ctrl
        if (ce_ctrl->get_block_id().get_block_name() == "Radio") {
            UHD_MSG(status) << "This is a radio, thus updating sample rate" << std::endl;
            _tree->access<double>(mb_path / "rx_dsps" / boost::lexical_cast<std::string>(ce_ctrl->get_block_id().get_block_count()) / "rate" / "value").update();
        }
    };
    _update_enables();
    return my_streamer;
}

/***********************************************************************
 * Transmit streamer
 **********************************************************************/
tx_streamer::sptr e300_impl::get_tx_stream(const uhd::stream_args_t &args_)
{
    boost::mutex::scoped_lock lock(_stream_spawn_mutex);
    stream_args_t args = sanitize_stream_args(args_);

    // I. Generate the channel list
    std::vector<uhd::rfnoc::block_id_t> chan_list;
    std::vector<device_addr_t> chan_args;
    _generate_channel_list(args, chan_list, chan_args, "tx");
    //shared async queue for all channels in streamer
    boost::shared_ptr<async_md_type> async_md(new async_md_type(1000/*messages deep*/));

    // II. Iterate over all channels
    boost::shared_ptr<sph::send_packet_streamer> my_streamer;
    for (size_t stream_i = 0; stream_i < chan_list.size(); stream_i++) {
        // Get block ID and mb index
        uhd::rfnoc::block_id_t block_id = chan_list[stream_i];
        size_t block_port = chan_args[stream_i].cast<size_t>("block_port", 0);
        UHD_ASSERT_THROW(block_port < 16);

        // Access to this channel's block control
        uhd::rfnoc::tx_block_ctrl_base::sptr ce_ctrl =
            boost::dynamic_pointer_cast<uhd::rfnoc::tx_block_ctrl_base>(get_block_ctrl(block_id));

        // TODO?! Setup the dsp transport hints

        //allocate sid and create transport
        uhd::sid_t data_sid = ce_ctrl->get_address(block_port);

        UHD_MSG(status) << "creating tx stream " << std::endl;
        both_xports_t xport = _make_transport(data_sid, TX_DATA, device_addr_t());

        UHD_MSG(status) << "data_sid = " << xport.send_sid << std::endl;

        // To calculate the max number of samples per packet, we assume the maximum header length
        // to avoid fragmentation should the entire header be used.
        static const size_t hdr_size = 0
            + vrt::num_vrl_words32*sizeof(boost::uint32_t)
            + vrt::max_if_hdr_words32*sizeof(boost::uint32_t)
            + sizeof(vrt::if_packet_info_t().tlr) //forced to have trailer
            - sizeof(vrt::if_packet_info_t().cid) //no class id ever used
            - sizeof(vrt::if_packet_info_t().tsi) //no int time ever used
        ;

        const size_t bpp = xport.send->get_send_frame_size() - hdr_size; // bytes per packet
        const size_t bpi = convert::get_bytes_per_item(args.otw_format); // bytes per item
        const size_t spp = std::min(args.args.cast<size_t>("spp", bpp/bpi), bpp/bpi); // samples per packet

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

        // Configure the block
        ce_ctrl->setup_tx_streamer(args);

        //flow control setup
        //const size_t fc_window = xport.send->get_num_send_frames();
        const size_t pkt_size = spp * bpi + hdr_size;
        const size_t fc_window = get_tx_flow_control_window(
           pkt_size,
           ce_ctrl->get_fifo_size(block_port));
        const size_t fc_handle_window = std::max<size_t>(1, fc_window / E300_TX_FC_RESPONSE_FREQ);

        ce_ctrl->configure_flow_control_in(0/*cycs off*/, fc_handle_window /*pkts*/, block_port);

        boost::shared_ptr<e300_tx_fc_cache_t> fc_cache(new e300_tx_fc_cache_t());
        fc_cache->stream_channel = stream_i;
        fc_cache->device_channel = args.channels[stream_i];
        fc_cache->async_queue = async_md;
        fc_cache->old_async_queue = _async_md;

        tick_rate_retriever_t get_tick_rate_fn = boost::bind(&e300_impl::_get_tick_rate, this);

        task::sptr task = task::make(
            boost::bind(&handle_tx_async_msgs, fc_cache, xport.recv, get_tick_rate_fn));

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

        //Store a weak pointer to prevent a streamer->e300_impl->streamer circular dependency
        _tx_streamers[ce_ctrl->get_block_id().get()] = boost::weak_ptr<sph::send_packet_streamer>(my_streamer);

        //sets all tick and samp rates on this streamer
        const fs_path mb_path = "/mboards/0";
        _tree->access<double>(mb_path / "tick_rate").update();
        if (ce_ctrl->get_block_id().get_block_name() == "Radio") {
            // TODO this is specific to radios and thus should be done by radio_ctrl
            UHD_MSG(status) << "This is a radio, thus updating sample rate" << std::endl;
            _tree->access<double>(mb_path / "tx_dsps" / boost::lexical_cast<std::string>(ce_ctrl->get_block_id().get_block_count()) / "rate" / "value").update();
        }
    }
    _update_enables();
    return my_streamer;
}
}}} // namespace
