//
// Copyright 2012-2013 Ettus Research LLC
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

#include "b200_regs.hpp"
#include "b200_impl.hpp"
#include "validate_subdev_spec.hpp"
#include "../../transport/super_recv_packet_handler.hpp"
#include "../../transport/super_send_packet_handler.hpp"
#include "async_packet_handler.hpp"
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <set>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;

/***********************************************************************
 * update streamer rates
 **********************************************************************/
void b200_impl::check_tick_rate_with_current_streamers(double rate)
{
    size_t max_tx_chan_count = 0, max_rx_chan_count = 0;
    BOOST_FOREACH(radio_perifs_t &perif, _radio_perifs)
    {
        {
            boost::shared_ptr<sph::recv_packet_streamer> rx_streamer =
                boost::dynamic_pointer_cast<sph::recv_packet_streamer>(perif.rx_streamer.lock());
            if (rx_streamer)
                max_rx_chan_count = std::max(max_rx_chan_count, rx_streamer->get_num_channels());
        }

        {
            boost::shared_ptr<sph::send_packet_streamer> tx_streamer =
                boost::dynamic_pointer_cast<sph::send_packet_streamer>(perif.tx_streamer.lock());
            if (tx_streamer)
                max_tx_chan_count = std::max(max_tx_chan_count, tx_streamer->get_num_channels());
        }
    }

    // Defined in b200_impl.cpp
    enforce_tick_rate_limits(max_rx_chan_count, rate, "RX");
    enforce_tick_rate_limits(max_tx_chan_count, rate, "TX");
}

void b200_impl::check_streamer_args(const uhd::stream_args_t &args, double tick_rate, const char* direction /*= NULL*/)
{
    std::set<size_t> chans_set;
    for (size_t stream_i = 0; stream_i < args.channels.size(); stream_i++)
    {
        const size_t chan = args.channels[stream_i];
        chans_set.insert(chan);
    }

    enforce_tick_rate_limits(chans_set.size(), tick_rate, direction);   // Defined in b200_impl.cpp
}

void b200_impl::update_tick_rate(const double rate)
{
    check_tick_rate_with_current_streamers(rate);

    BOOST_FOREACH(radio_perifs_t &perif, _radio_perifs)
    {
        boost::shared_ptr<sph::recv_packet_streamer> my_streamer =
            boost::dynamic_pointer_cast<sph::recv_packet_streamer>(perif.rx_streamer.lock());
        if (my_streamer) my_streamer->set_tick_rate(rate);
        perif.framer->set_tick_rate(_tick_rate);
    }
    BOOST_FOREACH(radio_perifs_t &perif, _radio_perifs)
    {
        boost::shared_ptr<sph::send_packet_streamer> my_streamer =
            boost::dynamic_pointer_cast<sph::send_packet_streamer>(perif.tx_streamer.lock());
        if (my_streamer) my_streamer->set_tick_rate(rate);
        perif.deframer->set_tick_rate(_tick_rate);
    }
}

void b200_impl::update_rx_samp_rate(const size_t dspno, const double rate)
{
    boost::shared_ptr<sph::recv_packet_streamer> my_streamer =
        boost::dynamic_pointer_cast<sph::recv_packet_streamer>(_radio_perifs[dspno].rx_streamer.lock());
    if (not my_streamer) return;
    my_streamer->set_samp_rate(rate);
    const double adj = _radio_perifs[dspno].ddc->get_scaling_adjustment();
    my_streamer->set_scale_factor(adj);
}

void b200_impl::update_tx_samp_rate(const size_t dspno, const double rate)
{
    boost::shared_ptr<sph::send_packet_streamer> my_streamer =
        boost::dynamic_pointer_cast<sph::send_packet_streamer>(_radio_perifs[dspno].tx_streamer.lock());
    if (not my_streamer) return;
    my_streamer->set_samp_rate(rate);
    const double adj = _radio_perifs[dspno].duc->get_scaling_adjustment();
    my_streamer->set_scale_factor(adj);
}

/***********************************************************************
 * frontend selection
 **********************************************************************/
