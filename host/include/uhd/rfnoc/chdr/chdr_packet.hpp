//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_RFNOC_CHDR_DATA_PACKET_HPP
#define INCLUDED_RFNOC_CHDR_DATA_PACKET_HPP

#include <uhd/rfnoc/chdr/chdr_header.hpp>
#include <cassert>

namespace uhd { namespace rfnoc { namespace chdr {

/*! CHDR Packet Abstraction Class
 *
 * This class wraps a pointer to a buffer containing a CHDR packet. It also
 * provides access to the packet header in native byte ordering. The byte
 * ordering of the metadata and payload needs to be handled elsewhere, but it
 * needs to be also passed into this class as a template argument \p endianness.
 * The byte ordering refers to the byte order on the transport.
 *
 * No size checking is performed in this class. The size of the buffer needs to
 * be tracked separately in order to avoid buffer overflows.
 *
 * NOTE: This assumes the host is little-endian.
 */
template <size_t chdr_w, endianness_t endianness>
class chdr_packet
{
public:
    typedef chdr_header<chdr_w> header_t;

    ~chdr_packet() = default;

    /*! This constructor is used to parse an incoming CHDR packet.
     *
     * Note: If the header declares the size of the packet to be larger than the
     * valid memory readable at \p pkt_buff, then there is the possibility of a
     * buffer overflow ocurring.
     *
     * \param pkt_buff a pointer to the packet buffer. Its byte ordering has to be
     *                 provided as the template argument \p endianness
     */
    chdr_packet(void* pkt_buff)
    {
        assert(pkt_buff != NULL);
        _pkt_buff = static_cast<uint64_t*>(pkt_buff);
        if (endianness == endianness_t::ENDIANNESS_BIG) {
            _header = header_t(ntohx<uint64_t>(*_pkt_buff));
        } else {
            _header = header_t(*_pkt_buff);
        }
    }

    /*! This constructor is used to construct an outgoing CHDR packet with given header.
     *
     * \param header CHDR header (in host-native byte ordering)
     * \param pkt_buff a pointer to packet buffer. Its byte ordering has to be provided
     *                 as the template argument \p endianness
     */
    chdr_packet(header_t header, void* pkt_buff) : _header(header)
    {
        assert(pkt_buff != NULL);
        _pkt_buff = static_cast<uint64_t*>(pkt_buff);
        if (endianness == endianness_t::ENDIANNESS_BIG) {
            *_pkt_buff = htonx<uint64_t>(header.get());
        } else {
            *_pkt_buff = header.get();
        }
    }

    /*! Get a copy of the header data (the first 64-bit of the CHDR packet)
     *
     * This copy will be in native byte order.
    */
    inline header_t get_header() const
    {
        return _header;
    }

    /*! Return the pointer to the first byte of metadata of this CHDR packet
     *
     * Note: This class does no bounds checking. It is up to the user to only
     * to the metadata portion of the the CHDR buffer.
     *
     * ~~~{.cpp}
     * // Assume chdrpkt is of type chdr_packet:
     * uint8_t* metadata_ptr = chdrpkt.metadata_ptr_of_type<uint8_t>();
     * const size_t metadata_size = chdrpkt.get_header().get_num_bytes_metadata();
     * // The following expression is fine if metadata_size > 0:
     * metadata_ptr[0] = 0xF0;
     * // The following expression is never fine, but the code will not throw an
     * // exception (most likely, it won't segfault either, unless the payload
     * // size is zero):
     * metadata_ptr[metadata_size] = 0xF0;
     * ~~~
    */
    template <typename data_t> inline data_t* metadata_ptr_of_type() const
    {
        return reinterpret_cast<data_t*>(
                reinterpret_cast<char*>(_pkt_buff) + _header.get_metadata_offset());
    }

    /*! Get 64-bit timestamp of this CHDR packet in native byte order
     *
     * This will byteswap if necessary.
     *
     */
    inline uint64_t get_timestamp() const
    {
        assert(_header.get_pkt_type() == packet_type_t::PACKET_TYPE_DATA_WITH_TS);
        return endianness_t::ENDIANNESS_BIG ? ntohx<uint64_t>(*(_pkt_buff + 1))
            : *(_pkt_buff + 1);
    }

    /*! Set 64-bit timestamp of this CHDR packet.
     *
     * If this packet is not a data packet with timestamp, this operation will
     * overwrite the metadata or payload memory.
     *
     * \param timestamp 64-bit value of timestamp of this CHDR packet
     */
    inline void set_timestamp(uint64_t timestamp)
    {
        assert(_header.get_pkt_type() == packet_type_t::PACKET_TYPE_DATA_WITH_TS);
        *(_pkt_buff + 1) = endianness_t::ENDIANNESS_BIG ? htonx<uint64_t>(timestamp)
            : timestamp;
    }

    /*! Return the pointer to the first payload byte of this CHDR packet
     *
     * Note: This class does no bounds checking. It is up to the user to only
     * to the metadata portion of the the CHDR buffer.
     *
     * ~~~{.cpp}
     * // Assume chdrpkt is of type chdr_packet:
     * uint8_t* payload_ptr = chdrpkt.metadata_ptr_of_type<uint8_t>();
     * const size_t payload_size = chdrpkt.get_header().get_num_bytes_payload();
     * // The following expression is fine if payload_size > 0:
     * payload_ptr[0] = 0xF0;
     * // The following expression is never fine, but the code will not throw an
     * // exception. Instead, it could even segfault:
     * payload_ptr[payload_size] = 0xF0;
     * ~~~
    */
    template <typename payload_t> inline payload_t* payload_ptr_of_type() const
    {
        return reinterpret_cast<payload_t*>(
                reinterpret_cast<char*>(_pkt_buff) + _header.get_payload_offset());
    }

private:
    uint64_t* _pkt_buff;
    header_t _header = header_t(0);
};

}}} // namespace uhd::rfnoc::chdr

#endif /* INCLUDED_RFNOC_CHDR_DATA_PACKET_HPP */
