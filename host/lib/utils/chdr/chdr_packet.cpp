//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/utils/chdr/chdr_packet.hpp>
#include <uhdlib/rfnoc/chdr_packet_writer.hpp>
#include <boost/format.hpp>

namespace chdr_rfnoc = uhd::rfnoc::chdr;
namespace chdr_util  = uhd::utils::chdr;
using namespace uhd;

chdr_util::chdr_packet::chdr_packet(uhd::rfnoc::chdr_w_t chdr_w,
    chdr_rfnoc::chdr_header header,
    std::vector<uint8_t> payload,
    boost::optional<uint64_t> timestamp,
    std::vector<uint64_t> mdata)
    : _chdr_w(chdr_w)
    , _header(header)
    , _payload(std::move(payload))
    , _timestamp(timestamp)
    , _mdata(std::move(mdata))
{
    set_header_lengths();
}

inline static uint64_t u64_to_host(uhd::endianness_t endianness, uint64_t word)
{
    return (endianness == ENDIANNESS_BIG) ? uhd::ntohx<uint64_t>(word)
                                          : uhd::wtohx<uint64_t>(word);
}

inline static uint64_t u64_from_host(uhd::endianness_t endianness, uint64_t word)
{
    return (endianness == ENDIANNESS_BIG) ? uhd::htonx<uint64_t>(word)
                                          : uhd::htowx<uint64_t>(word);
}

chdr_rfnoc::chdr_header chdr_util::chdr_packet::get_header() const
{
    return this->_header;
}

void chdr_util::chdr_packet::set_header(chdr_rfnoc::chdr_header header)
{
    _header = header;
    set_header_lengths();
}

boost::optional<uint64_t> chdr_util::chdr_packet::get_timestamp() const
{
    return this->_timestamp;
}

void chdr_util::chdr_packet::set_timestamp(boost::optional<uint64_t> timestamp)
{
    _timestamp = timestamp;
    set_header_lengths();
}

std::vector<uint8_t> chdr_util::chdr_packet::serialize_to_byte_vector(
    endianness_t endianness) const
{
    std::vector<uint8_t> buffer(get_packet_len(), 0);
    serialize(buffer.begin(), buffer.end(), endianness);
    return buffer;
}

void chdr_util::chdr_packet::serialize_ptr(
    endianness_t endianness, void* start, void* end) const
{
    size_t len = static_cast<uint8_t*>(end) - static_cast<uint8_t*>(start);
    UHD_ASSERT_THROW(get_packet_len() <= len);
    chdr_rfnoc::chdr_packet_factory factory(_chdr_w, endianness);
    chdr_rfnoc::chdr_packet_writer::uptr packet_writer =
        factory.make_generic(std::numeric_limits<size_t>::max());
    chdr_rfnoc::chdr_header header = _header;
    packet_writer->refresh(start, header, (_timestamp ? *_timestamp : 0));

    uint64_t* mdata_ptr  = static_cast<uint64_t*>(packet_writer->get_mdata_ptr());
    auto mdata_src_begin = this->_mdata.begin();
    auto mdata_src_end   = this->_mdata.end();
    std::transform(
        mdata_src_begin, mdata_src_end, mdata_ptr, [endianness](uint64_t value) {
            return u64_from_host(endianness, value);
        });

    uint8_t* payload_ptr = static_cast<uint8_t*>(packet_writer->get_payload_ptr());
    std::copy(_payload.begin(), _payload.end(), payload_ptr);
}

chdr_util::chdr_packet chdr_util::chdr_packet::deserialize_ptr(
    uhd::rfnoc::chdr_w_t chdr_w,
    endianness_t endianness,
    const void* start,
    const void* end)
{
    chdr_rfnoc::chdr_packet_factory factory(chdr_w, endianness);
    chdr_rfnoc::chdr_packet_writer::uptr packet_writer =
        factory.make_generic(std::numeric_limits<size_t>::max());
    packet_writer->refresh(start);

    chdr_rfnoc::chdr_header header      = packet_writer->get_chdr_header();
    boost::optional<uint64_t> timestamp = packet_writer->get_timestamp();

    const size_t mdata_size_words = packet_writer->get_mdata_size() / 8;
    const uint64_t* mdata_src_begin =
        static_cast<const uint64_t*>(packet_writer->get_mdata_const_ptr());
    const uint64_t* mdata_src_end = mdata_src_begin + mdata_size_words;
    std::vector<uint64_t> mdata(mdata_size_words, 0);
    uint64_t* mdata_ptr = mdata.data();
    UHD_ASSERT_THROW(mdata_src_end < static_cast<const uint64_t*>(end));
    std::transform(
        mdata_src_begin, mdata_src_end, mdata_ptr, [endianness](uint64_t value) {
            return u64_to_host(endianness, value);
        });

    size_t payload_size = packet_writer->get_payload_size();
    const uint8_t* payload_begin =
        static_cast<const uint8_t*>(packet_writer->get_payload_const_ptr());
    const uint8_t* payload_end = payload_begin + payload_size;
    std::vector<uint8_t> payload(payload_size, 0);
    UHD_ASSERT_THROW(payload_end <= static_cast<const uint8_t*>(end));
    std::copy(payload_begin, payload_end, payload.begin());

    return chdr_util::chdr_packet(chdr_w, header, payload, timestamp, mdata);
}

void chdr_util::chdr_packet::set_metadata(std::vector<uint64_t> metadata)
{
    _mdata = std::move(metadata);
    set_header_lengths();
}

const std::vector<uint64_t>& chdr_util::chdr_packet::get_metadata() const
{
    return _mdata;
}

size_t chdr_util::chdr_packet::get_packet_len() const
{
    size_t chdr_w_bytes = uhd::rfnoc::chdr_w_to_bits(_chdr_w) / 8;
    // The timestamp and header take up 2 chdr_w lengths only when CHDR_W = 64 bits and
    // there is a timestamp (RFNoC Specifcation section 2.2.1)
    return _payload.size() * sizeof(uint8_t) + _mdata.size() * sizeof(uint64_t)
           + ((_timestamp != boost::none && _chdr_w == uhd::rfnoc::CHDR_W_64) ? 2 : 1)
                 * chdr_w_bytes;
}

const std::vector<uint8_t>& chdr_util::chdr_packet::get_payload_bytes() const
{
    return _payload;
}

void chdr_util::chdr_packet::set_payload_bytes(std::vector<uint8_t> bytes)
{
    this->_payload = std::move(bytes);
    set_header_lengths();
}

//! Return a string representation of this object
std::string chdr_util::chdr_packet::to_string() const
{
    return str(boost::format("chdr_packet{chdr_w:%u}\n%s")
               % uhd::rfnoc::chdr_w_to_bits(_chdr_w) % _header.to_string());
}
