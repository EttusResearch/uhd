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
#include <uhd/transport/alignment_buffer.hpp>
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
    typedef alignment_buffer<managed_recv_buffer::sptr, time_spec_t> alignment_buffer_type;

    io_impl(std::vector<udp_zero_copy::sptr> &zc_ifs);
    ~io_impl(void);

    bool get_recv_buffs(vrt_packet_handler::managed_recv_buffs_t &buffs);

    //state management for the vrt packet handler code
    vrt_packet_handler::recv_state packet_handler_recv_state;
    vrt_packet_handler::send_state packet_handler_send_state;

    //methods and variables for the recv pirate
    void recv_pirate_loop(zero_copy_if::sptr zc_if, size_t index);
    boost::thread_group recv_pirate_crew;
    bool recv_pirate_crew_running;
    alignment_buffer_type::sptr recv_pirate_booty;
};

usrp2_impl::io_impl::io_impl(std::vector<udp_zero_copy::sptr> &zc_ifs):
    packet_handler_recv_state(zc_ifs.size())
{
    //create a large enough booty
    size_t num_frames = zc_ifs.at(0)->get_num_recv_frames();
    std::cout << "Recv pirate num frames: " << num_frames << std::endl;
    recv_pirate_booty = alignment_buffer_type::make(num_frames, zc_ifs.size());

    //create a new pirate thread for each zc if (yarr!!)
    for (size_t i = 0; i < zc_ifs.size(); i++){
        recv_pirate_crew.create_thread(
            boost::bind(&usrp2_impl::io_impl::recv_pirate_loop, this, zc_ifs.at(i), i)
        );
    }
}

usrp2_impl::io_impl::~io_impl(void){
    recv_pirate_crew_running = false;
    recv_pirate_crew.interrupt_all();
    recv_pirate_crew.join_all();
}

bool usrp2_impl::io_impl::get_recv_buffs(vrt_packet_handler::managed_recv_buffs_t &buffs){
    boost::this_thread::disable_interruption di; //disable because the wait can throw
    return recv_pirate_booty->pop_elems_with_timed_wait(buffs, boost::posix_time::milliseconds(100));
}

void usrp2_impl::io_impl::recv_pirate_loop(zero_copy_if::sptr zc_if, size_t index){
    set_thread_priority_safe();
    recv_pirate_crew_running = true;
    while(recv_pirate_crew_running){
        managed_recv_buffer::sptr buff = zc_if->get_recv_buff();
        //TODO unpack vrt, get time spec, round to nearest packet bound, use below:
        if (buff->size()) recv_pirate_booty->push_with_pop_on_full(buff, time_spec_t(/*todoseq*/), index);
    }
}

/***********************************************************************
 * Helper Functions
 **********************************************************************/
void usrp2_impl::io_init(void){
    //send a small data packet so the usrp2 knows the udp source port
    for(size_t i = 0; i < _data_transports.size(); i++){
        managed_send_buffer::sptr send_buff = _data_transports[i]->get_send_buff();
        boost::uint32_t data = htonl(USRP2_INVALID_VRT_HEADER);
        memcpy(send_buff->cast<void*>(), &data, sizeof(data));
        send_buff->commit(sizeof(data));
    }

    std::cout << "RX samples per packet: " << get_max_recv_samps_per_packet() << std::endl;
    std::cout << "TX samples per packet: " << get_max_send_samps_per_packet() << std::endl;

    //create new io impl
    _io_impl = UHD_PIMPL_MAKE(io_impl, (_data_transports));
}

/***********************************************************************
 * Send Data
 **********************************************************************/
bool get_send_buffs(
    const std::vector<udp_zero_copy::sptr> &trans,
    vrt_packet_handler::managed_send_buffs_t &buffs
){
    UHD_ASSERT_THROW(trans.size() == buffs.size());
    for (size_t i = 0; i < buffs.size(); i++){
        buffs[i] = trans[i]->get_send_buff();
    }
    return true;
}

size_t usrp2_impl::send(
    const std::vector<const void *> &buffs,
    size_t num_samps,
    const tx_metadata_t &metadata,
    const io_type_t &io_type,
    send_mode_t send_mode
){
    return vrt_packet_handler::send(
        _io_impl->packet_handler_send_state,       //last state of the send handler
        buffs, num_samps,                          //buffer to fill
        metadata, send_mode,                       //samples metadata
        io_type, _io_helper.get_tx_otw_type(),     //input and output types to convert
        _mboards.front()->get_master_clock_freq(), //master clock tick rate
        uhd::transport::vrt::if_hdr_pack_be,
        boost::bind(&get_send_buffs, _data_transports, _1),
        get_max_send_samps_per_packet()
    );
}

/***********************************************************************
 * Receive Data
 **********************************************************************/
size_t usrp2_impl::recv(
    const std::vector<void *> &buffs,
    size_t num_samps,
    rx_metadata_t &metadata,
    const io_type_t &io_type,
    recv_mode_t recv_mode
){
    return vrt_packet_handler::recv(
        _io_impl->packet_handler_recv_state,       //last state of the recv handler
        buffs, num_samps,                          //buffer to fill
        metadata, recv_mode,                       //samples metadata
        io_type, _io_helper.get_rx_otw_type(),     //input and output types to convert
        _mboards.front()->get_master_clock_freq(), //master clock tick rate
        uhd::transport::vrt::if_hdr_unpack_be,
        boost::bind(&usrp2_impl::io_impl::get_recv_buffs, _io_impl, _1)
    );
}
