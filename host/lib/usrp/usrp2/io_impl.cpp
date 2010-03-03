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

#include <complex>
#include <boost/format.hpp>
#include "usrp2_impl.hpp"

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;
namespace asio = boost::asio;

/***********************************************************************
 * Constants
 **********************************************************************/
typedef std::complex<float>   fc32_t;
typedef std::complex<int16_t> sc16_t;

static const float shorts_per_float = pow(2.0, 15);
static const float floats_per_short = 1.0/shorts_per_float;

/***********************************************************************
 * Helper Functions
 **********************************************************************/
void usrp2_impl::io_init(void){
    //initially empty copy buffer
    _rx_copy_buff = asio::buffer("", 0);

    //send a small data packet so the usrp2 knows the udp source port
    uint32_t zero_data = 0;
    _data_transport->send(asio::buffer(&zero_data, sizeof(zero_data)));
}

#define unrolled_loop(__i, __len, __inst) {\
    size_t __i = 0; \
    while(__i < (__len & ~0x7)){ \
        __inst; __i++; __inst; __i++; \
        __inst; __i++; __inst; __i++; \
        __inst; __i++; __inst; __i++; \
        __inst; __i++; __inst; __i++; \
    } \
    while(__i < __len){ \
        __inst; __i++;\
    } \
}

// set a boolean flag that indicates the endianess
#ifdef HAVE_BIG_ENDIAN
static const bool is_big_endian = true;
#else
static const bool is_big_endian = false;
#endif

static inline void host_floats_to_usrp2_items(
    uint32_t *usrp2_items,
    const fc32_t *host_floats,
    size_t num_samps
){
    unrolled_loop(i, num_samps,{
        int16_t real = host_floats[i].real()*shorts_per_float;
        int16_t imag = host_floats[i].imag()*shorts_per_float;
        usrp2_items[i] = htonl(((real << 16) & 0xffff) | ((imag << 0) & 0xffff));
    });
}

static inline void usrp2_items_to_host_floats(
    fc32_t *host_floats,
    const uint32_t *usrp2_items,
    size_t num_samps
){
    unrolled_loop(i, num_samps,{
        uint32_t item = ntohl(usrp2_items[i]);
        int16_t real = (item >> 16) & 0xffff;
        int16_t imag = (item >> 0)  & 0xffff;
        host_floats[i] = fc32_t(real*floats_per_short, imag*floats_per_short);
    });
}

static inline void host_items_to_usrp2_items(
    uint32_t *usrp2_items,
    const uint32_t *host_items,
    size_t num_samps
){
    if (is_big_endian){
        std::memcpy(usrp2_items, host_items, num_samps*sizeof(uint32_t));
    }
    else{
        unrolled_loop(i, num_samps, usrp2_items[i] = htonl(host_items[i]));
    }
}

static inline void usrp2_items_to_host_items(
    uint32_t *host_items,
    const uint32_t *usrp2_items,
    size_t num_samps
){
    if (is_big_endian){
        std::memcpy(host_items, usrp2_items, num_samps*sizeof(uint32_t));
    }
    else{
        unrolled_loop(i, num_samps, host_items[i] = ntohl(usrp2_items[i]));
    }
}

/***********************************************************************
 * Receive Raw Data
 **********************************************************************/
void usrp2_impl::recv_raw(uhd::metadata_t &metadata){
    //do a receive
    _rx_smart_buff = _data_transport->recv();

    //unpack the vrt header
    const uint32_t *vrt_hdr = asio::buffer_cast<const uint32_t *>(_rx_smart_buff->get());
    size_t num_packet_words32 = asio::buffer_size(_rx_smart_buff->get())/sizeof(uint32_t);
    size_t num_header_words32_out;
    size_t num_payload_words32_out;
    size_t packet_count_out;
    try{
        vrt::unpack(
            metadata,                //output
            vrt_hdr,                 //input
            num_header_words32_out,  //output
            num_payload_words32_out, //output
            num_packet_words32,      //input
            packet_count_out         //output
        );
    }catch(const std::exception &e){
        std::cerr << "bad vrt header: " << e.what() << std::endl;
        _rx_copy_buff = boost::asio::buffer("", 0);
    }

    //handle the packet count / sequence number
    size_t last_packet_count = _rx_stream_id_to_packet_seq[metadata.stream_id];
    if (packet_count_out != (last_packet_count+1)%16){
        std::cerr << "bad packet count: " << packet_count_out << std::endl;
    }
    _rx_stream_id_to_packet_seq[metadata.stream_id] = packet_count_out;

    //setup the rx buffer to point to the data
    _rx_copy_buff = boost::asio::buffer(
        vrt_hdr + num_header_words32_out,
        num_payload_words32_out*sizeof(uint32_t)
    );
}

