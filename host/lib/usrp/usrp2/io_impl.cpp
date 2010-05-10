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

#include "usrp2_impl.hpp"
#include "usrp2_regs.hpp"
#include <uhd/transport/convert_types.hpp>
#include <boost/format.hpp>
#include <boost/asio.hpp> //htonl and ntohl
#include <iostream>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;
namespace asio = boost::asio;

/***********************************************************************
 * Helper Functions
 **********************************************************************/
void usrp2_impl::io_init(void){
    //setup otw type
    _otw_type.width = 16;
    _otw_type.shift = 0;
    _otw_type.byteorder = otw_type_t::BO_BIG_ENDIAN;

    //initially empty copy buffer
    _rx_copy_buff = asio::buffer("", 0);

    //init the expected rx seq number
    _rx_stream_id_to_packet_seq[0] = 0;

    //send a small data packet so the usrp2 knows the udp source port
    managed_send_buffer::sptr send_buff = _data_transport->get_send_buff();
    boost::uint32_t data = htonl(USRP2_INVALID_VRT_HEADER);
    memcpy(send_buff->cast<void*>(), &data, sizeof(data));
    send_buff->done(sizeof(data));

    //setup RX DSP regs
    _iface->poke32(FR_RX_CTRL_NSAMPS_PER_PKT, _max_rx_samples_per_packet);
    _iface->poke32(FR_RX_CTRL_NCHANNELS, 1);
    _iface->poke32(FR_RX_CTRL_CLEAR_OVERRUN, 1); //reset
    _iface->poke32(FR_RX_CTRL_VRT_HEADER, 0
        | (0x1 << 28) //if data with stream id
        | (0x1 << 26) //has trailer
        | (0x3 << 22) //integer time other
        | (0x1 << 20) //fractional time sample count
    );
    _iface->poke32(FR_RX_CTRL_VRT_STREAM_ID, 0);
    _iface->poke32(FR_RX_CTRL_VRT_TRAILER, 0);
}

/***********************************************************************
 * Receive Raw Data
 **********************************************************************/
void usrp2_impl::recv_raw(rx_metadata_t &metadata){
    //do a receive
    _rx_smart_buff = _data_transport->get_recv_buff();

    //unpack the vrt header
    size_t num_packet_words32 = _rx_smart_buff->size()/sizeof(boost::uint32_t);
    if (num_packet_words32 == 0){
        _rx_copy_buff = boost::asio::buffer("", 0);
        return; //must exit here after setting the buffer
    }
    const boost::uint32_t *vrt_hdr = _rx_smart_buff->cast<const boost::uint32_t *>();
    size_t num_header_words32_out, num_payload_words32_out, packet_count_out;
    try{
        vrt::unpack(
            metadata,                //output
            vrt_hdr,                 //input
            num_header_words32_out,  //output
            num_payload_words32_out, //output
            num_packet_words32,      //input
            packet_count_out,        //output
            get_master_clock_freq()
        );
    }catch(const std::exception &e){
        std::cerr << "bad vrt header: " << e.what() << std::endl;
        _rx_copy_buff = boost::asio::buffer("", 0);
        return; //must exit here after setting the buffer
    }

    //handle the packet count / sequence number
    size_t expected_packet_count = _rx_stream_id_to_packet_seq[metadata.stream_id];
    if (packet_count_out != expected_packet_count){
        std::cerr << "S" << (packet_count_out - expected_packet_count)%16;
    }
    _rx_stream_id_to_packet_seq[metadata.stream_id] = (packet_count_out+1)%16;

    //setup the rx buffer to point to the data
    _rx_copy_buff = asio::buffer(
        vrt_hdr + num_header_words32_out,
        num_payload_words32_out*sizeof(boost::uint32_t)
    );
}

/***********************************************************************
 * Send Data
 **********************************************************************/
size_t usrp2_impl::send(
    const asio::const_buffer &buff,
    const tx_metadata_t &metadata_,
    const io_type_t &io_type
){
    tx_metadata_t metadata = metadata_; //rw copy to change later

    transport::managed_send_buffer::sptr send_buff = _data_transport->get_send_buff();
    boost::uint32_t *tx_mem = send_buff->cast<boost::uint32_t *>();
    size_t num_samps = std::min(std::min(
        asio::buffer_size(buff)/io_type.size,
        size_t(_max_tx_samples_per_packet)),
        send_buff->size()/io_type.size
    );

    //kill the end of burst flag if this is a fragment
    if (asio::buffer_size(buff)/io_type.size < num_samps)
        metadata.end_of_burst = false;

    size_t num_header_words32, num_packet_words32;
    size_t packet_count = _tx_stream_id_to_packet_seq[metadata.stream_id]++;

    //pack metadata into a vrt header
    vrt::pack(
        metadata,            //input
        tx_mem,              //output
        num_header_words32,  //output
        num_samps,           //input
        num_packet_words32,  //output
        packet_count,        //input
        get_master_clock_freq()
    );

    boost::uint32_t *items = tx_mem + num_header_words32; //offset for data

    //copy-convert the samples into the send buffer
    convert_io_type_to_otw_type(
        asio::buffer_cast<const void*>(buff), io_type,
        (void*)items, _otw_type,
        num_samps
    );

    //send and return number of samples
    send_buff->done(num_packet_words32*sizeof(boost::uint32_t));
    return num_samps;
}

/***********************************************************************
 * Receive Data
 **********************************************************************/
size_t usrp2_impl::recv(
    const asio::mutable_buffer &buff,
    rx_metadata_t &metadata,
    const io_type_t &io_type
){
    //perform a receive if no rx data is waiting to be copied
    if (asio::buffer_size(_rx_copy_buff) == 0){
        _fragment_offset_in_samps = 0;
        recv_raw(metadata);
    }
    //otherwise flag the metadata to show that is is a fragment
    else{
        metadata = rx_metadata_t();
    }

    //extract the number of samples available to copy
    //and a pointer into the usrp2 received items memory
    size_t bytes_to_copy = asio::buffer_size(_rx_copy_buff);
    if (bytes_to_copy == 0) return 0; //nothing to receive
    size_t num_samps = std::min(
        asio::buffer_size(buff)/io_type.size,
        bytes_to_copy/sizeof(boost::uint32_t)
    );
    const boost::uint32_t *items = asio::buffer_cast<const boost::uint32_t*>(_rx_copy_buff);

    //setup the fragment flags and offset
    metadata.more_fragments = asio::buffer_size(buff)/io_type.size < num_samps;
    metadata.fragment_offset = _fragment_offset_in_samps;
    _fragment_offset_in_samps += num_samps; //set for next time

    //copy-convert the samples from the recv buffer
    convert_otw_type_to_io_type(
        (const void*)items, _otw_type,
        asio::buffer_cast<void*>(buff), io_type,
        num_samps
    );

    //update the rx copy buffer to reflect the bytes copied
    _rx_copy_buff = asio::buffer(
        items + num_samps, bytes_to_copy - num_samps*sizeof(boost::uint32_t)
    );

    return num_samps;
}
