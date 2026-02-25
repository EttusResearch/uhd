//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/exception.hpp>
#include <uhd/types/endianness.hpp>

namespace chdr_rfnoc = uhd::rfnoc::chdr;
namespace uhd { namespace utils { namespace chdr {

template <typename payload_t>
chdr_packet::chdr_packet(uhd::rfnoc::chdr_w_t chdr_w,
    chdr_rfnoc::chdr_header header,
    payload_t payload,
    std::optional<uint64_t> timestamp,
    std::vector<uint64_t> metadata)
    : chdr_packet(chdr_w, header, std::vector<uint8_t>(), timestamp, std::move(metadata))
{
    set_payload(payload);
}

template <typename OutputIterator>
void chdr_packet::serialize(
    OutputIterator first, OutputIterator last, uhd::endianness_t endianness) const
{
    void* start = static_cast<void*>(&*first);
    void* end   = static_cast<void*>(&*last);
    serialize_ptr(endianness, start, end);
}

template <typename InputIterator>
chdr_packet chdr_packet::deserialize(uhd::rfnoc::chdr_w_t chdr_w,
    InputIterator first,
    InputIterator last,
    uhd::endianness_t endianness)
{
    void* start = static_cast<void*>(&*first);
    void* end   = static_cast<void*>(&*last);
    return deserialize_ptr(chdr_w, endianness, start, end);
}

// Generic get_payload for 64-bit-serialized payload types: strs_payload,
// strc_payload, and mgmt_payload.
template <typename payload_t>
payload_t chdr_packet::get_payload(uhd::endianness_t endianness) const
{
    payload_t payload;
    UHD_ASSERT_THROW(this->_payload.size() % sizeof(uint64_t) == 0);
    const size_t buff_size_words = this->_payload.size() / sizeof(uint64_t);
    auto conv_byte_order         = [endianness](uint64_t x) -> uint64_t {
        return (endianness == uhd::ENDIANNESS_BIG) ? uhd::htonx<uint64_t>(x)
                                                           : uhd::htowx<uint64_t>(x);
    };
    payload.deserialize(reinterpret_cast<const uint64_t*>(this->_payload.data()),
        buff_size_words,
        conv_byte_order);
    return payload;
}

// Generic set_payload for 64-bit-serialized payload types: strs_payload,
// strc_payload, and mgmt_payload.
template <typename payload_t>
void chdr_packet::set_payload(payload_t payload, uhd::endianness_t endianness)
{
    _header.set_pkt_type(chdr_rfnoc::payload_to_packet_type<payload_t>());
    const size_t payload_len_bytes = payload.get_length();
    this->_payload.resize(payload_len_bytes, 0);
    auto conv_byte_order = [endianness](uint64_t x) -> uint64_t {
        return (endianness == uhd::ENDIANNESS_BIG) ? uhd::htonx<uint64_t>(x)
                                                   : uhd::htowx<uint64_t>(x);
    };
    payload.serialize(reinterpret_cast<uint64_t*>(this->_payload.data()),
        this->_payload.size(),
        conv_byte_order);
    set_header_lengths();
}

// get_payload specialization for 32-bit ctrl_payload words.
template <>
chdr_rfnoc::ctrl_payload chdr_packet::get_payload<chdr_rfnoc::ctrl_payload>(
    uhd::endianness_t endianness) const
{
    chdr_rfnoc::ctrl_payload payload;
    UHD_ASSERT_THROW(this->_payload.size() % sizeof(uint32_t) == 0);
    const size_t buff_size_words = this->_payload.size() / sizeof(uint32_t);
    auto conv_byte_order         = [endianness](uint32_t x) -> uint32_t {
        return (endianness == uhd::ENDIANNESS_BIG) ? uhd::htonx<uint32_t>(x)
                                                           : uhd::htowx<uint32_t>(x);
    };
    payload.deserialize(reinterpret_cast<const uint32_t*>(this->_payload.data()),
        buff_size_words,
        conv_byte_order);
    return payload;
}

// get_payload specialization for 32-bit ctrl_payload words.
template <>
void chdr_packet::set_payload<chdr_rfnoc::ctrl_payload>(
    chdr_rfnoc::ctrl_payload payload, uhd::endianness_t endianness)
{
    _header.set_pkt_type(chdr_rfnoc::payload_to_packet_type<chdr_rfnoc::ctrl_payload>());
    // ctrl_payload::serialize() returns exactly get_length() bytes with no
    // padding, so allocate precisely that amount.
    const size_t payload_len_bytes = payload.get_length();
    this->_payload.resize(payload_len_bytes, 0);
    auto conv_byte_order = [endianness](uint32_t x) -> uint32_t {
        return (endianness == uhd::ENDIANNESS_BIG) ? uhd::htonx<uint32_t>(x)
                                                   : uhd::htowx<uint32_t>(x);
    };
    payload.serialize(reinterpret_cast<uint32_t*>(this->_payload.data()),
        this->_payload.size(),
        conv_byte_order);
    set_header_lengths();
}

template <typename payload_t>
std::string chdr_packet::to_string_with_payload(uhd::endianness_t endianness) const
{
    payload_t payload = this->get_payload<payload_t>(endianness);
    return to_string() + payload.to_string();
}

template <>
std::string chdr_packet::to_string_with_payload<chdr_rfnoc::mgmt_payload>(
    uhd::endianness_t endianness) const
{
    chdr_rfnoc::mgmt_payload payload =
        this->get_payload<chdr_rfnoc::mgmt_payload>(endianness);
    return to_string() + payload.to_string() + payload.hops_to_string();
}

}}} // namespace uhd::utils::chdr
