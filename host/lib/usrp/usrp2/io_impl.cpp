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
#include <boost/shared_array.hpp>
#include <boost/format.hpp>
#include "usrp2_impl.hpp"

using namespace uhd;
using namespace uhd::usrp;
namespace asio = boost::asio;

/***********************************************************************
 * Constants
 **********************************************************************/
typedef std::complex<float> fc32_t;
typedef std::complex<short> sc16_t;

static const float float_scale_factor = pow(2.0, 15);

//max length with header, stream id, seconds, fractional seconds
static const size_t max_vrt_header_words = 5;

/***********************************************************************
 * Helper Functions
 **********************************************************************/
static inline void host_floats_to_usrp2_shorts(
    short *usrp2_shorts,
    const float *host_floats,
    size_t num_samps
){
    for(size_t i = 0; i < num_samps; i++){
        usrp2_shorts[i] = htons(short(host_floats[i]*float_scale_factor));
    }
}

static inline void usrp2_shorts_to_host_floats(
    float *host_floats,
    const short *usrp2_shorts,
    size_t num_samps
){
    for(size_t i = 0; i < num_samps; i++){
        host_floats[i] = float(short(ntohs(usrp2_shorts[i])))/float_scale_factor;
    }
}

static inline void host_shorts_to_usrp2_shorts(
    short *usrp2_shorts,
    const short *host_shorts,
    size_t num_samps
){
    for(size_t i = 0; i < num_samps; i++){
        usrp2_shorts[i] = htons(host_shorts[i]);
    }
}

static inline void usrp2_shorts_to_host_shorts(
    short *host_shorts,
    const short *usrp2_shorts,
    size_t num_samps
){
    for(size_t i = 0; i < num_samps; i++){
        host_shorts[i] = ntohs(usrp2_shorts[i]);
    }
}

/***********************************************************************
 * Send Raw Data
 **********************************************************************/
size_t usrp2_impl::send_raw(
    const boost::asio::const_buffer &buff,
    const uhd::metadata_t &metadata
){
    std::vector<boost::asio::const_buffer> buffs(2);
    uint32_t vrt_hdr[max_vrt_header_words];
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
    num_vrt_hdr_words += asio::buffer_size(buff)/sizeof(uint32_t);

    //fill in complete header word
    vrt_hdr[0] = htonl(vrt_hdr_flags |
        ((_stream_id_to_packet_seq[metadata.stream_id]++ & 0xf) << 16) |
        (num_vrt_hdr_words & 0xffff)
    );

    //load the buffer vector
    size_t vrt_hdr_size = num_vrt_hdr_words*sizeof(uint32_t);
    buffs[0] = asio::buffer(&vrt_hdr, vrt_hdr_size);
    buffs[1] = buff;

    //send and return number of samples
    return (_data_transport->send(buffs) - vrt_hdr_size)/sizeof(sc16_t);
}

/***********************************************************************
 * Receive Raw Data
 **********************************************************************/
size_t usrp2_impl::recv_raw(
    const boost::asio::mutable_buffer &buff,
    uhd::metadata_t &metadata
){
    //handle the case where there is spillover
    if (asio::buffer_size(_splillover_buff) != 0){
        size_t bytes_to_copy = std::min(
            asio::buffer_size(_splillover_buff),
            asio::buffer_size(buff)
        );
        std::memcpy(
            asio::buffer_cast<void*>(buff),
            asio::buffer_cast<const void*>(_splillover_buff),
            bytes_to_copy
        );
        _splillover_buff = asio::buffer(
            asio::buffer_cast<uint8_t*>(_splillover_buff)+bytes_to_copy,
            asio::buffer_size(_splillover_buff)-bytes_to_copy
        );
        //std::cout << boost::format("Copied spillover %d samples") % (bytes_to_copy/sizeof(sc16_t)) << std::endl;
        return bytes_to_copy/sizeof(sc16_t);
    }

    //load the buffer vector
    std::vector<boost::asio::mutable_buffer> buffs(3);
    uint32_t vrt_hdr[max_vrt_header_words];
    buffs[0] = asio::buffer(vrt_hdr, max_vrt_header_words*sizeof(uint32_t));
    buffs[1] = asio::buffer(//make sure its on a word boundary
        buff, asio::buffer_size(buff) & ~(sizeof(uint32_t) - 1)
    );
    buffs[2] = asio::buffer(_spillover_mem, _mtu);

    //receive into the buffers
    size_t bytes_recvd = _data_transport->recv(buffs);

    //failure case
    if (bytes_recvd < max_vrt_header_words*sizeof(uint32_t)) return 0;

    //unpack the vrt header
    metadata = uhd::metadata_t();
    uint32_t vrt_header = ntohl(vrt_hdr[0]);
    metadata.has_stream_id = true;
    metadata.stream_id = ntohl(vrt_hdr[1]);
    metadata.has_time_spec = true;
    metadata.time_spec.secs = ntohl(vrt_hdr[2]);
    metadata.time_spec.ticks = ntohl(vrt_hdr[3]);

    //extract the number of bytes received
    size_t num_words = (vrt_header & 0xffff) - max_vrt_header_words;
    size_t num_bytes = num_words*sizeof(uint32_t);

    //handle the case where spillover memory was used
    size_t spillover_size = num_bytes - std::min(num_bytes, asio::buffer_size(buff));
    _splillover_buff = asio::buffer(_spillover_mem, spillover_size);

    return (num_bytes - spillover_size)/sizeof(sc16_t);
}

