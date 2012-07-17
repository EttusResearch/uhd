//
// Copyright 2010-2012 Ettus Research LLC
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

#include "recv_packet_demuxer.hpp"
#include "validate_subdev_spec.hpp"
#include "async_packet_handler.hpp"
#include "../../transport/super_recv_packet_handler.hpp"
#include "../../transport/super_send_packet_handler.hpp"
#include <linux/usrp_e.h> //ioctl structures and constants
#include "e100_impl.hpp"
#include "e100_regs.hpp"
#include <uhd/utils/msg.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/tasks.hpp>
#include <uhd/utils/thread_priority.hpp>
#include <uhd/transport/bounded_buffer.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <poll.h> //poll
#include <fcntl.h> //open, close
#include <sstream>
#include <fstream>
#include <boost/make_shared.hpp>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;

static const size_t vrt_send_header_offset_words32 = 1;

/***********************************************************************
 * io impl details (internal to this file)
 * - pirate crew of 1
 * - bounded buffer
 * - thread loop
 * - vrt packet handler states
 **********************************************************************/
struct e100_impl::io_impl{
    io_impl(void):
        false_alarm(0), async_msg_fifo(1000/*messages deep*/)
    { /* NOP */ }

    double tick_rate; //set by update tick rate method
    e100_ctrl::sptr iface; //so handle irq can peek and poke
    void handle_irq(void);
    size_t false_alarm;
    //The data transport is listed first so that it is deconstructed last,
    //which is after the states and booty which may hold managed buffers.
    recv_packet_demuxer::sptr demuxer;

    //a pirate's life is the life for me!
    void recv_pirate_loop(
        spi_iface::sptr //keep a sptr to iface which shares gpio147
    ){
        //open the GPIO and set it up for an IRQ
        std::ofstream edge_file("/sys/class/gpio/gpio147/edge");
        edge_file << "rising" << std::endl << std::flush;
        edge_file.close();
        int fd = ::open("/sys/class/gpio/gpio147/value", O_RDONLY);
        if (fd < 0) UHD_MSG(error) << "Unable to open GPIO for IRQ\n";

        while (not boost::this_thread::interruption_requested()){
            pollfd pfd;
            pfd.fd = fd;
            pfd.events = POLLPRI | POLLERR;
            ssize_t ret = ::poll(&pfd, 1, 100/*ms*/);
            if (ret > 0) this->handle_irq();
        }

        //cleanup before thread exit
        ::close(fd);
    }
    bounded_buffer<async_metadata_t> async_msg_fifo;
    task::sptr pirate_task;
};

void e100_impl::io_impl::handle_irq(void){
    //check the status of the async msg buffer
    const boost::uint32_t status = iface->peek32(E100_REG_RB_ERR_STATUS);
    if ((status & 0x3) == 0){ //not done or error
        //This could be a false-alarm because spi readback is mixed in.
        //So we just sleep for a bit rather than interrupt continuously.
        if (false_alarm++ > 3) boost::this_thread::sleep(boost::posix_time::milliseconds(1));
        return;
    }
    false_alarm = 0; //its a real message, reset the count...
    //std::cout << boost::format("status: 0x%x") % status << std::endl;

    //load the data struct and call the ioctl
    usrp_e_ctl32 data;
    data.offset = E100_REG_ERR_BUFF;
    data.count = status >> 16;
    iface->ioctl(USRP_E_READ_CTL32, &data);
    //for (size_t i = 0; i < data.count; i++){
        //data.buf[i] = iface->peek32(E100_REG_ERR_BUFF + i*sizeof(boost::uint32_t));
        //std::cout << boost::format("    buff[%u] = 0x%08x\n") % i % data.buf[i];
    //}

    //unpack the vrt header and process below...
    vrt::if_packet_info_t if_packet_info;
    if_packet_info.num_packet_words32 = data.count;
    try{vrt::if_hdr_unpack_le(data.buf, if_packet_info);}
    catch(const std::exception &e){
        UHD_MSG(error) << "Error unpacking vrt header:\n" << e.what() << std::endl;
        goto prepare;
    }

    //handle a tx async report message
    if (if_packet_info.sid == E100_TX_ASYNC_SID and if_packet_info.packet_type != vrt::if_packet_info_t::PACKET_TYPE_DATA){

        //fill in the async metadata
        async_metadata_t metadata;
        load_metadata_from_buff(uhd::wtohx<boost::uint32_t>, metadata, if_packet_info, data.buf, tick_rate);

        //push the message onto the queue
        async_msg_fifo.push_with_pop_on_full(metadata);

        //print some fastpath messages
        standard_async_msg_prints(metadata);
    }

    //prepare for the next round
    prepare:
    iface->poke32(E100_REG_SR_ERR_CTRL, 1 << 0); //clear
    while ((iface->peek32(E100_REG_RB_ERR_STATUS) & (1 << 2)) == 0){} //wait for idle
    iface->poke32(E100_REG_SR_ERR_CTRL, 1 << 1); //start
}

