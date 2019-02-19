//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_RFNOC_CHDR_HEADER_HPP
#define INCLUDED_RFNOC_CHDR_HEADER_HPP

#include <uhd/types/endianness.hpp>
#include <uhd/utils/byteswap.hpp>
#include <cassert>

namespace uhd { namespace rfnoc { namespace chdr {

static constexpr size_t FLAGS_OFFSET        = 58;
static constexpr size_t PKT_TYPE_OFFSET     = 55;
static constexpr size_t NUM_METADATA_OFFSET = 48;
static constexpr size_t SEQ_NUM_OFFSET      = 32;
static constexpr size_t LENGTH_OFFSET       = 16;
static constexpr size_t DST_EPID_OFFSET     = 0;
static constexpr uint64_t FLAGS_MASK        = ((uint64_t)0x3F << FLAGS_OFFSET);
static constexpr uint64_t PKT_TYPE_MASK     = ((uint64_t)0x7 << PKT_TYPE_OFFSET);
static constexpr uint64_t NUM_METADATA_MASK = ((uint64_t)0x7F << NUM_METADATA_OFFSET);
static constexpr uint64_t SEQ_NUM_MASK      = ((uint64_t)0xFFFF << SEQ_NUM_OFFSET);
static constexpr uint64_t LENGTH_MASK       = ((uint64_t)0xFFFF << LENGTH_OFFSET);
static constexpr uint64_t DST_EPID_MASK     = ((uint64_t)0xFFFF << DST_EPID_OFFSET);

enum class packet_type_t {
    PACKET_TYPE_MGMT         = 0x0, // Management packet
    PACKET_TYPE_STS          = 0x1, // Stream status
    PACKET_TYPE_STC          = 0x2, // Stream Command
    PACKET_TYPE_CTRL         = 0x4, // Control Transaction
    PACKET_TYPE_DATA_NO_TS   = 0x6, // Data Packet without TimeStamp
    PACKET_TYPE_DATA_WITH_TS = 0x7, // Data Packet with TimeStamp
};

/*! Header information of a data packet
 * This is the first 64-bit
 */
template <size_t chdr_w> class chdr_header
{
public:

    ~chdr_header() = default;

    chdr_header(packet_type_t packet_type)
    {
        _set_pkt_type(packet_type);
    }

    chdr_header(uint64_t header_val) : _header_val(header_val)
    {
    }

    /*! Return the word size in bytes
     */
    constexpr size_t get_word_size() const
    {
        return chdr_w / 8;
    }

    /*! Returns the 64 header bits in native byte order
     */
    uint64_t get() const
    {
        return _header_val;
    }

    /*! Get flags field of this CHDR packet
     */
    inline uint8_t get_flags() const
    {
        return _get_value<uint8_t>(_header_val, FLAGS_MASK, FLAGS_OFFSET);
    };

    /*! Get packet type field of this CHDR packet
     */
    inline packet_type_t get_pkt_type() const
    {
        return _get_value<packet_type_t>(_header_val, PKT_TYPE_MASK, PKT_TYPE_OFFSET);
    }

    /*! Get number length of metadata of this CHDR packet in words
     */
    inline uint8_t get_num_metadata() const
    {
        return _get_value<uint8_t>(_header_val, NUM_METADATA_MASK, NUM_METADATA_OFFSET);
    }

    /*! Get number length of metadata of this CHDR packet in bytes
     */
    inline uint8_t get_num_bytes_metadata() const
    {
        return get_num_metadata() * get_word_size();
    }

    /*! Get sequence number field of this CHDR packet
     */
    inline uint16_t get_seq_num() const
    {
        return _get_value<uint16_t>(_header_val, SEQ_NUM_MASK, SEQ_NUM_OFFSET);
    }

    /*! Get length (in bytes) of this CHDR packet
     */
    inline uint16_t get_length() const
    {
        return _get_value<uint16_t>(_header_val, LENGTH_MASK, LENGTH_OFFSET);
    }

    /*! Get dst_epid field of this CHDR packet
     */
    inline uint16_t get_dst_epid() const
    {
        return _get_value<uint16_t>(_header_val, DST_EPID_MASK, DST_EPID_OFFSET);
    }

    /*! Return number of payload in the packet (in CHDR word unit)
     */
    inline size_t get_num_words_payload() const
    {
        return (get_length() / get_word_size()) - get_num_metadata() - 1
               - _has_timestamp(get_pkt_type());
    }

    /*! Return number of payload in the packet (in CHDR word unit)
     */
    inline size_t get_num_bytes_payload() const
    {
        return get_length() - get_num_bytes_metadata() - get_word_size()
               - _has_timestamp(get_pkt_type()) * get_word_size();
    }

    /*! Return offset to payload this offset in bytes unit
     */
    inline size_t get_metadata_offset() const
    {
        return (1 + _has_timestamp(get_pkt_type())) * get_word_size();
    }

    /*! Return offset to payload this offset in bytes unit
     */
    inline size_t get_payload_offset() const
    {
        return get_metadata_offset() + get_num_metadata() * get_word_size();
    }


    /*********** Setters *****************************************************/

    /*! Set flags field of this CHDR packet
     * \param flags value to set to flags field of this CHDR packet
     */
    inline void set_flags(uint8_t flags)
    {
        _set_value(_header_val, flags, FLAGS_OFFSET, FLAGS_MASK);
    }

    /*! Set sequence number field of this CHDR packet
     *\param seq_num value to set to sequence number field of this CHDR packet
     */
    inline void set_seq_num(uint16_t seq_num)
    {
        _set_value(_header_val, seq_num, SEQ_NUM_OFFSET, SEQ_NUM_MASK);
    }

    /*! Set dst_epid field of this CHDR packet
     * \param dst_epid value to set to dst_epid field of CHDR packet
     */
    inline void set_dst_epid(uint16_t dst_epid)
    {
        _set_value(_header_val, dst_epid, DST_EPID_OFFSET, DST_EPID_MASK);
    }

    /*! Set the size of payload and metadata in word unit
     *\param num_payload number of payload in word
     *\param num_metadata number of metadata in word
     */
    inline void set_packet_size(size_t num_payload, size_t num_metadata = 0)
    {
        _set_num_metadata(num_metadata);
        _update_length(num_metadata, num_payload);
    }

private:
    /*! The actual header bits, stored in native (host) byte order
     */
    uint64_t _header_val = 0;

    template <typename value_t, typename data_t>
    inline value_t _get_value(const data_t data, data_t mask, size_t offset) const
    {
        return static_cast<value_t>((data & mask) >> offset);
    }

    template <typename data_t, typename value_t>
    inline void _set_value(data_t& data, value_t value, size_t offset, data_t mask)
    {
        data = (data & ~mask) | (static_cast<data_t>(value) << offset);
    }

    inline uint8_t _has_timestamp(packet_type_t pkt_type) const
    {
        return pkt_type == packet_type_t::PACKET_TYPE_DATA_WITH_TS and chdr_w == 64 ? 1
                                                                                    : 0;
    }

    inline void _update_length(uint8_t num_metadata, size_t num_payload)
    {
        _set_length(static_cast<uint16_t>(
            (num_metadata + num_payload + 1 + _has_timestamp(get_pkt_type())) * chdr_w
            / 8));
    }

    inline void _set_length(uint16_t length)
    {
        _set_value(_header_val, length, LENGTH_OFFSET, LENGTH_MASK);
    }

    inline void _set_pkt_type(packet_type_t pkt_type)
    {
        _set_value(_header_val, pkt_type, PKT_TYPE_OFFSET, PKT_TYPE_MASK);
    }

    /*! Set number of metadata (in word) of this CHDR packet
     * This function also update the total length field of this CHDR packet
     * \param num_metadata number of metadata (in word) of this CHDR packet
     */
    inline void _set_num_metadata(uint8_t num_metadata)
    {
        _set_value(_header_val, num_metadata, NUM_METADATA_OFFSET, NUM_METADATA_MASK);
    }
};

}}} // namespace uhd::rfnoc::chdr

#endif /* INCLUDED_RFNOC_CHDR_HEADER_HPP */
