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

#ifndef INCLUDED_LIBUHD_TRANSPORT_VRT_PACKET_HANDLER_STATE_HPP
#define INCLUDED_LIBUHD_TRANSPORT_VRT_PACKET_HANDLER_STATE_HPP

#include <uhd/config.hpp>
#include <uhd/transport/zero_copy.hpp>
#include <boost/asio/buffer.hpp>

namespace vrt_packet_handler{

    struct recv_state{
        //init the expected seq number
        size_t next_packet_seq;

        //state variables to handle fragments
        uhd::transport::managed_recv_buffer::sptr managed_buff;
        boost::asio::const_buffer copy_buff;
        size_t fragment_offset_in_samps;

        recv_state(void){
            //first expected seq is zero
            next_packet_seq = 0;

            //initially empty copy buffer
            copy_buff = boost::asio::buffer("", 0);
        }
    };

    struct send_state{
        //init the expected seq number
        size_t next_packet_seq;

        send_state(void){
            next_packet_seq = 0;
        }
    };

} //namespace vrt_packet_handler

#endif /* INCLUDED_LIBUHD_TRANSPORT_VRT_PACKET_HANDLER_STATE_HPP */
