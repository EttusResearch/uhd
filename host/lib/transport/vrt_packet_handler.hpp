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

#include <uhd/config.hpp>
#include <uhd/device.hpp>
#include <uhd/types/io_type.hpp>
#include <uhd/types/otw_type.hpp>
#include <uhd/types/metadata.hpp>
#include <uhd/transport/vrt.hpp>
#include <uhd/transport/convert_types.hpp>
#include <uhd/transport/zero_copy.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/function.hpp>
#include <stdexcept>
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

    static UHD_INLINE void recv_cb_nop(uhd::transport::managed_recv_buffer::sptr){
        /* NOP */
    }

    /*******************************************************************
     * Unpack a received vrt header and set the copy buffer.
     *  - helper function for vrt_packet_handler::_recv1
     ******************************************************************/
    static UHD_INLINE void _recv1_helper(
        recv_state &state,
        uhd::rx_metadata_t &metadata,
        double tick_rate,
        size_t vrt_header_offset_words32
    ){
        size_t num_packet_words32 = state.managed_buff->size()/sizeof(boost::uint32_t);
        if (num_packet_words32 <= vrt_header_offset_words32){
            state.copy_buff = boost::asio::buffer("", 0);
            return; //must exit here after setting the buffer
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

    /*******************************************************************
     * Recv data, unpack a vrt header, and copy-convert the data.
     *  - helper function for vrt_packet_handler::recv
     ******************************************************************/
    static UHD_INLINE size_t _recv1(
        recv_state &state,
        void *recv_mem,
        size_t total_samps,
        uhd::rx_metadata_t &metadata,
        const uhd::io_type_t &io_type,
        const uhd::otw_type_t &otw_type,
        double tick_rate,
        uhd::transport::zero_copy_if::sptr zc_iface,
        //use these two params to handle a layer above vrt
        size_t vrt_header_offset_words32,
        const recv_cb_t& recv_cb
    ){
        //perform a receive if no rx data is waiting to be copied
        if (boost::asio::buffer_size(state.copy_buff) == 0){
            state.fragment_offset_in_samps = 0;
            state.managed_buff = zc_iface->get_recv_buff();
            recv_cb(state.managed_buff); //callback before vrt unpack
            _recv1_helper(
                state, metadata, tick_rate, vrt_header_offset_words32
            );
        }

        //extract the number of samples available to copy
        size_t bytes_per_item = otw_type.get_sample_size();
        size_t bytes_available = boost::asio::buffer_size(state.copy_buff);
        size_t num_samps = std::min(total_samps, bytes_available/bytes_per_item);

        //setup the fragment flags and offset
        metadata.more_fragments = total_samps < num_samps;
        metadata.fragment_offset = state.fragment_offset_in_samps;
        state.fragment_offset_in_samps += num_samps; //set for next call

        //copy-convert the samples from the recv buffer
        uhd::transport::convert_otw_type_to_io_type(
            boost::asio::buffer_cast<const void*>(state.copy_buff), otw_type,
            recv_mem, io_type, num_samps
        );

        //update the rx copy buffer to reflect the bytes copied
        size_t bytes_copied = num_samps*bytes_per_item;
        state.copy_buff = boost::asio::buffer(
            boost::asio::buffer_cast<const boost::uint8_t*>(state.copy_buff) + bytes_copied,
            bytes_available - bytes_copied
        );

        return num_samps;
    }

    /*******************************************************************
     * Recv vrt packets and copy convert the samples into the buffer.
     ******************************************************************/
    static UHD_INLINE size_t recv(
        recv_state &state,
        const boost::asio::mutable_buffer &buff,
        uhd::rx_metadata_t &metadata,
        uhd::device::recv_mode_t recv_mode,
        const uhd::io_type_t &io_type,
        const uhd::otw_type_t &otw_type,
        double tick_rate,
        uhd::transport::zero_copy_if::sptr zc_iface,
        //use these two params to handle a layer above vrt
        size_t vrt_header_offset_words32 = 0,
        const recv_cb_t& recv_cb = &recv_cb_nop
    ){
        metadata = uhd::rx_metadata_t(); //init the metadata
        const size_t total_num_samps = boost::asio::buffer_size(buff)/io_type.size;

        switch(recv_mode){

        ////////////////////////////////////////////////////////////////
        case uhd::device::RECV_MODE_ONE_PACKET:{
        ////////////////////////////////////////////////////////////////
            return _recv1(
                state,
                boost::asio::buffer_cast<void *>(buff),
                total_num_samps,
                metadata,
                io_type, otw_type,
                tick_rate,
                zc_iface,
                vrt_header_offset_words32,
                recv_cb
            );
        }

        ////////////////////////////////////////////////////////////////
        case uhd::device::RECV_MODE_FULL_BUFF:{
        ////////////////////////////////////////////////////////////////
            size_t accum_num_samps = 0;
            uhd::rx_metadata_t tmp_md;
            while(accum_num_samps < total_num_samps){
                size_t num_samps = _recv1(
                    state,
                    boost::asio::buffer_cast<boost::uint8_t *>(buff) + (num_samps*io_type.size),
                    total_num_samps - num_samps,
                    (accum_num_samps == 0)? metadata : tmp_md, //only the first metadata gets kept
                    io_type, otw_type,
                    tick_rate,
                    zc_iface,
                    vrt_header_offset_words32,
                    recv_cb
                );
                if (num_samps == 0) break; //had a recv timeout or error, break loop
                accum_num_samps += num_samps;
            }
            return accum_num_samps;
        }

        default: throw std::runtime_error("unknown recv mode");
        }//switch(recv_mode)
    }

/***********************************************************************
 * vrt packet handler for send
 **********************************************************************/
    struct send_state{
        //init the expected seq number
        size_t next_packet_seq;

        send_state(void){
            next_packet_seq = 0;
        }
    };

    typedef boost::function<void(uhd::transport::managed_send_buffer::sptr)> send_cb_t;

    static UHD_INLINE void send_cb_nop(uhd::transport::managed_send_buffer::sptr){
        /* NOP */
    }

    /*******************************************************************
     * Pack a vrt header, copy-convert the data, and send it.
     *  - helper function for vrt_packet_handler::send
     ******************************************************************/
    static UHD_INLINE void _send1(
        send_state &state,
        const void *send_mem,
        size_t num_samps,
        const uhd::tx_metadata_t &metadata,
        const uhd::io_type_t &io_type,
        const uhd::otw_type_t &otw_type,
        double tick_rate,
        uhd::transport::zero_copy_if::sptr zc_iface,
        size_t vrt_header_offset_words32,
        const send_cb_t& send_cb
    ){
        //get a new managed send buffer
        uhd::transport::managed_send_buffer::sptr send_buff = zc_iface->get_send_buff();
        boost::uint32_t *tx_mem = send_buff->cast<boost::uint32_t *>() + vrt_header_offset_words32;

        size_t num_header_words32, num_packet_words32;
        size_t packet_count = state.next_packet_seq++;

        //pack metadata into a vrt header
        uhd::transport::vrt::pack(
            metadata,            //input
            tx_mem,              //output
            num_header_words32,  //output
            num_samps,           //input
            num_packet_words32,  //output
            packet_count,        //input
            tick_rate
        );

        //copy-convert the samples into the send buffer
        uhd::transport::convert_io_type_to_otw_type(
            send_mem, io_type,
            tx_mem + num_header_words32, otw_type,
            num_samps
        );

        send_cb(send_buff); //callback after memory filled

        //commit the samples to the zero-copy interface
        send_buff->done(num_packet_words32*sizeof(boost::uint32_t));
    }

    /*******************************************************************
     * Send vrt packets and copy convert the samples into the buffer.
     ******************************************************************/
    static UHD_INLINE size_t send(
        send_state &state,
        const boost::asio::const_buffer &buff,
        const uhd::tx_metadata_t &metadata,
        uhd::device::send_mode_t send_mode,
        const uhd::io_type_t &io_type,
        const uhd::otw_type_t &otw_type,
        double tick_rate,
        uhd::transport::zero_copy_if::sptr zc_iface,
        size_t max_samples_per_packet,
        //use these two params to handle a layer above vrt
        size_t vrt_header_offset_words32 = 0,
        const send_cb_t& send_cb = &send_cb_nop
    ){
        const size_t total_num_samps = boost::asio::buffer_size(buff)/io_type.size;
        switch(send_mode){

        ////////////////////////////////////////////////////////////////
        case uhd::device::SEND_MODE_ONE_PACKET:{
        ////////////////////////////////////////////////////////////////
            size_t num_samps = std::min(total_num_samps, max_samples_per_packet);
            _send1(
                state,
                boost::asio::buffer_cast<const void *>(buff),
                num_samps,
                metadata,
                io_type, otw_type,
                tick_rate,
                zc_iface,
                vrt_header_offset_words32,
                send_cb
            );
            return num_samps;
        }

        ////////////////////////////////////////////////////////////////
        case uhd::device::SEND_MODE_FULL_BUFF:{
        ////////////////////////////////////////////////////////////////
            //calculate constants for fragmentation
            const size_t final_packet_samps = total_num_samps%max_samples_per_packet;
            const size_t num_fragments = (total_num_samps+max_samples_per_packet-1)/max_samples_per_packet;
            static const size_t first_fragment_index = 0;
            const size_t final_fragment_index = num_fragments-1;

            //make a rw copy of the metadata to re-flag below
            uhd::tx_metadata_t md(metadata);

            //loop through the following fragment indexes
            for (size_t n = first_fragment_index; n <= final_fragment_index; n++){

                //calculate new flags for the fragments
                md.has_time_spec  = md.has_time_spec  and (n == first_fragment_index);
                md.start_of_burst = md.start_of_burst and (n == first_fragment_index);
                md.end_of_burst   = md.end_of_burst   and (n == final_fragment_index);

                //send the fragment with the helper function
                _send1(
                    state,
                    boost::asio::buffer_cast<const boost::uint8_t *>(buff) + (n*max_samples_per_packet*io_type.size),
                    (n == final_fragment_index)?final_packet_samps:max_samples_per_packet,
                    md,
                    io_type, otw_type,
                    tick_rate,
                    zc_iface,
                    vrt_header_offset_words32,
                    send_cb
                );
            }
            return total_num_samps;
        }

        default: throw std::runtime_error("unknown send mode");
        }//switch(send_mode)
    }

} //namespace vrt_packet_handler

#endif /* INCLUDED_LIBUHD_TRANSPORT_VRT_PACKET_HANDLER_HPP */