/***********************************************************************
 * Send Data
 **********************************************************************/
size_t usrp2_impl::send(
    const boost::asio::const_buffer &buff,
    const uhd::metadata_t &metadata,
    const std::string &type
){
    if (type == "32fc"){
        size_t num_samps = asio::buffer_size(buff)/sizeof(fc32_t);
        boost::shared_array<sc16_t> raw_mem(new sc16_t[num_samps]);
        boost::asio::mutable_buffer raw_buff(raw_mem.get(), num_samps*sizeof(sc16_t));

        host_floats_to_usrp2_shorts(
            asio::buffer_cast<short*>(raw_buff),
            asio::buffer_cast<const float*>(buff),
            num_samps*2 //double for complex
        );

        return send_raw(raw_buff, metadata);
    }

    if (type == "16sc"){
        #ifdef HAVE_BIG_ENDIAN
        return send_raw(buff, metadata);
        #else
        size_t num_samps = asio::buffer_size(buff)/sizeof(sc16_t);
        boost::shared_array<sc16_t> raw_mem(new sc16_t[num_samps]);
        boost::asio::mutable_buffer raw_buff(raw_mem.get(), num_samps*sizeof(sc16_t));

        host_shorts_to_usrp2_shorts(
            asio::buffer_cast<short*>(raw_buff),
            asio::buffer_cast<const short*>(buff),
            num_samps*2 //double for complex
        );

        return send_raw(raw_buff, metadata);
        #endif
    }

    throw std::runtime_error(str(boost::format("usrp2 send: cannot handle type \"%s\"") % type));
}

/***********************************************************************
 * Receive Data
 **********************************************************************/
size_t usrp2_impl::recv(
    const boost::asio::mutable_buffer &buff,
    uhd::metadata_t &metadata,
    const std::string &type
){
    if (type == "32fc"){
        size_t num_samps = asio::buffer_size(buff)/sizeof(fc32_t);
        boost::shared_array<sc16_t> raw_mem(new sc16_t[num_samps]);
        boost::asio::mutable_buffer raw_buff(raw_mem.get(), num_samps*sizeof(sc16_t));

        num_samps = recv_raw(raw_buff, metadata);

        usrp2_shorts_to_host_floats(
            asio::buffer_cast<float*>(buff),
            asio::buffer_cast<const short*>(raw_buff),
            num_samps*2 //double for complex
        );

        return num_samps;
    }

    if (type == "16sc"){
        #ifdef HAVE_BIG_ENDIAN
        return recv_raw(buff, metadata);
        #else
        size_t num_samps = asio::buffer_size(buff)/sizeof(sc16_t);
        boost::shared_array<sc16_t> raw_mem(new sc16_t[num_samps]);
        boost::asio::mutable_buffer raw_buff(raw_mem.get(), num_samps*sizeof(sc16_t));

        num_samps = recv_raw(raw_buff, metadata);

        usrp2_shorts_to_host_shorts(
            asio::buffer_cast<short*>(buff),
            asio::buffer_cast<const short*>(raw_buff),
            num_samps*2 //double for complex
        );

        return num_samps;
        #endif
    }

    throw std::runtime_error(str(boost::format("usrp2 recv: cannot handle type \"%s\"") % type));
}
