//
// Copyright 2010-2011 Ettus Research LLC
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
#include "../../transport/super_recv_packet_handler.hpp"
#include "../../transport/super_send_packet_handler.hpp"
#include <linux/usrp_e.h> //ioctl structures and constants
#include "e100_impl.hpp"
#include "e100_regs.hpp"
#include <uhd/utils/msg.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/thread_priority.hpp>
#include <uhd/transport/bounded_buffer.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/barrier.hpp>
#include <poll.h> //poll
#include <fcntl.h> //open, close
#include <sstream>
#include <fstream>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;

/***********************************************************************
 * io impl details (internal to this file)
 * - pirate crew of 1
 * - bounded buffer
 * - thread loop
 * - vrt packet handler states
 **********************************************************************/
struct e100_impl::io_impl{
    io_impl(void):
        false_alarm(0), async_msg_fifo(100/*messages deep*/)
    { /* NOP */ }

    ~io_impl(void){
        recv_pirate_crew.interrupt_all();
        recv_pirate_crew.join_all();
    }

    double tick_rate; //set by update tick rate method
    e100_ctrl::sptr iface; //so handle irq can peek and poke
    void handle_irq(void);
    size_t false_alarm;
    //The data transport is listed first so that it is deconstructed last,
    //which is after the states and booty which may hold managed buffers.
    recv_packet_demuxer::sptr demuxer;

    //state management for the vrt packet handler code
    sph::recv_packet_handler recv_handler;
    sph::send_packet_handler send_handler;