/***********************************************************************
 * Send Data
 **********************************************************************/
size_t usrp2_impl::send(
    const boost::asio::const_buffer &buff,
    const uhd::metadata_t &metadata,
    const std::string &type
){
    uint32_t tx_mem[_mtu/sizeof(uint32_t)];
    uint32_t *items = tx_mem + vrt::max_header_words32; //offset for data
    size_t num_samps = _max_tx_samples_per_packet;

    //calculate the number of samples to be copied
    //and copy the samples into the send buffer
    if (type == "32fc"){
        num_samps = std::min(asio::buffer_size(buff)/sizeof(fc32_t), num_samps);
        host_floats_to_usrp2_items(items, asio::buffer_cast<const fc32_t*>(buff), num_samps);
    }
    else if (type == "16sc"){
        num_samps = std::min(asio::buffer_size(buff)/sizeof(sc16_t), num_samps);
        host_items_to_usrp2_items(items, asio::buffer_cast<const uint32_t*>(buff), num_samps);
    }
    else{
        throw std::runtime_error(str(boost::format("usrp2 send: cannot handle type \"%s\"") % type));
    }

    uint32_t vrt_hdr[vrt::max_header_words32];
    size_t num_header_words32, num_packet_words32;
    size_t packet_count = _tx_stream_id_to_packet_seq[metadata.stream_id]++;

    //pack metadata into a vrt header
    vrt::pack(
        metadata,            //input
        vrt_hdr,             //output
        num_header_words32,  //output
        num_samps,           //input
        num_packet_words32,  //output
        packet_count         //input
    );

    //copy in the vrt header (yes we left space)
    items -= num_header_words32;
    std::memcpy(items, vrt_hdr, num_header_words32*sizeof(uint32_t));

    //send and return number of samples
    _data_transport->send(asio::buffer(items, num_packet_words32*sizeof(uint32_t)));
    return num_samps;
}

/***********************************************************************
 * Receive Data
 **********************************************************************/
size_t usrp2_impl::recv(
    const boost::asio::mutable_buffer &buff,
    uhd::metadata_t &metadata,
    const std::string &type
){
    //perform a receive if no rx data is waiting to be copied
    if (asio::buffer_size(_rx_copy_buff) == 0) recv_raw(metadata);
    //TODO otherwise flag the metadata to show that is is a fragment

    //extract the number of samples available to copy
    //and a pointer into the usrp2 received items memory
    size_t num_samps = asio::buffer_size(_rx_copy_buff)/sizeof(uint32_t);
    const uint32_t *items = asio::buffer_cast<const uint32_t*>(_rx_copy_buff);

    //calculate the number of samples to be copied
    //and copy the samples from the recv buffer
    if (type == "32fc"){
        num_samps = std::min(asio::buffer_size(buff)/sizeof(fc32_t), num_samps);
        usrp2_items_to_host_floats(asio::buffer_cast<fc32_t*>(buff), items, num_samps);
    }
    else if (type == "16sc"){
        num_samps = std::min(asio::buffer_size(buff)/sizeof(sc16_t), num_samps);
        usrp2_items_to_host_items(asio::buffer_cast<uint32_t*>(buff), items, num_samps);
    }
    else{
        throw std::runtime_error(str(boost::format("usrp2 recv: cannot handle type \"%s\"") % type));
    }

    //update the rx copy buffer to reflect the bytes copied
    _rx_copy_buff = asio::buffer(items + num_samps, num_samps*sizeof(uint32_t));

    return num_samps;
}
