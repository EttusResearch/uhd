//
// Copyright 2010-2013 Ettus Research LLC
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

#ifndef INCLUDED_UHD_TRANSPORT_VRT_IF_PACKET_HPP
#define INCLUDED_UHD_TRANSPORT_VRT_IF_PACKET_HPP

#include <uhd/config.hpp>
#include <boost/cstdint.hpp>
#include <cstddef> //size_t

namespace uhd{ namespace transport{

namespace vrt{

    //! The maximum number of 32-bit words in the vrlp link layer
    static const size_t num_vrl_words32 = 3;

    //! The maximum number of 32-bit words in a vrt if packet header
    static const size_t max_if_hdr_words32 = 7; //hdr+sid+cid+tsi+tsf

    /*!
     * Definition for fields that can be packed into a vrt if header.
     * The size fields are used for input and output depending upon
     * the operation used (ie the pack or unpack function call).
     */
    struct UHD_API if_packet_info_t
    {
        if_packet_info_t(void);

        //link layer type - always set for pack and unpack
        enum link_type_t
        {
            LINK_TYPE_NONE = 0x0,
            LINK_TYPE_CHDR = 0x1,
            LINK_TYPE_VRLP = 0x2,
        } link_type;

        //packet type
        enum packet_type_t
        {
            PACKET_TYPE_DATA      = 0x0,
            PACKET_TYPE_IF_EXT    = 0x1,
            PACKET_TYPE_CONTEXT   = 0x2, //extension context: has_sid = true
        } packet_type;

        //size fields
        size_t num_payload_words32; //required in pack, derived in unpack
        size_t num_payload_bytes;   //required in pack, derived in unpack
        size_t num_header_words32;  //derived in pack, derived in unpack
        size_t num_packet_words32;  //derived in pack, required in unpack

        //header fields
        size_t packet_count;
        bool sob, eob;

        //optional fields
        bool has_sid; boost::uint32_t sid;
        bool has_cid; boost::uint64_t cid;
        bool has_tsi; boost::uint32_t tsi;
        bool has_tsf; boost::uint64_t tsf;
        bool has_tlr; boost::uint32_t tlr;
    };

    /*!
     * Pack a vrt header from metadata (big endian format).
     * \param packet_buff memory to write the packed vrt header
     * \param if_packet_info the if packet info (read/write)
     */
    UHD_API void if_hdr_pack_be(
        boost::uint32_t *packet_buff,
        if_packet_info_t &if_packet_info
    );

    /*!
     * Unpack a vrt header to metadata (big endian format).
     * \param packet_buff memory to read the packed vrt header
     * \param if_packet_info the if packet info (read/write)
     */
    UHD_API void if_hdr_unpack_be(
        const boost::uint32_t *packet_buff,
        if_packet_info_t &if_packet_info
    );

    /*!
     * Pack a vrt header from metadata (little endian format).
     * \param packet_buff memory to write the packed vrt header
     * \param if_packet_info the if packet info (read/write)
     */
    UHD_API void if_hdr_pack_le(
        boost::uint32_t *packet_buff,
        if_packet_info_t &if_packet_info
    );

    /*!
     * Unpack a vrt header to metadata (little endian format).
     * \param packet_buff memory to read the packed vrt header
     * \param if_packet_info the if packet info (read/write)
     */
    UHD_API void if_hdr_unpack_le(
        const boost::uint32_t *packet_buff,
        if_packet_info_t &if_packet_info
    );

    UHD_INLINE if_packet_info_t::if_packet_info_t(void):
        link_type(LINK_TYPE_NONE),
        packet_type(PACKET_TYPE_DATA),
        num_payload_words32(0),
        num_payload_bytes(0),
        num_header_words32(0),
        num_packet_words32(0),
        packet_count(0),
        sob(false), eob(false),
        has_sid(false), sid(0),
        has_cid(false), cid(0),
        has_tsi(false), tsi(0),
        has_tsf(false), tsf(0),
        has_tlr(false), tlr(0)
    {}

} //namespace vrt

}} //namespace

#endif /* INCLUDED_UHD_TRANSPORT_VRT_IF_PACKET_HPP */
