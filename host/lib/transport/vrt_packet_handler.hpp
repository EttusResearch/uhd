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

#ifndef INCLUDED_LIBUHD_TRANSPORT_VRT_PACKET_HANDLER_HPP
#define INCLUDED_LIBUHD_TRANSPORT_VRT_PACKET_HANDLER_HPP

#include <uhd/types/io_type.hpp>
#include <uhd/types/otw_type.hpp>
#include <uhd/types/metadata.hpp>
#include <uhd/transport/vrt.hpp>
#include <uhd/transport/convert_types.hpp>
#include <uhd/transport/zero_copy.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/function.hpp>
#include <iostream>

namespace vrt_packet_handler{

/***********************************************************************
 * vrt packet handler for recv
 **********************************************************************/
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

    typedef boost::function<void(uhd::transport::managed_recv_buffer::sptr)> recv_cb_t;

    static inline void recv_cb_nop(uhd::transport::managed_recv_buffer::sptr){
        /* NOP */
    }

    static inline size_t recv(
        recv_state &state,
        const boost::asio::mutable_buffer &buff,
        uhd::rx_metadata_t &metadata,
        const uhd::io_type_t &io_type,
        const uhd::otw_type_t &otw_type,
        double tick_rate,
        uhd::transport::zero_copy_if::sptr zc_iface,
        size_t vrt_header_offset_words32 = 0,
        const recv_cb_t& recv_cb = &recv_cb_nop
    ){
        ////////////////////////////////////////////////////////////////
        // Perform the recv
        ////////////////////////////////////////////////////////////////
        {
            //perform a receive if no rx data is waiting to be copied
            if (boost::asio::buffer_size(state.copy_buff) == 0){
                state.fragment_offset_in_samps = 0;
                state.managed_buff = zc_iface->get_recv_buff();
                recv_cb(state.managed_buff);
            }
            //otherwise flag the metadata to show that is is a fragment
            else{
                metadata = uhd::rx_metadata_t();
                goto vrt_recv_copy_convert;
            }
        }

        ////////////////////////////////////////////////////////////////
        // Unpack the vrt header
        ////////////////////////////////////////////////////////////////
        {
            size_t num_packet_words32 = state.managed_buff->size()/sizeof(boost::uint32_t);
            if (num_packet_words32 <= vrt_header_offset_words32){
                state.copy_buff = boost::asio::buffer("", 0);
                goto vrt_recv_copy_convert; //must exit here after setting the buffer
            }
            const boost::uint32_t *vrt_hdr = state.managed_buff->cast<const boost::uint32_t *>() + vrt_header_offset_words32;
            size_t num_header_words32_out, num_payload_words32_out, packet_count_out;
            uhd::transport::vrt::unpack(
                metadata,                //output
                vrt_hdr,                 //input
                num_header_words32_out,  //output
                num_payload_words32_out, //output
                num_packet_words32,      //input
                packet_count_out,        //output
                tick_rate
            );

            //handle the packet count / sequence number
            if (packet_count_out != state.next_packet_seq){
                std::cerr << "S" << (packet_count_out - state.next_packet_seq)%16;
            }
            state.next_packet_seq = (packet_count_out+1)%16;

            //setup the buffer to point to the data
            state.copy_buff = boost::asio::buffer(
                vrt_hdr + num_header_words32_out,
                num_payload_words32_out*sizeof(boost::uint32_t)
            );
        }

        ////////////////////////////////////////////////////////////////
        // Perform copy-convert into buffer
        ////////////////////////////////////////////////////////////////
        vrt_recv_copy_convert:{
            size_t bytes_per_item = (otw_type.width * 2) / 8;

            //extract the number of samples available to copy
            //and a pointer into the usrp2 received items memory
            size_t bytes_available = boost::asio::buffer_size(state.copy_buff);
            size_t num_samps = std::min(
                boost::asio::buffer_size(buff)/io_type.size,
                bytes_available/bytes_per_item
            );

            //setup the fragment flags and offset
            metadata.more_fragments = boost::asio::buffer_size(buff)/io_type.size < num_samps;
            metadata.fragment_offset = state.fragment_offset_in_samps;
            state.fragment_offset_in_samps += num_samps; //set for next time

            //copy-convert the samples from the recv buffer
            uhd::transport::convert_otw_type_to_io_type(
                boost::asio::buffer_cast<const void*>(state.copy_buff), otw_type,
                boost::asio::buffer_cast<void*>(buff), io_type,
                num_samps
            );

            //update the rx copy buffer to reflect the bytes copied
            size_t bytes_copied = num_samps*bytes_per_item;
            state.copy_buff = boost::asio::buffer(
                boost::asio::buffer_cast<const boost::uint8_t*>(state.copy_buff) + bytes_copied,
                bytes_available - bytes_copied
            );

            return num_samps;
        }
    }

} //namespace vrt_packet_handler

#endif /* INCLUDED_LIBUHD_TRANSPORT_VRT_PACKET_HANDLER_HPP */
