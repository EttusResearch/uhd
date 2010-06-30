//
// Copyright 2010 Ettus Research LLC
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

#include "../../transport/vrt_packet_handler.hpp"
#include "usrp2_impl.hpp"
#include "usrp2_regs.hpp"
#include <uhd/utils/thread_priority.hpp>
#include <uhd/transport/convert_types.hpp>
#include <uhd/transport/bounded_buffer.hpp>
#include <boost/format.hpp>
#include <boost/asio.hpp> //htonl and ntohl
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <iostream>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;
namespace asio = boost::asio;

/***********************************************************************
 * io impl details (internal to this file)
 **********************************************************************/
struct usrp2_impl::io_impl{

    io_impl(zero_copy_if::sptr zc_if);
    ~io_impl(void);

    managed_recv_buffer::sptr get_recv_buff(void);

    //state management for the vrt packet handler code
    vrt_packet_handler::recv_state packet_handler_recv_state;
    vrt_packet_handler::send_state packet_handler_send_state;

    //methods and variables for the recv pirate
    void recv_pirate_loop(zero_copy_if::sptr zc_if);
    boost::thread *recv_pirate_thread; bool recv_pirate_running;
    bounded_buffer<managed_recv_buffer::sptr>::sptr recv_pirate_booty;
};

usrp2_impl::io_impl::io_impl(zero_copy_if::sptr zc_if): packet_handler_recv_state(1){
    //create a large enough booty
    size_t num_frames = zc_if->get_num_recv_frames();
    std::cout << "Recv pirate num frames: " << num_frames << std::endl;
    recv_pirate_booty = bounded_buffer<managed_recv_buffer::sptr>::make(num_frames);

    //create a new pirate thread (yarr!!)
    recv_pirate_thread = new boost::thread(
        boost::bind(&usrp2_impl::io_impl::recv_pirate_loop, this, zc_if)
    );
}

usrp2_impl::io_impl::~io_impl(void){
    recv_pirate_running = false;
    recv_pirate_thread->interrupt();
    recv_pirate_thread->join();
    delete recv_pirate_thread;
}

managed_recv_buffer::sptr usrp2_impl::io_impl::get_recv_buff(void){
    managed_recv_buffer::sptr buff;
    boost::this_thread::disable_interruption di; //disable because the wait can throw
    recv_pirate_booty->pop_with_timed_wait(buff, boost::posix_time::milliseconds(100));
    return buff; //a timeout means that we return a null sptr...
}

void usrp2_impl::io_impl::recv_pirate_loop(zero_copy_if::sptr zc_if){
    set_thread_priority_safe();
    recv_pirate_running = true;
    while(recv_pirate_running){
        managed_recv_buffer::sptr buff = zc_if->get_recv_buff();
        if (buff->size()) recv_pirate_booty->push_with_pop_on_full(buff);
    }
}

/***********************************************************************
 * Helper Functions
 **********************************************************************/
void usrp2_impl::io_init(void){
    //setup rx otw type
    _rx_otw_type.width = 16;
    _rx_otw_type.shift = 0;
    _rx_otw_type.byteorder = otw_type_t::BO_BIG_ENDIAN;

    //setup tx otw type
    _tx_otw_type.width = 16;
    _tx_otw_type.shift = 0;
    _tx_otw_type.byteorder = otw_type_t::BO_BIG_ENDIAN;

    //send a small data packet so the usrp2 knows the udp source port
    managed_send_buffer::sptr send_buff = _data_transport->get_send_buff();
    boost::uint32_t data = htonl(USRP2_INVALID_VRT_HEADER);
    memcpy(send_buff->cast<void*>(), &data, sizeof(data));
    send_buff->commit(sizeof(data));

    //setup RX DSP regs
    std::cout << "RX samples per packet: " << get_max_recv_samps_per_packet() << std::endl;
    _iface->poke32(U2_REG_RX_CTRL_NSAMPS_PER_PKT, get_max_recv_samps_per_packet());
    _iface->poke32(U2_REG_RX_CTRL_NCHANNELS, 1);
    _iface->poke32(U2_REG_RX_CTRL_CLEAR_OVERRUN, 1); //reset
    _iface->poke32(U2_REG_RX_CTRL_VRT_HEADER, 0
        | (0x1 << 28) //if data with stream id
        | (0x1 << 26) //has trailer
        | (0x3 << 22) //integer time other
        | (0x1 << 20) //fractional time sample count
    );
    _iface->poke32(U2_REG_RX_CTRL_VRT_STREAM_ID, 0);
    _iface->poke32(U2_REG_RX_CTRL_VRT_TRAILER, 0);

    std::cout << "TX samples per packet: " << get_max_send_samps_per_packet() << std::endl;

    //create new io impl
    _io_impl = UHD_PIMPL_MAKE(io_impl, (_data_transport));
}

/***********************************************************************
 * Send Data
 **********************************************************************/
bool tmp_todo_fixme_remove_get_send_buffs(vrt_packet_handler::managed_send_buffs_t &buffs, const zero_copy_if::sptr &zc_if){
    buffs[0] = zc_if->get_send_buff();
    return buffs[0].get() != NULL;
}

size_t usrp2_impl::send(
    const std::vector<const void *> &buffs,
    size_t nsamps_per_buff,
    const tx_metadata_t &metadata,
    const io_type_t &io_type,
    send_mode_t send_mode
){
    return vrt_packet_handler::send(
        _io_impl->packet_handler_send_state, //last state of the send handler
        buffs, nsamps_per_buff,     //buffer to empty
        metadata, send_mode,        //samples metadata
        io_type, _tx_otw_type,      //input and output types to convert
        get_master_clock_freq(),    //master clock tick rate
        uhd::transport::vrt::if_hdr_pack_be,
        boost::bind(&tmp_todo_fixme_remove_get_send_buffs, _1, _data_transport),
        get_max_send_samps_per_packet()
    );
}

/***********************************************************************
 * Receive Data
 **********************************************************************/
bool tmp_todo_fixme_remove_get_recv_buffs(vrt_packet_handler::managed_recv_buffs_t &buffs, boost::shared_ptr<usrp2_impl::io_impl> impl){
    buffs[0] = impl->get_recv_buff();
    return buffs[0].get() != NULL;
}

size_t usrp2_impl::recv(
    const std::vector<void *> &buffs,
    size_t nsamps_per_buff,
    rx_metadata_t &metadata,
    const io_type_t &io_type,
    recv_mode_t recv_mode
){
    return vrt_packet_handler::recv(
        _io_impl->packet_handler_recv_state, //last state of the recv handler
        buffs, nsamps_per_buff,     //buffer to empty
        metadata, recv_mode,        //samples metadata
        io_type, _rx_otw_type,      //input and output types to convert
        get_master_clock_freq(),    //master clock tick rate
        uhd::transport::vrt::if_hdr_unpack_be,
        boost::bind(&tmp_todo_fixme_remove_get_recv_buffs, _1, _io_impl)
    );
}
