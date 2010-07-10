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
 * - pirate crew
 * - alignment buffer
 * - thread loop
 * - vrt packet handler states
 **********************************************************************/
struct usrp2_impl::io_impl{
    typedef alignment_buffer<managed_recv_buffer::sptr, time_spec_t> alignment_buffer_type;

    io_impl(size_t num_frames, size_t width):
        packet_handler_recv_state(width),
        recv_pirate_booty(alignment_buffer_type::make(num_frames, width))
    {
        /* NOP */
    }

    ~io_impl(void){
        recv_pirate_crew_raiding = false;
        recv_pirate_crew.interrupt_all();
        recv_pirate_crew.join_all();
    }

    bool get_recv_buffs(vrt_packet_handler::managed_recv_buffs_t &buffs){
        boost::this_thread::disable_interruption di; //disable because the wait can throw
        return recv_pirate_booty->pop_elems_with_timed_wait(buffs, boost::posix_time::milliseconds(100));
    }

    //state management for the vrt packet handler code
    vrt_packet_handler::recv_state packet_handler_recv_state;
    vrt_packet_handler::send_state packet_handler_send_state;

    //methods and variables for the pirate crew
    void recv_pirate_loop(zero_copy_if::sptr, usrp2_mboard_impl::sptr, size_t);
    boost::thread_group recv_pirate_crew;
    bool recv_pirate_crew_raiding;
    alignment_buffer_type::sptr recv_pirate_booty;
};

/***********************************************************************
 * Receive Pirate Loop
 * - while raiding, loot for recv buffers
 * - put booty into the alignment buffer
 **********************************************************************/
void usrp2_impl::io_impl::recv_pirate_loop(
    zero_copy_if::sptr zc_if,
    usrp2_mboard_impl::sptr mboard,
    size_t index
){
    set_thread_priority_safe();
    recv_pirate_crew_raiding = true;
    size_t next_packet_seq = 0;

    while(recv_pirate_crew_raiding){
        managed_recv_buffer::sptr buff = zc_if->get_recv_buff();
        if (not buff.get()) continue; //ignore timeout/error buffers

        try{
            //extract the vrt header packet info
            vrt::if_packet_info_t if_packet_info;
            if_packet_info.num_packet_words32 = buff->size()/sizeof(boost::uint32_t);
            vrt::if_hdr_unpack_be(buff->cast<const boost::uint32_t *>(), if_packet_info);

            //handle the packet count / sequence number
            if (if_packet_info.packet_count != next_packet_seq){
                //std::cerr << "S" << (if_packet_info.packet_count - next_packet_seq)%16;
                std::cerr << "O"; //report overrun (drops in the kernel)
            }
            next_packet_seq = (if_packet_info.packet_count+1)%16;

            //extract the timespec and round to the nearest packet
            UHD_ASSERT_THROW(if_packet_info.has_tsi and if_packet_info.has_tsf);
            time_spec_t time(
                time_t(if_packet_info.tsi), size_t(if_packet_info.tsf), mboard->get_master_clock_freq()
            );

            //push the packet into the buffer with the new time
            recv_pirate_booty->push_with_pop_on_full(buff, time, index);
        }catch(const std::exception &e){
            std::cerr << "Error (usrp2 recv pirate loop): " << e.what() << std::endl;
        }
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

    //the number of recv frames is the number for the first transport
    //the assumption is that all data transports should be identical
    size_t num_frames = _data_transports.front()->get_num_recv_frames();

    //create new io impl
    _io_impl = UHD_PIMPL_MAKE(io_impl, (num_frames, _data_transports.size()));

    //create a new pirate thread for each zc if (yarr!!)
    for (size_t i = 0; i < _data_transports.size(); i++){
        _io_impl->recv_pirate_crew.create_thread(boost::bind(
            &usrp2_impl::io_impl::recv_pirate_loop,
            _io_impl.get(), _data_transports.at(i),
            _mboards.at(i), i
        ));
    }

    std::cout << "RX samples per packet: " << get_max_recv_samps_per_packet() << std::endl;
    std::cout << "TX samples per packet: " << get_max_send_samps_per_packet() << std::endl;
    std::cout << "Recv pirate num frames: " << num_frames << std::endl;
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
        boost::bind(&usrp2_impl::io_impl::get_recv_buffs, _io_impl.get(), _1)
    );
}