    //a pirate's life is the life for me!
    void recv_pirate_loop(
        boost::barrier &spawn_barrier,
        spi_iface::sptr //keep a sptr to iface which shares gpio147
    ){
        spawn_barrier.wait();

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
    boost::thread_group recv_pirate_crew;
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
    //FIXME ioctl reads words32 incorrectly _fpga_ctrl->ioctl(USRP_E_READ_CTL32, &data);
    for (size_t i = 0; i < data.count; i++){
        data.buf[i] = iface->peek32(E100_REG_ERR_BUFF + i*sizeof(boost::uint32_t));
        //std::cout << boost::format("    buff[%u] = 0x%08x\n") % i % data.buf[i];
    }

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
        metadata.channel = 0;
        metadata.has_time_spec = if_packet_info.has_tsi and if_packet_info.has_tsf;
        metadata.time_spec = time_spec_t(
            time_t(if_packet_info.tsi), long(if_packet_info.tsf), tick_rate
        );
        metadata.event_code = async_metadata_t::event_code_t(sph::get_context_code(data.buf, if_packet_info));

        //push the message onto the queue
        async_msg_fifo.push_with_pop_on_full(metadata);

        //print some fastpath messages
        if (metadata.event_code &
            ( async_metadata_t::EVENT_CODE_UNDERFLOW
            | async_metadata_t::EVENT_CODE_UNDERFLOW_IN_PACKET)
        ) UHD_MSG(fastpath) << "U";
        else if (metadata.event_code &
            ( async_metadata_t::EVENT_CODE_SEQ_ERROR
            | async_metadata_t::EVENT_CODE_SEQ_ERROR_IN_BURST)
        ) UHD_MSG(fastpath) << "S";
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

    //setup rx otw type
    _rx_otw_type.width = 16;
    _rx_otw_type.shift = 0;
    _rx_otw_type.byteorder = uhd::otw_type_t::BO_LITTLE_ENDIAN;

    //setup tx otw type
    _tx_otw_type.width = 16;
    _tx_otw_type.shift = 0;
    _tx_otw_type.byteorder = uhd::otw_type_t::BO_LITTLE_ENDIAN;

    //create new io impl
    _io_impl = UHD_PIMPL_MAKE(io_impl, ());
    _io_impl->demuxer = recv_packet_demuxer::make(_data_transport, _rx_dsps.size(), E100_RX_SID_BASE);
    _io_impl->iface = _fpga_ctrl;

    //clear state machines
    _fpga_ctrl->poke32(E100_REG_CLEAR_RX, 0);
    _fpga_ctrl->poke32(E100_REG_CLEAR_TX, 0);

    //prepare the async msg buffer for incoming messages
    _fpga_ctrl->poke32(E100_REG_SR_ERR_CTRL, 1 << 0); //clear
    while ((_fpga_ctrl->peek32(E100_REG_RB_ERR_STATUS) & (1 << 2)) == 0){} //wait for idle
    _fpga_ctrl->poke32(E100_REG_SR_ERR_CTRL, 1 << 1); //start

    //spawn a pirate, yarrr!
    boost::barrier spawn_barrier(2);
    _io_impl->recv_pirate_crew.create_thread(boost::bind(
        &e100_impl::io_impl::recv_pirate_loop, _io_impl.get(),
        boost::ref(spawn_barrier), _aux_spi_iface
    ));
    spawn_barrier.wait();

    //init some handler stuff
    _io_impl->recv_handler.set_vrt_unpacker(&vrt::if_hdr_unpack_le);
    _io_impl->recv_handler.set_converter(_rx_otw_type);
    _io_impl->send_handler.set_vrt_packer(&vrt::if_hdr_pack_le);
    _io_impl->send_handler.set_converter(_tx_otw_type);
    _io_impl->send_handler.set_max_samples_per_packet(get_max_send_samps_per_packet());
}

void e100_impl::update_tick_rate(const double rate){
    _io_impl->tick_rate = rate;
    boost::mutex::scoped_lock recv_lock = _io_impl->recv_handler.get_scoped_lock();
    _io_impl->recv_handler.set_tick_rate(rate);
    boost::mutex::scoped_lock send_lock = _io_impl->send_handler.get_scoped_lock();
    _io_impl->send_handler.set_tick_rate(rate);
}

void e100_impl::update_rx_samp_rate(const double rate){
    boost::mutex::scoped_lock recv_lock = _io_impl->recv_handler.get_scoped_lock();
    _io_impl->recv_handler.set_samp_rate(rate);
    const double adj = _rx_dsps.front()->get_scaling_adjustment();
    _io_impl->recv_handler.set_scale_factor(adj/32767.);
}

void e100_impl::update_tx_samp_rate(const double rate){
    boost::mutex::scoped_lock send_lock = _io_impl->send_handler.get_scoped_lock();
    _io_impl->send_handler.set_samp_rate(rate);
}

void e100_impl::update_rx_subdev_spec(const uhd::usrp::subdev_spec_t &spec){
    boost::mutex::scoped_lock recv_lock = _io_impl->recv_handler.get_scoped_lock();
    property_tree::path_type root = "/mboards/0/dboards";

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

    //resize for the new occupancy
    _io_impl->recv_handler.resize(spec.size());

    //bind new callbacks for the handler
    for (size_t i = 0; i < _io_impl->recv_handler.size(); i++){
        _rx_dsps[i]->set_nsamps_per_packet(get_max_recv_samps_per_packet()); //seems to be a good place to set this
        _io_impl->recv_handler.set_xport_chan_get_buff(i, boost::bind(
            &recv_packet_demuxer::get_recv_buff, _io_impl->demuxer, i, _1
        ));
        _io_impl->recv_handler.set_overflow_handler(i, boost::bind(&rx_dsp_core_200::handle_overflow, _rx_dsps[i]));
    }
}

void e100_impl::update_tx_subdev_spec(const uhd::usrp::subdev_spec_t &spec){
    boost::mutex::scoped_lock send_lock = _io_impl->send_handler.get_scoped_lock();
    property_tree::path_type root = "/mboards/0/dboards";

    //sanity checking
    validate_subdev_spec(_tree, spec, "tx");

    //set the mux for this spec
    const std::string conn = _tree->access<std::string>(root / spec[0].db_name / "tx_frontends" / spec[0].sd_name / "connection").get();
    _tx_fe->set_mux(conn);

    //resize for the new occupancy
    _io_impl->send_handler.resize(spec.size());

    //bind new callbacks for the handler
    for (size_t i = 0; i < _io_impl->send_handler.size(); i++){
        _io_impl->send_handler.set_xport_chan_get_buff(i, boost::bind(
            &zero_copy_if::get_send_buff, _data_transport, _1
        ));
    }
}

/***********************************************************************
 * Data Send
 **********************************************************************/
size_t e100_impl::get_max_send_samps_per_packet(void) const{
    static const size_t hdr_size = 0
        + vrt::max_if_hdr_words32*sizeof(boost::uint32_t)
        - sizeof(vrt::if_packet_info_t().cid) //no class id ever used
    ;
    size_t bpp = _data_transport->get_send_frame_size() - hdr_size;
    return bpp/_tx_otw_type.get_sample_size();
}

size_t e100_impl::send(
    const send_buffs_type &buffs, size_t nsamps_per_buff,
    const tx_metadata_t &metadata, const io_type_t &io_type,
    send_mode_t send_mode, double timeout
){
    return _io_impl->send_handler.send(
        buffs, nsamps_per_buff,
        metadata, io_type,
        send_mode, timeout
    );
}

/***********************************************************************
 * Data Recv
 **********************************************************************/
size_t e100_impl::get_max_recv_samps_per_packet(void) const{
    static const size_t hdr_size = 0
        + vrt::max_if_hdr_words32*sizeof(boost::uint32_t)
        + sizeof(vrt::if_packet_info_t().tlr) //forced to have trailer
        - sizeof(vrt::if_packet_info_t().cid) //no class id ever used
    ;
    size_t bpp = _data_transport->get_recv_frame_size() - hdr_size;
    return bpp/_rx_otw_type.get_sample_size();
}

size_t e100_impl::recv(
    const recv_buffs_type &buffs, size_t nsamps_per_buff,
    rx_metadata_t &metadata, const io_type_t &io_type,
    recv_mode_t recv_mode, double timeout
){
    return _io_impl->recv_handler.recv(
        buffs, nsamps_per_buff,
        metadata, io_type,
        recv_mode, timeout
    );
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
