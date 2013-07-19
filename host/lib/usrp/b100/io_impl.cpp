//
// Copyright 2011-2013 Ettus Research LLC
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

#include "validate_subdev_spec.hpp"
#include "../../transport/super_recv_packet_handler.hpp"
#include "../../transport/super_send_packet_handler.hpp"
#include "b100_impl.hpp"
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/utils/log.hpp>
#include <boost/make_shared.hpp>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;

void b100_impl::update_rates(void){
    const fs_path mb_path = "/mboards/0";
    _tree->access<double>(mb_path / "tick_rate").update();

    //and now that the tick rate is set, init the host rates to something
    BOOST_FOREACH(const std::string &name, _tree->list(mb_path / "rx_dsps")){
        _tree->access<double>(mb_path / "rx_dsps" / name / "rate" / "value").update();
    }
    BOOST_FOREACH(const std::string &name, _tree->list(mb_path / "tx_dsps")){
        _tree->access<double>(mb_path / "tx_dsps" / name / "rate" / "value").update();
    }
}

void b100_impl::update_tick_rate(const double rate){

    //update the tick rate on all existing streamers -> thread safe
    for (size_t i = 0; i < _rx_streamers.size(); i++){
        boost::shared_ptr<sph::recv_packet_streamer> my_streamer =
            boost::dynamic_pointer_cast<sph::recv_packet_streamer>(_rx_streamers[i].lock());
        if (my_streamer.get() == NULL) continue;
        my_streamer->set_tick_rate(rate);
    }
    for (size_t i = 0; i < _tx_streamers.size(); i++){
        boost::shared_ptr<sph::send_packet_streamer> my_streamer =
            boost::dynamic_pointer_cast<sph::send_packet_streamer>(_tx_streamers[i].lock());
        if (my_streamer.get() == NULL) continue;
        my_streamer->set_tick_rate(rate);
    }
}

void b100_impl::update_rx_samp_rate(const size_t dspno, const double rate){
    boost::shared_ptr<sph::recv_packet_streamer> my_streamer =
        boost::dynamic_pointer_cast<sph::recv_packet_streamer>(_rx_streamers[dspno].lock());
    if (my_streamer.get() == NULL) return;

    my_streamer->set_samp_rate(rate);
    const double adj = _rx_dsps[dspno]->get_scaling_adjustment();
    my_streamer->set_scale_factor(adj);
}

void b100_impl::update_tx_samp_rate(const size_t dspno, const double rate){
    boost::shared_ptr<sph::send_packet_streamer> my_streamer =
        boost::dynamic_pointer_cast<sph::send_packet_streamer>(_tx_streamers[dspno].lock());
    if (my_streamer.get() == NULL) return;

    my_streamer->set_samp_rate(rate);
    const double adj = _tx_dsp->get_scaling_adjustment();
    my_streamer->set_scale_factor(adj);
}

void b100_impl::update_rx_subdev_spec(const uhd::usrp::subdev_spec_t &spec){
    fs_path root = "/mboards/0/dboards";

    //sanity checking
    validate_subdev_spec(_tree, spec, "rx");

    //setup mux for this spec
    bool fe_swapped = false;
    for (size_t i = 0; i < spec.size(); i++){
        const std::string conn = _tree->access<std::string>(root / spec[i].db_name / "rx_frontends" / spec[i].sd_name / "connection").get();
        if (i == 0 and (conn == "QI" or conn == "Q")) fe_swapped = true;
        _rx_dsps[i]->set_mux(conn, fe_swapped);
    }
    _rx_fe->set_mux(fe_swapped);
}

void b100_impl::update_tx_subdev_spec(const uhd::usrp::subdev_spec_t &spec){
    fs_path root = "/mboards/0/dboards";

    //sanity checking
    validate_subdev_spec(_tree, spec, "tx");

    //set the mux for this spec
    const std::string conn = _tree->access<std::string>(root / spec[0].db_name / "tx_frontends" / spec[0].sd_name / "connection").get();
    _tx_fe->set_mux(conn);
}

/***********************************************************************
 * Async Data
 **********************************************************************/
bool b100_impl::recv_async_msg(
    async_metadata_t &async_metadata, double timeout
){
    return _fifo_ctrl->pop_async_msg(async_metadata, timeout);
}

/***********************************************************************
 * Receive streamer
 **********************************************************************/
