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
 * Send Raw Data
 **********************************************************************/
size_t usrp2_impl::send_raw(const uhd::metadata_t &metadata){
    size_t num_items = asio::buffer_size(_tx_copy_buff)/sizeof(uint32_t);
    const uint32_t *items = asio::buffer_cast<const uint32_t *>(_tx_copy_buff);

    uint32_t vrt_hdr[_tx_vrt_max_offset_words32];
    uint32_t vrt_hdr_flags = 0;
    size_t num_vrt_hdr_words = 1;

    //load the vrt header and flags
    if(metadata.has_stream_id){
        vrt_hdr_flags |= (0x1 << 28); //IF Data packet with Stream Identifier
        vrt_hdr[num_vrt_hdr_words++] = htonl(metadata.stream_id);
    }
    if(metadata.has_time_spec){
        vrt_hdr_flags |= (0x3 << 22) | (0x1 << 20); //TSI: Other, TSF: Sample Count Timestamp
        vrt_hdr[num_vrt_hdr_words++] = htonl(metadata.time_spec.secs);
        vrt_hdr[num_vrt_hdr_words++] = htonl(metadata.time_spec.ticks);
        vrt_hdr[num_vrt_hdr_words++] = 0; //unused part of fractional seconds
    }
    vrt_hdr_flags |= (metadata.start_of_burst)? (0x1 << 25) : 0;
    vrt_hdr_flags |= (metadata.end_of_burst)?   (0x1 << 24) : 0;

    //fill in complete header word
    vrt_hdr[0] = htonl(vrt_hdr_flags |
        ((_tx_stream_id_to_packet_seq[metadata.stream_id]++ & 0xf) << 16) |
        ((num_vrt_hdr_words + num_items) & 0xffff)
    );

    //copy in the vrt header (yes we left space)
    std::memcpy(((uint32_t *)items) - num_vrt_hdr_words, vrt_hdr, num_vrt_hdr_words);
    asio::const_buffer buff(items - num_vrt_hdr_words, (num_vrt_hdr_words + num_items)*sizeof(uint32_t));

    //send and return number of samples
    return (_data_transport->send(buff) - num_vrt_hdr_words*sizeof(uint32_t))/sizeof(sc16_t);
}

/***********************************************************************
 * Receive Raw Data
 **********************************************************************/
void usrp2_impl::recv_raw(uhd::metadata_t &metadata){
    //do a receive
    _rx_smart_buff = _data_transport->recv();

    ////////////////////////////////////////////////////////////////////
    // !!!! FIXME this is very flawed, use a proper vrt unpacker !!!!!!!
    ////////////////////////////////////////////////////////////////////

    //unpack the vrt header
    const uint32_t *vrt_hdr = asio::buffer_cast<const uint32_t *>(_rx_smart_buff->get());
    metadata = uhd::metadata_t();
    uint32_t vrt_header = ntohl(vrt_hdr[0]);
    metadata.has_stream_id = true;
    metadata.stream_id = ntohl(vrt_hdr[1]);
    metadata.has_time_spec = true;
    metadata.time_spec.secs = ntohl(vrt_hdr[2]);
    metadata.time_spec.ticks = ntohl(vrt_hdr[3]);

    size_t my_seq = (vrt_header >> 16) & 0xf;
    //std::cout << "seq " << my_seq << std::endl;
    if (my_seq != ((_rx_stream_id_to_packet_seq[metadata.stream_id]+1) & 0xf)) std::cout << "bad seq " << my_seq << std::endl;
    _rx_stream_id_to_packet_seq[metadata.stream_id] = my_seq;

    //extract the number of bytes received
    size_t num_words = (vrt_header & 0xffff) -
        USRP2_HOST_RX_VRT_HEADER_WORDS32 -
        USRP2_HOST_RX_VRT_TRAILER_WORDS32;

    //setup the rx buffer to point to the data
    _rx_copy_buff = boost::asio::buffer(
        vrt_hdr + USRP2_HOST_RX_VRT_HEADER_WORDS32,
        num_words*sizeof(uint32_t)
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
    uint32_t *items = _tx_mem + _tx_vrt_max_offset_words32; //offset for data
    size_t num_samps = _max_samples_per_packet;

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

    //send the samples (this line seems silly, will be better with vrt lib)
    _tx_copy_buff = asio::buffer(items, num_samps*sizeof(uint32_t));
    return send_raw(metadata); //return num_samps;
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
