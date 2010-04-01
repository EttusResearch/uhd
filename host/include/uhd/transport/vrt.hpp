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

#ifndef INCLUDED_UHD_TRANSPORT_VRT_HPP
#define INCLUDED_UHD_TRANSPORT_VRT_HPP

#include <uhd/config.hpp>
#include <uhd/types/metadata.hpp>
#include <cstddef>

namespace uhd{ namespace transport{

namespace vrt{

    static const size_t max_header_words32 = 7;

    /*!
     * Pack a vrt header from metadata.
     * \param metadata the tx metadata with flags and timestamps
     * \param header_buff memory to write the packed vrt header
     * \param num_header_words32 number of words in the vrt header
     * \param num_payload_words32 the length of the payload
     * \param num_packet_words32 the length of the packet
     * \param packet_count the packet count sequence number
     */
    UHD_API void pack(
        const tx_metadata_t &metadata, //input
        boost::uint32_t *header_buff,  //output
        size_t &num_header_words32,    //output
        size_t num_payload_words32,    //input
        size_t &num_packet_words32,    //output
        size_t packet_count            //input
    );

    /*!
     * Unpack a vrt header to metadata.
     * \param metadata the rx metadata with flags and timestamps
     * \param header_buff memory to read the packed vrt header
     * \param num_header_words32 number of words in the vrt header
     * \param num_payload_words32 the length of the payload
     * \param num_packet_words32 the length of the packet
     * \param packet_count the packet count sequence number
     */
    UHD_API void unpack(
        rx_metadata_t &metadata,            //output
        const boost::uint32_t *header_buff, //input
        size_t &num_header_words32,         //output
        size_t &num_payload_words32,        //output
        size_t num_packet_words32,          //input
        size_t &packet_count                //output
    );

} //namespace vrt

}} //namespace

#endif /* INCLUDED_UHD_TRANSPORT_VRT_HPP */