void b200_impl::update_subdev_spec(const std::string &tx_rx, const uhd::usrp::subdev_spec_t &spec)
{
    //sanity checking
    if (spec.size()) validate_subdev_spec(_tree, spec, tx_rx);
    UHD_ASSERT_THROW(spec.size() <= _radio_perifs.size());

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
    for (size_t i = 0; i < spec.size(); i++) {
	chan_to_dsp_map[i] = (spec[i].sd_name == "A") ? 0 : 1;
    }
    _tree->access<std::vector<size_t> >("/mboards/0" / (tx_rx + "_chan_dsp_mapping")).set(chan_to_dsp_map);

    this->update_enables();
}

static void b200_if_hdr_unpack_le(
    const boost::uint32_t *packet_buff,
    vrt::if_packet_info_t &if_packet_info
){
    if_packet_info.link_type = vrt::if_packet_info_t::LINK_TYPE_CHDR;
    return vrt::if_hdr_unpack_le(packet_buff, if_packet_info);
}

static void b200_if_hdr_pack_le(
    boost::uint32_t *packet_buff,
    vrt::if_packet_info_t &if_packet_info
){
    if_packet_info.link_type = vrt::if_packet_info_t::LINK_TYPE_CHDR;
    return vrt::if_hdr_pack_le(packet_buff, if_packet_info);
}

/***********************************************************************
 * Async Data
 **********************************************************************/
bool b200_impl::recv_async_msg(
    async_metadata_t &async_metadata, double timeout
){
    return _async_task_data->async_md->pop_with_timed_wait(async_metadata, timeout);
}

/*
 * This method is constantly called in a msg_task loop.
 * Incoming messages are dispatched in to the hosts radio_ctrl_cores.
 * The radio_ctrl_core queues are accessed via a weak_ptr to them, stored in AsyncTaskData.
 * During shutdown the radio_ctrl_core dtor's are called.
 * An empty peek32(0) is sent out to flush pending async messages.
 * The response to those messages can't be delivered to the ctrl_core queues anymore
 * because the shared pointer corresponding to the weak_ptrs is no longer valid.
 * Those stranded messages are put into a dump_queue implemented in msg_task.
 * A radio_ctrl_core can search for missing messages there.
 */
boost::optional<uhd::msg_task::msg_type_t> b200_impl::handle_async_task(
    uhd::transport::zero_copy_if::sptr xport,
    boost::shared_ptr<AsyncTaskData> data
)
{
    managed_recv_buffer::sptr buff = xport->get_recv_buff();
    if (not buff or buff->size() < 8)
        return boost::none;

    const boost::uint32_t sid = uhd::wtohx(buff->cast<const boost::uint32_t *>()[1]);
    switch (sid) {

    //if the packet is a control response
    case B200_RESP0_MSG_SID:
    case B200_RESP1_MSG_SID:
    case B200_LOCAL_RESP_SID:
    {
    	radio_ctrl_core_3000::sptr ctrl;
        if (sid == B200_RESP0_MSG_SID) ctrl = data->radio_ctrl[0].lock();
        if (sid == B200_RESP1_MSG_SID) ctrl = data->radio_ctrl[1].lock();
        if (sid == B200_LOCAL_RESP_SID) ctrl = data->local_ctrl.lock();
        if (ctrl){
        	ctrl->push_response(buff->cast<const boost::uint32_t *>());
        }
        else{
            return std::make_pair(sid, uhd::msg_task::buff_to_vector(buff->cast<boost::uint8_t *>(), buff->size() ) );
        }
        break;
    }

    //if the packet is a uart message
    case B200_RX_GPS_UART_SID:
    {
        data->gpsdo_uart->handle_uart_packet(buff);
        break;
    }

    //or maybe the packet is a TX async message
    case B200_TX_MSG0_SID:
    case B200_TX_MSG1_SID:
    {
        const size_t i = (sid == B200_TX_MSG0_SID)? 0 : 1;

        //extract packet info
        vrt::if_packet_info_t if_packet_info;
        if_packet_info.num_packet_words32 = buff->size()/sizeof(boost::uint32_t);
        const boost::uint32_t *packet_buff = buff->cast<const boost::uint32_t *>();

        //unpacking can fail
        try
        {
            b200_if_hdr_unpack_le(packet_buff, if_packet_info);
        }
        catch(const std::exception &ex)
        {
            UHD_MSG(error) << "Error parsing ctrl packet: " << ex.what() << std::endl;
            break;
        }

        //fill in the async metadata
        async_metadata_t metadata;
        load_metadata_from_buff(uhd::wtohx<boost::uint32_t>, metadata, if_packet_info, packet_buff, _tick_rate, i);
        data->async_md->push_with_pop_on_full(metadata);
        standard_async_msg_prints(metadata);
        break;
    }

    //doh!
    default:
        UHD_MSG(error) << "Got a ctrl packet with unknown SID " << sid << std::endl;
    }
    return boost::none;
}