/***********************************************************************
 * Helper Functions
 **********************************************************************/
void e100_impl::io_init(void){

    //create new io impl
    _io_impl = UHD_PIMPL_MAKE(io_impl, ());
    _io_impl->demuxer = recv_packet_demuxer::make(_data_transport, _rx_dsps.size(), E100_RX_SID_BASE);
    _io_impl->iface = _fpga_ctrl;

    //clear fifo state machines
    _fpga_ctrl->poke32(E100_REG_CLEAR_FIFO, 0);

    //allocate streamer weak ptrs containers
    _rx_streamers.resize(_rx_dsps.size());
    _tx_streamers.resize(1/*known to be 1 dsp*/);

    //prepare the async msg buffer for incoming messages
    _fpga_ctrl->poke32(E100_REG_SR_ERR_CTRL, 1 << 0); //clear
    while ((_fpga_ctrl->peek32(E100_REG_RB_ERR_STATUS) & (1 << 2)) == 0){} //wait for idle
    _fpga_ctrl->poke32(E100_REG_SR_ERR_CTRL, 1 << 1); //start

    //spawn a pirate, yarrr!
    _io_impl->pirate_task = task::make(boost::bind(
        &e100_impl::io_impl::recv_pirate_loop, _io_impl.get(), _aux_spi_iface
    ));
}

void e100_impl::update_tick_rate(const double rate){
    _io_impl->tick_rate = rate;

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

void e100_impl::update_rx_samp_rate(const size_t dspno, const double rate){
    boost::shared_ptr<sph::recv_packet_streamer> my_streamer =
        boost::dynamic_pointer_cast<sph::recv_packet_streamer>(_rx_streamers[dspno].lock());
    if (my_streamer.get() == NULL) return;

    my_streamer->set_samp_rate(rate);
    const double adj = _rx_dsps[dspno]->get_scaling_adjustment();
    my_streamer->set_scale_factor(adj);
}

void e100_impl::update_tx_samp_rate(const size_t dspno, const double rate){
    boost::shared_ptr<sph::send_packet_streamer> my_streamer =
        boost::dynamic_pointer_cast<sph::send_packet_streamer>(_tx_streamers[dspno].lock());
    if (my_streamer.get() == NULL) return;

    my_streamer->set_samp_rate(rate);
    const double adj = _tx_dsp->get_scaling_adjustment();
    my_streamer->set_scale_factor(adj);
}

void e100_impl::update_rates(void){
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

void e100_impl::update_rx_subdev_spec(const uhd::usrp::subdev_spec_t &spec){
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

void e100_impl::update_tx_subdev_spec(const uhd::usrp::subdev_spec_t &spec){
    fs_path root = "/mboards/0/dboards";

    //sanity checking
    validate_subdev_spec(_tree, spec, "tx");

    //set the mux for this spec
    const std::string conn = _tree->access<std::string>(root / spec[0].db_name / "tx_frontends" / spec[0].sd_name / "connection").get();
    _tx_fe->set_mux(conn);
}

/***********************************************************************
 * Async Recv
 **********************************************************************/
bool e100_impl::recv_async_msg(
    async_metadata_t &async_metadata, double timeout
){
    boost::this_thread::disable_interruption di; //disable because the wait can throw
    return _io_impl->async_msg_fifo.pop_with_timed_wait(async_metadata, timeout);
}

/***********************************************************************
 * Receive streamer
 **********************************************************************/
rx_streamer::sptr e100_impl::get_rx_stream(const uhd::stream_args_t &args_){
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
        my_streamer->set_xport_chan_get_buff(chan_i, boost::bind(
            &recv_packet_demuxer::get_recv_buff, _io_impl->demuxer, dsp, _1
        ), true /*flush*/);
        my_streamer->set_overflow_handler(chan_i, boost::bind(
            &rx_dsp_core_200::handle_overflow, _rx_dsps[dsp]
        ));
        _rx_streamers[dsp] = my_streamer; //store weak pointer
    }

    //sets all tick and samp rates on this streamer
    this->update_rates();

    return my_streamer;
}

/***********************************************************************
 * Transmit streamer
 **********************************************************************/
tx_streamer::sptr e100_impl::get_tx_stream(const uhd::stream_args_t &args_){
    stream_args_t args = args_;

    //setup defaults for unspecified values
    args.otw_format = args.otw_format.empty()? "sc16" : args.otw_format;
    args.channels = args.channels.empty()? std::vector<size_t>(1, 0) : args.channels;

    //calculate packet size
    static const size_t hdr_size = 0
        + vrt_send_header_offset_words32*sizeof(boost::uint32_t)
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
    my_streamer->set_vrt_packer(&vrt::if_hdr_pack_le, vrt_send_header_offset_words32);

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
        _tx_streamers[dsp] = my_streamer; //store weak pointer
    }

    //sets all tick and samp rates on this streamer
    this->update_rates();

    return my_streamer;
}
