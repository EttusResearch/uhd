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

#include <uhd/transport/vrt.hpp>
#include <netinet/in.h>
#include <stdexcept>

using namespace uhd::transport;

void vrt::pack(
    const metadata_t &metadata, //input
    uint32_t *header_buff,      //output
    size_t &num_header_words32, //output
    size_t num_payload_words32, //input
    size_t &num_packet_words32, //output
    size_t packet_count         //input
){
    uint32_t vrt_hdr_flags = 0;
    num_header_words32 = 1;

    //load the vrt header and flags
    if(metadata.has_stream_id){
        vrt_hdr_flags |= (0x1 << 28); //IF Data packet with Stream Identifier
        header_buff[num_header_words32++] = htonl(metadata.stream_id);
    }

    if(metadata.has_time_spec){
        vrt_hdr_flags |= (0x3 << 22) | (0x1 << 20); //TSI: Other, TSF: Sample Count Timestamp
        header_buff[num_header_words32++] = htonl(metadata.time_spec.secs);
        header_buff[num_header_words32++] = htonl(metadata.time_spec.ticks);
        header_buff[num_header_words32++] = 0; //unused part of fractional seconds
    }

    vrt_hdr_flags |= (metadata.start_of_burst)? (0x1 << 25) : 0;
    vrt_hdr_flags |= (metadata.end_of_burst)?   (0x1 << 24) : 0;

    num_packet_words32 = num_header_words32 + num_payload_words32;

    //fill in complete header word
    header_buff[0] = htonl(vrt_hdr_flags |
        ((packet_count & 0xf) << 16) |
        (num_packet_words32 & 0xffff)
    );
}

void vrt::unpack(
    metadata_t &metadata,            //output
    const uint32_t *header_buff,     //input
    size_t &num_header_words32,      //output
    size_t &num_payload_words32,     //output
    size_t num_packet_words32,       //input
    size_t &packet_count             //output
){
    //clear the metadata
    metadata = metadata_t();

    //extract vrt header
    uint32_t vrt_hdr_word = ntohl(header_buff[0]);
    size_t packet_words32 = vrt_hdr_word & 0xffff;
    packet_count = (vrt_hdr_word >> 16) & 0xf;

    //failure cases
    if (packet_words32 == 0 or num_packet_words32 < packet_words32)
        throw std::runtime_error("bad vrt header or packet fragment");
    if (vrt_hdr_word & (0x7 << 29))
        throw std::runtime_error("unsupported vrt packet type");

    //parse the header flags
    num_header_words32 = 1;

    if (vrt_hdr_word & (0x1 << 28)){ //stream id
        metadata.has_stream_id = true;
        metadata.stream_id = ntohl(header_buff[num_header_words32++]);
    }

    if (vrt_hdr_word & (0x1 << 27)){ //class id (we dont use)
        num_header_words32 += 2;
    }

    if (vrt_hdr_word & (0x3 << 22)){ //integer time
        metadata.has_time_spec = true;
        metadata.time_spec.secs = ntohl(header_buff[num_header_words32++]);
    }

    if (vrt_hdr_word & (0x3 << 20)){ //fractional time
        metadata.has_time_spec = true;
        metadata.time_spec.ticks = ntohl(header_buff[num_header_words32++]);
        num_header_words32++; //unused part of fractional seconds
    }

    size_t num_trailer_words32 = (vrt_hdr_word & (0x1 << 26))? 1 : 0;

    num_payload_words32 = packet_words32 - num_header_words32 - num_trailer_words32;
}