/***********************************************************************
 * Receive streamer
 **********************************************************************/
rx_streamer::sptr b200_impl::get_rx_stream(const uhd::stream_args_t &args_)
{
    stream_args_t args = args_;

    //setup defaults for unspecified values
    if (args.otw_format.empty()) args.otw_format = "sc16";
    args.channels = args.channels.empty()? std::vector<size_t>(1, 0) : args.channels;

    check_streamer_args(args, this->get_tick_rate(), "RX");

    boost::shared_ptr<sph::recv_packet_streamer> my_streamer;
    for (size_t stream_i = 0; stream_i < args.channels.size(); stream_i++)
    {
        const size_t radio_index = _tree->access<std::vector<size_t> >("/mboards/0/rx_chan_dsp_mapping")
                                        .get().at(args.channels[stream_i]);
        radio_perifs_t &perif = _radio_perifs[radio_index];
        if (args.otw_format == "sc16") perif.ctrl->poke32(TOREG(SR_RX_FMT), 0);
        if (args.otw_format == "sc12") perif.ctrl->poke32(TOREG(SR_RX_FMT), 1);
        if (args.otw_format == "fc32") perif.ctrl->poke32(TOREG(SR_RX_FMT), 2);
        if (args.otw_format == "sc8") perif.ctrl->poke32(TOREG(SR_RX_FMT), 3);
        const boost::uint32_t sid = radio_index ? B200_RX_DATA1_SID : B200_RX_DATA0_SID;

        //calculate packet size
        static const size_t hdr_size = 0
            + vrt::max_if_hdr_words32*sizeof(boost::uint32_t)
            //+ sizeof(vrt::if_packet_info_t().tlr) //no longer using trailer
            - sizeof(vrt::if_packet_info_t().cid) //no class id ever used
            - sizeof(vrt::if_packet_info_t().tsi) //no int time ever used
        ;
        const size_t bpp = _data_transport->get_recv_frame_size() - hdr_size;
        const size_t bpi = convert::get_bytes_per_item(args.otw_format);
        size_t spp = unsigned(args.args.cast<double>("spp", bpp/bpi));
        spp = std::min<size_t>(4092, spp); //FPGA FIFO maximum for framing at full rate

        //make the new streamer given the samples per packet
        if (not my_streamer) my_streamer = boost::make_shared<sph::recv_packet_streamer>(spp);
        my_streamer->resize(args.channels.size());

        //init some streamer stuff
        my_streamer->set_vrt_unpacker(&b200_if_hdr_unpack_le);

        //set the converter
        uhd::convert::id_type id;
        id.input_format = args.otw_format + "_item32_le";
        id.num_inputs = 1;
        id.output_format = args.cpu_format;
        id.num_outputs = 1;
        my_streamer->set_converter(id);

        perif.framer->clear();
        perif.framer->set_nsamps_per_packet(spp);
        perif.framer->set_sid(sid);
        perif.framer->setup(args);
        perif.ddc->setup(args);
        _demux->realloc_sid(sid);
        my_streamer->set_xport_chan_get_buff(stream_i, boost::bind(
            &recv_packet_demuxer_3000::get_recv_buff, _demux, sid, _1
        ), true /*flush*/);
        my_streamer->set_overflow_handler(stream_i, boost::bind(
            &b200_impl::handle_overflow, this, radio_index
        ));
        my_streamer->set_issue_stream_cmd(stream_i, boost::bind(
            &rx_vita_core_3000::issue_stream_command, perif.framer, _1
        ));
        perif.rx_streamer = my_streamer; //store weak pointer

        //sets all tick and samp rates on this streamer
        this->update_tick_rate(this->get_tick_rate());
        _tree->access<double>(str(boost::format("/mboards/0/rx_dsps/%u/rate/value") % radio_index)).update();
    }
    this->update_enables();

    return my_streamer;
}