rx_streamer::sptr b100_impl::get_rx_stream(const uhd::stream_args_t &args_){
    stream_args_t args = args_;

    //setup defaults for unspecified values
    args.otw_format = args.otw_format.empty()? "sc16" : args.otw_format;
    args.channels = args.channels.empty()? std::vector<size_t>(1, 0) : args.channels;

    //calculate packet size
    static const size_t hdr_size = 0
        + vrt::max_if_hdr_words32*sizeof(boost::uint32_t)
        + sizeof(vrt::if_packet_info_t().tlr) //forced to have trailer
        - sizeof(vrt::if_packet_info_t().cid) //no class id ever used
        - sizeof(vrt::if_packet_info_t().tsi) //no int time ever used
    ;
    const size_t bpp = _data_transport->get_recv_frame_size() - hdr_size;
    const size_t bpi = convert::get_bytes_per_item(args.otw_format);
    const size_t spp = unsigned(args.args.cast<double>("spp", bpp/bpi));

    //make the new streamer given the samples per packet
    boost::shared_ptr<sph::recv_packet_streamer> my_streamer = boost::make_shared<sph::recv_packet_streamer>(spp);

    //init some streamer stuff
    my_streamer->resize(args.channels.size());
    my_streamer->set_vrt_unpacker(&vrt::if_hdr_unpack_le);

    //set the converter
    uhd::convert::id_type id;
    id.input_format = args.otw_format + "_item32_le";
    id.num_inputs = 1;
    id.output_format = args.cpu_format;
    id.num_outputs = 1;
    my_streamer->set_converter(id);

    //bind callbacks for the handler
    for (size_t chan_i = 0; chan_i < args.channels.size(); chan_i++){
        const size_t dsp = args.channels[chan_i];
        _rx_dsps[dsp]->set_nsamps_per_packet(spp); //seems to be a good place to set this
        _rx_dsps[dsp]->setup(args);
        _recv_demuxer->realloc_sid(B100_RX_SID_BASE + dsp);
        my_streamer->set_xport_chan_get_buff(chan_i, boost::bind(
            &recv_packet_demuxer_3000::get_recv_buff, _recv_demuxer, B100_RX_SID_BASE + dsp, _1
        ), true /*flush*/);
        my_streamer->set_overflow_handler(chan_i, boost::bind(
            &rx_dsp_core_200::handle_overflow, _rx_dsps[dsp]
        ));
        my_streamer->set_issue_stream_cmd(chan_i, boost::bind(
            &rx_dsp_core_200::issue_stream_command, _rx_dsps[dsp], _1));
        _rx_streamers[dsp] = my_streamer; //store weak pointer
    }

    //sets all tick and samp rates on this streamer
    this->update_rates();

    return my_streamer;
}

/***********************************************************************
 * Transmit streamer
 **********************************************************************/
tx_streamer::sptr b100_impl::get_tx_stream(const uhd::stream_args_t &args_){
    stream_args_t args = args_;

    //setup defaults for unspecified values
    args.otw_format = args.otw_format.empty()? "sc16" : args.otw_format;
    args.channels = args.channels.empty()? std::vector<size_t>(1, 0) : args.channels;

    //calculate packet size
    static const size_t hdr_size = 0
        + vrt::max_if_hdr_words32*sizeof(boost::uint32_t)
        + sizeof(vrt::if_packet_info_t().tlr) //forced to have trailer
        - sizeof(vrt::if_packet_info_t().sid) //no stream id ever used
        - sizeof(vrt::if_packet_info_t().cid) //no class id ever used
        - sizeof(vrt::if_packet_info_t().tsi) //no int time ever used
    ;
    static const size_t bpp = _data_transport->get_send_frame_size() - hdr_size;
    const size_t spp = bpp/convert::get_bytes_per_item(args.otw_format);

    //make the new streamer given the samples per packet
    boost::shared_ptr<sph::send_packet_streamer> my_streamer = boost::make_shared<sph::send_packet_streamer>(spp);

    //init some streamer stuff
    my_streamer->resize(args.channels.size());
    my_streamer->set_vrt_packer(&vrt::if_hdr_pack_le);

    //set the converter
    uhd::convert::id_type id;
    id.input_format = args.cpu_format;
    id.num_inputs = 1;
    id.output_format = args.otw_format + "_item32_le";
    id.num_outputs = 1;
    my_streamer->set_converter(id);

    //bind callbacks for the handler
    for (size_t chan_i = 0; chan_i < args.channels.size(); chan_i++){
        const size_t dsp = args.channels[chan_i];
        UHD_ASSERT_THROW(dsp == 0); //always 0
        _tx_dsp->setup(args);
        my_streamer->set_xport_chan_get_buff(chan_i, boost::bind(
            &zero_copy_if::get_send_buff, _data_transport, _1
        ));
        my_streamer->set_async_receiver(boost::bind(&fifo_ctrl_excelsior::pop_async_msg, _fifo_ctrl, _1, _2));
        _tx_streamers[dsp] = my_streamer; //store weak pointer
    }

    //sets all tick and samp rates on this streamer
    this->update_rates();

    return my_streamer;
}
