//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_TRANSPORT_CHDR_HPP
#define INCLUDED_UHD_TRANSPORT_CHDR_HPP

#include <uhd/transport/vrt_if_packet.hpp>

namespace uhd{ namespace transport{ namespace vrt{

/*! \brief CHDR related function
 *
 * See \ref rtp_chdr for details on the CHDR protocol.
 *
 * All packers take the host format into account. Choose the _le functions
 * if the transport uses little endian format (e.g. PCIe) and the _be
 * functions if the transport uses big endian format (e.g. Ethernet).
 *
 * Note 1: All packers assume there to be enough space at the address
 * provided by \p packet_buff. See also \ref vrt_pack_contract.
 *
 * Note 2: All these packers assume the following options without checking them:
 * - `if_packet_info.link_type == LINK_TYPE_CHDR`
 * - `if_packet_info.has_cid == false`
 * - `if_packet_info.has_sid == true`
 * - `if_packet_info.has_tsi == false`
 * - `if_packet_info.has_tlr == false`
 * This relaxes some of \ref vrt_pack_contract, but adds the additional
 * constraint that the input data must be CHDR.
 *
 * In the unpacker, these values will be set accordingly.
 */
namespace chdr{

    //! The maximum number of 64-bit words in a CHDR header
    static const size_t max_if_hdr_words64 = 2; // CHDR + tsf (fractional timestamp)

    /*!
     * Pack a CHDR header from metadata (big endian format).
     *
     * See \ref vrt_pack_contract, but `link_type` is assumed to be
     * `LINK_TYPE_CHDR`.
     *
     * \param packet_buff memory to write the packed vrt header
     * \param if_packet_info the if packet info (read/write)
     */
    UHD_API void if_hdr_pack_be(
        uint32_t *packet_buff,
        if_packet_info_t &if_packet_info
    );

    /*!
     * Unpack a CHDR header to metadata (big endian format).
     *
     * See \ref vrt_unpack_contract, but `link_type` is assumed to be
     * `LINK_TYPE_CHDR`.
     *
     * \param packet_buff memory to read the packed vrt header
     * \param if_packet_info the if packet info (read/write)
     */
    UHD_API void if_hdr_unpack_be(
        const uint32_t *packet_buff,
        if_packet_info_t &if_packet_info
    );

    /*!
     * Pack a CHDR header from metadata (little endian format).
     *
     * See \ref vrt_pack_contract, but `link_type` is assumed to be
     * `LINK_TYPE_CHDR`.
     *
     * \param packet_buff memory to write the packed vrt header
     * \param if_packet_info the if packet info (read/write)
     */
    UHD_API void if_hdr_pack_le(
        uint32_t *packet_buff,
        if_packet_info_t &if_packet_info
    );

    /*!
     * Unpack a CHDR header to metadata (little endian format).
     *
     * See \ref vrt_unpack_contract, but `link_type` is assumed to be
     * `LINK_TYPE_CHDR`.
     *
     * \param packet_buff memory to read the packed vrt header
     * \param if_packet_info the if packet info (read/write)
     */
    UHD_API void if_hdr_unpack_le(
        const uint32_t *packet_buff,
        if_packet_info_t &if_packet_info
    );

} //namespace chdr

}}} //namespace uhd::transport::vrt

#endif /* INCLUDED_UHD_TRANSPORT_CHDR_HPP */