void b200_impl::handle_overflow(const size_t radio_index)
{
    boost::shared_ptr<sph::recv_packet_streamer> my_streamer =
            boost::dynamic_pointer_cast<sph::recv_packet_streamer>(_radio_perifs[radio_index].rx_streamer.lock());
    if (my_streamer->get_num_channels() == 2) //MIMO time
    {
        //find out if we were in continuous mode before stopping
        const bool in_continuous_streaming_mode = _radio_perifs[radio_index].framer->in_continuous_streaming_mode();
        //stop streaming
        my_streamer->issue_stream_cmd(stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
        //flush demux
        _demux->realloc_sid(B200_RX_DATA0_SID);
        _demux->realloc_sid(B200_RX_DATA1_SID);
        //flush actual transport
        while (_data_transport->get_recv_buff(0.001)){}
        //restart streaming
        if (in_continuous_streaming_mode)
        {
            stream_cmd_t stream_cmd(stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
            stream_cmd.stream_now = false;
            stream_cmd.time_spec = _radio_perifs[radio_index].time64->get_time_now() + time_spec_t(0.01);
            my_streamer->issue_stream_cmd(stream_cmd);
        }
    }
    else _radio_perifs[radio_index].framer->handle_overflow();
}

/***********************************************************************
 * Transmit streamer
 **********************************************************************/
tx_streamer::sptr b200_impl::get_tx_stream(const uhd::stream_args_t &args_)
{
    stream_args_t args = args_;

    //setup defaults for unspecified values
    if (args.otw_format.empty()) args.otw_format = "sc16";
    args.channels = args.channels.empty()? std::vector<size_t>(1, 0) : args.channels;

    check_streamer_args(args, this->get_tick_rate(), "TX");

    boost::shared_ptr<sph::send_packet_streamer> my_streamer;
    for (size_t stream_i = 0; stream_i < args.channels.size(); stream_i++)
    {
        const size_t radio_index = _tree->access<std::vector<size_t> >("/mboards/0/tx_chan_dsp_mapping")
                                        .get().at(args.channels[stream_i]);
        radio_perifs_t &perif = _radio_perifs[radio_index];
        if (args.otw_format == "sc16") perif.ctrl->poke32(TOREG(SR_TX_FMT), 0);
        if (args.otw_format == "sc12") perif.ctrl->poke32(TOREG(SR_TX_FMT), 1);
        if (args.otw_format == "fc32") perif.ctrl->poke32(TOREG(SR_TX_FMT), 2);
        if (args.otw_format == "sc8") perif.ctrl->poke32(TOREG(SR_TX_FMT), 3);

        //calculate packet size
        static const size_t hdr_size = 0
            + vrt::max_if_hdr_words32*sizeof(boost::uint32_t)
            //+ sizeof(vrt::if_packet_info_t().tlr) //forced to have trailer
            - sizeof(vrt::if_packet_info_t().cid) //no class id ever used
            - sizeof(vrt::if_packet_info_t().tsi) //no int time ever used
        ;
        static const size_t bpp = _data_transport->get_send_frame_size() - hdr_size;
        const size_t spp = bpp/convert::get_bytes_per_item(args.otw_format);

        //make the new streamer given the samples per packet
        if (not my_streamer) my_streamer = boost::make_shared<sph::send_packet_streamer>(spp);
        my_streamer->resize(args.channels.size());

        //init some streamer stuff
        my_streamer->set_vrt_packer(&b200_if_hdr_pack_le);

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

        my_streamer->set_xport_chan_get_buff(stream_i, boost::bind(
            &zero_copy_if::get_send_buff, _data_transport, _1
        ));
        my_streamer->set_async_receiver(boost::bind(
            &async_md_type::pop_with_timed_wait, _async_task_data->async_md, _1, _2
        ));
        my_streamer->set_xport_chan_sid(stream_i, true, radio_index ? B200_TX_DATA1_SID : B200_TX_DATA0_SID);
        my_streamer->set_enable_trailer(false); //TODO not implemented trailer support yet
        perif.tx_streamer = my_streamer; //store weak pointer

        //sets all tick and samp rates on this streamer
        this->update_tick_rate(this->get_tick_rate());
        _tree->access<double>(str(boost::format("/mboards/0/tx_dsps/%u/rate/value") % radio_index)).update();
    }
    this->update_enables();

    return my_streamer;
}
