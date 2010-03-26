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
typedef std::complex<float>          fc32_t;
typedef std::complex<boost::int16_t> sc16_t;

static const float shorts_per_float = float(1 << 15);
static const float floats_per_short = float(1.0/shorts_per_float);

/***********************************************************************
 * Helper Functions
 **********************************************************************/
void usrp2_impl::io_init(void){
    //initially empty copy buffer
    _rx_copy_buff = asio::buffer("", 0);

    //send a small data packet so the usrp2 knows the udp source port
    //and the maximum number of lines (32 bit words) per packet
    boost::uint32_t data[2] = {
        htonl(USRP2_INVALID_VRT_HEADER),
        htonl(_max_rx_samples_per_packet)
    };
    _data_transport->send(asio::buffer(&data, sizeof(data)));
}

#define unrolled_loop(__inst, __len){ \
    size_t __i = 0; \
    for(; __i < (__len & ~0x3); __i+= 4){ \
        __inst(__i+0); __inst(__i+1); \
        __inst(__i+2); __inst(__i+3); \
    } \
    for(; __i < __len; __i++){ \
        __inst(__i); \
    } \
}

// set a boolean flag that indicates the endianess
#ifdef HAVE_BIG_ENDIAN
static const bool is_big_endian = true;
#else
static const bool is_big_endian = false;
#endif

static inline void host_floats_to_usrp2_items(
    boost::uint32_t *usrp2_items,
    const fc32_t *host_floats,
    size_t num_samps
){
    #define host_floats_to_usrp2_items_i(i){ \
        boost::uint16_t real = boost::int16_t(host_floats[i].real()*shorts_per_float); \
        boost::uint16_t imag = boost::int16_t(host_floats[i].imag()*shorts_per_float); \
        usrp2_items[i] = htonl((real << 16) | (imag << 0)); \
    }
    unrolled_loop(host_floats_to_usrp2_items_i, num_samps);
}

static inline void usrp2_items_to_host_floats(
    fc32_t *host_floats,
    const boost::uint32_t *usrp2_items,
    size_t num_samps
){
    #define usrp2_items_to_host_floats_i(i){ \
        boost::uint32_t item = ntohl(usrp2_items[i]); \
        boost::int16_t real = boost::uint16_t(item >> 16); \
        boost::int16_t imag = boost::uint16_t(item >> 0); \
        host_floats[i] = fc32_t(float(real*floats_per_short), float(imag*floats_per_short)); \
    }
    unrolled_loop(usrp2_items_to_host_floats_i, num_samps);
}

static inline void host_items_to_usrp2_items(
    boost::uint32_t *usrp2_items,
    const boost::uint32_t *host_items,
    size_t num_samps
){
    #define host_items_to_usrp2_items_i(i) usrp2_items[i] = htonl(host_items[i])
    if (is_big_endian){
        std::memcpy(usrp2_items, host_items, num_samps*sizeof(boost::uint32_t));
    }
    else{
        unrolled_loop(host_items_to_usrp2_items_i, num_samps);
    }
}

static inline void usrp2_items_to_host_items(
    boost::uint32_t *host_items,
    const boost::uint32_t *usrp2_items,
    size_t num_samps
){
    #define usrp2_items_to_host_items_i(i) host_items[i] = ntohl(usrp2_items[i])
    if (is_big_endian){
        std::memcpy(host_items, usrp2_items, num_samps*sizeof(boost::uint32_t));
    }
    else{
        unrolled_loop(usrp2_items_to_host_items_i, num_samps);
    }
}

/***********************************************************************
 * Receive Raw Data
 **********************************************************************/
void usrp2_impl::recv_raw(rx_metadata_t &metadata){
    //do a receive
    _rx_smart_buff = _data_transport->recv();

    //unpack the vrt header
    size_t num_packet_words32 = asio::buffer_size(_rx_smart_buff->get())/sizeof(boost::uint32_t);
    if (num_packet_words32 == 0){
        _rx_copy_buff = boost::asio::buffer("", 0);
        return; //must exit here after setting the buffer
    }
    const boost::uint32_t *vrt_hdr = asio::buffer_cast<const boost::uint32_t *>(_rx_smart_buff->get());
    size_t num_header_words32_out, num_payload_words32_out, packet_count_out;
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
    const tx_metadata_t &metadata,
    const std::string &type
){
    boost::uint32_t tx_mem[_mtu/sizeof(boost::uint32_t)];
    boost::uint32_t *items = tx_mem + vrt::max_header_words32; //offset for data
    size_t num_samps = _max_tx_samples_per_packet;

    //calculate the number of samples to be copied
    //and copy the samples into the send buffer
    if (type == "32fc"){
        num_samps = std::min(asio::buffer_size(buff)/sizeof(fc32_t), num_samps);
        host_floats_to_usrp2_items(items, asio::buffer_cast<const fc32_t*>(buff), num_samps);
    }
    else if (type == "16sc"){
        num_samps = std::min(asio::buffer_size(buff)/sizeof(sc16_t), num_samps);
        host_items_to_usrp2_items(items, asio::buffer_cast<const boost::uint32_t*>(buff), num_samps);
    }
    else{
        throw std::runtime_error(str(boost::format("usrp2 send: cannot handle type \"%s\"") % type));
    }

    boost::uint32_t vrt_hdr[vrt::max_header_words32];
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
    std::memcpy(items, vrt_hdr, num_header_words32*sizeof(boost::uint32_t));

    //send and return number of samples
    _data_transport->send(asio::buffer(items, num_packet_words32*sizeof(boost::uint32_t)));
    return num_samps;
}

/***********************************************************************
 * Receive Data
 **********************************************************************/
size_t usrp2_impl::recv(
    const asio::mutable_buffer &buff,
    rx_metadata_t &metadata,
    const std::string &type
){
    //perform a receive if no rx data is waiting to be copied
    if (asio::buffer_size(_rx_copy_buff) == 0){
        recv_raw(metadata);
    }
    //otherwise flag the metadata to show that is is a fragment
    else{
        metadata = rx_metadata_t();
        metadata.is_fragment = true;
    }

    //extract the number of samples available to copy
    //and a pointer into the usrp2 received items memory
    size_t bytes_to_copy = asio::buffer_size(_rx_copy_buff);
    if (bytes_to_copy == 0) return 0; //nothing to receive
    size_t num_samps = bytes_to_copy/sizeof(boost::uint32_t);
    const boost::uint32_t *items = asio::buffer_cast<const boost::uint32_t*>(_rx_copy_buff);

    //calculate the number of samples to be copied
    //and copy the samples from the recv buffer
    if (type == "32fc"){
        num_samps = std::min(asio::buffer_size(buff)/sizeof(fc32_t), num_samps);
        usrp2_items_to_host_floats(asio::buffer_cast<fc32_t*>(buff), items, num_samps);
    }
    else if (type == "16sc"){
        num_samps = std::min(asio::buffer_size(buff)/sizeof(sc16_t), num_samps);
        usrp2_items_to_host_items(asio::buffer_cast<boost::uint32_t*>(buff), items, num_samps);
    }
    else{
        throw std::runtime_error(str(boost::format("usrp2 recv: cannot handle type \"%s\"") % type));
    }

    //update the rx copy buffer to reflect the bytes copied
    _rx_copy_buff = asio::buffer(
        items + num_samps, bytes_to_copy - num_samps*sizeof(boost::uint32_t)
    );

    return num_samps;
}
