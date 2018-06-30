//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/transport/chdr.hpp>
#include <uhd/utils/byteswap.hpp>
#include <uhd/exception.hpp>

//define the endian macros to convert integers
#ifdef BOOST_BIG_ENDIAN
    #define BE_MACRO(x) (x)
    #define LE_MACRO(x) uhd::byteswap(x)
#else
    #define BE_MACRO(x) uhd::byteswap(x)
    #define LE_MACRO(x) (x)
#endif

using namespace uhd::transport::vrt;

static const uint32_t HDR_FLAG_TSF = (1 << 29);
static const uint32_t HDR_FLAG_EOB = (1 << 28);
static const uint32_t HDR_FLAG_ERROR = (1 << 28);

/***************************************************************************/
/* Packing                                                                 */
/***************************************************************************/
/*! Translate the contents of \p if_packet_info into a 32-Bit word and return it.
 */
UHD_INLINE uint32_t _hdr_pack_chdr(
        if_packet_info_t &if_packet_info
) {
    // Set fields in if_packet_info
    if_packet_info.num_header_words32 = 2 + (if_packet_info.has_tsf ? 2 : 0);
    if_packet_info.num_packet_words32 =
            if_packet_info.num_header_words32 +
            if_packet_info.num_payload_words32;

    uint16_t pkt_length =
        if_packet_info.num_payload_bytes + (4 * if_packet_info.num_header_words32);
    uint32_t chdr = 0
        // 2 Bits: Packet type
        | (if_packet_info.packet_type << 30)
        // 1 Bit: Has time
        | (if_packet_info.has_tsf ? HDR_FLAG_TSF : 0)
        // 1 Bit: EOB or Error
        | ((if_packet_info.eob or if_packet_info.error) ? HDR_FLAG_EOB : 0)
        // 12 Bits: Sequence number
        | ((if_packet_info.packet_count & 0xFFF) << 16)
        // 16 Bits: Total packet length
        | pkt_length;
    return chdr;
}

void chdr::if_hdr_pack_be(
        uint32_t *packet_buff,
        if_packet_info_t &if_packet_info
) {
    // Write header and update if_packet_info
    packet_buff[0] = BE_MACRO(_hdr_pack_chdr(if_packet_info));

    // Write SID
    packet_buff[1] = BE_MACRO(if_packet_info.sid);

    // Write time
    if (if_packet_info.has_tsf) {
        packet_buff[2] = BE_MACRO(uint32_t(if_packet_info.tsf >> 32));
        packet_buff[3] = BE_MACRO(uint32_t(if_packet_info.tsf >> 0));
    }
}

void chdr::if_hdr_pack_le(
        uint32_t *packet_buff,
        if_packet_info_t &if_packet_info
) {
    // Write header and update if_packet_info
    packet_buff[0] = LE_MACRO(_hdr_pack_chdr(if_packet_info));

    // Write SID
    packet_buff[1] = LE_MACRO(if_packet_info.sid);

    // Write time
    if (if_packet_info.has_tsf) {
        packet_buff[2] = LE_MACRO(uint32_t(if_packet_info.tsf >> 32));
        packet_buff[3] = LE_MACRO(uint32_t(if_packet_info.tsf >> 0));
    }
}


/***************************************************************************/
/* Unpacking                                                               */
/***************************************************************************/
UHD_INLINE void _hdr_unpack_chdr(
        const uint32_t chdr,
        if_packet_info_t &if_packet_info
) {
    // Set constant members
    if_packet_info.link_type = if_packet_info_t::LINK_TYPE_CHDR;
    if_packet_info.has_cid = false;
    if_packet_info.has_sid = true;
    if_packet_info.has_tsi = false;
    if_packet_info.has_tlr = false;
    if_packet_info.sob = false;

    // Set configurable members
    if_packet_info.has_tsf = (chdr & HDR_FLAG_TSF) > 0;
    if_packet_info.packet_type = if_packet_info_t::packet_type_t((chdr >> 30) & 0x3);
    if_packet_info.eob = (if_packet_info.packet_type == if_packet_info_t::PACKET_TYPE_DATA)
                         && ((chdr & HDR_FLAG_EOB) > 0);
    if_packet_info.error = (if_packet_info.packet_type == if_packet_info_t::PACKET_TYPE_RESP)
                         && ((chdr & HDR_FLAG_ERROR) > 0);
    if_packet_info.packet_count = (chdr >> 16) & 0xFFF;

    // Set packet length variables
    if (if_packet_info.has_tsf) {
        if_packet_info.num_header_words32 = 4;
    } else {
        if_packet_info.num_header_words32 = 2;
    }
    size_t pkt_size_bytes = (chdr & 0xFFFF);
    size_t pkt_size_word32 = (pkt_size_bytes / 4) + ((pkt_size_bytes % 4) ? 1 : 0);
    // Check lengths match:
    if (pkt_size_word32 < if_packet_info.num_header_words32) {
        throw uhd::value_error("Bad CHDR or invalid packet length");
    }
    if (if_packet_info.num_packet_words32 < pkt_size_word32) {
        throw uhd::value_error("Bad CHDR or packet fragment");
    }
    if_packet_info.num_payload_bytes = pkt_size_bytes - (4 * if_packet_info.num_header_words32);
    if_packet_info.num_payload_words32 = pkt_size_word32 - if_packet_info.num_header_words32;
}

void chdr::if_hdr_unpack_be(
        const uint32_t *packet_buff,
        if_packet_info_t &if_packet_info
) {
    // Read header and update if_packet_info
    uint32_t chdr = BE_MACRO(packet_buff[0]);
    _hdr_unpack_chdr(chdr, if_packet_info);

    // Read SID
    if_packet_info.sid = BE_MACRO(packet_buff[1]);

    // Read time (has_tsf was updated earlier)
    if (if_packet_info.has_tsf) {
        if_packet_info.tsf = 0
            | uint64_t(BE_MACRO(packet_buff[2])) << 32
            | BE_MACRO(packet_buff[3]);
    }
}

void chdr::if_hdr_unpack_le(
        const uint32_t *packet_buff,
        if_packet_info_t &if_packet_info
) {
    // Read header and update if_packet_info
    uint32_t chdr = LE_MACRO(packet_buff[0]);
    _hdr_unpack_chdr(chdr, if_packet_info);

    // Read SID
    if_packet_info.sid = LE_MACRO(packet_buff[1]);

    // Read time (has_tsf was updated earlier)
    if (if_packet_info.has_tsf) {
        if_packet_info.tsf = 0
            | uint64_t(LE_MACRO(packet_buff[2])) << 32
            | LE_MACRO(packet_buff[3]);
    }
}

