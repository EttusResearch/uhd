//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/rfnoc/chdr_types.hpp>
#include <uhd/rfnoc/rfnoc_types.hpp>
#include <uhd/types/endianness.hpp>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace uhd { namespace utils { namespace chdr {

/*! A Generic class that represents a CHDR Packet
 *
 * This class is not used by UHD, it is a helper class for writing tests or
 * other types of utilities. The class has the capability of representing any
 * CHDR packet, and serializing this class into a CHDR packet (or deserializing
 * from a buffer to populate this class). It is not optimized for high
 * performance.
 * For example, this is used in UHD unit tests, and in the USRP simulator to
 * provide an easy way to generate CHDR packets from Python.
 *
 * Whether the packet has a specific type of payload is not specified, but of
 * course the CHDR header can be set as desired, and a payload can be provided
 * to the constructor.
 */
class UHD_API chdr_packet
{
public:
    /*! Constructs a CHDR Packet from a header and a payload
     *
     * This is commonly used to create a non-data packet (e.g., control,
     * stream status, etc.).
     *
     * \param chdr_w CHDR width
     * \param header CHDR header
     * \param payload The payload object (e.g., a chdr_rfnoc::ctrl_payload if
     *                that's the type of payload_t).
     * \param timestamp Timestamp, defaults to no timestamp in the header.
     * \param metadata Metadata, defaults to no metadata.
     */
    template <typename payload_t>
    chdr_packet(uhd::rfnoc::chdr_w_t chdr_w,
        uhd::rfnoc::chdr::chdr_header header,
        payload_t payload,
        std::optional<uint64_t> timestamp = {},
        std::vector<uint64_t> metadata    = {});

    /*! Construct a CHDR Packet from a header and raw payload words
     *
     * This is the typical way to construct data packets, or to manually
     * construct non-data packets by bypassing the corresponding payload types.
     *
     * \param chdr_w CHDR width
     * \param header CHDR header
     * \param payload_data The payload data as bytes, i.e., whatever follows
     *                     the CHDR header.
     * \param timestamp Timestamp, defaults to no timestamp in the header.
     * \param metadata Metadata, defaults to no metadata.
     */
    chdr_packet(uhd::rfnoc::chdr_w_t chdr_w,
        uhd::rfnoc::chdr::chdr_header header,
        std::vector<uint8_t> payload_data,
        std::optional<uint64_t> timestamp = {},
        std::vector<uint64_t> metadata    = {});

    /*! Returns the contents of the CHDR header
     *
     * \return The CHDR header
     */
    uhd::rfnoc::chdr::chdr_header get_header() const;

    /*! Change this object's header
     *
     * \param header The new CHDR header
     */
    void set_header(uhd::rfnoc::chdr::chdr_header header);

    /*! Returns the timestamp in the packet as an optional value
     *
     * \return A std::optional which if initialized has the timestamp
     */
    std::optional<uint64_t> get_timestamp() const;

    /*! Sets the timestamp in the packet
     *
     * \param timestamp the timestamp to set, or an empty optional for no
     *                  timestamp
     */
    void set_timestamp(std::optional<uint64_t> timestamp);

    /*! Returns a const reference to the metadata
     *
     * \return The metadata reference
     */
    const std::vector<uint64_t>& get_metadata() const;

    /*! Sets the metadata for a CHDR packet and updates the headers
     *
     * \param metadata A vector containing the new metadata
     */
    void set_metadata(std::vector<uint64_t> metadata);

    /*! Serialize a CHDR Packet into a vector of bytes
     *
     * \param endianness the endianness of the returned vector (link endianness)
     * \return a vector of bytes which represents the CHDR Packet in serialize form
     */
    std::vector<uint8_t> serialize_to_byte_vector(
        endianness_t endianness = uhd::ENDIANNESS_LITTLE) const;

    /*! Serialize a CHDR Packet into a buffer
     *
     * \param endianness the endianness of the output buffer (link endianness)
     * \param first the start of the output buffer
     * \param last the end of the output buffer
     */
    template <typename OutputIterator>
    void serialize(OutputIterator first,
        OutputIterator last,
        endianness_t endianness = uhd::ENDIANNESS_LITTLE) const;

    /*! Deserialize a CHDR Packet from a buffer of bytes
     *
     * \param chdr_w the CHDR_W of the incoming packet
     * \param endianness the endianness of the input buffer (link endianness)
     * \param first the start of the input buffer
     * \param last the end of the input buffer
     * \return a CHDR Packet with an unspecified payload type
     */
    template <typename InputIterator>
    static chdr_packet deserialize(uhd::rfnoc::chdr_w_t chdr_w,
        InputIterator first,
        InputIterator last,
        endianness_t endianness = uhd::ENDIANNESS_LITTLE);

    /*! Get the total serialized length of the packet
     *
     * \return The length of the packet in bytes
     */
    size_t get_packet_len() const;

    /*! Returns a const reference to the payload bytes
     *
     * \return The payload reference
     */
    const std::vector<uint8_t>& get_payload_bytes() const;

    /*! Sets the current payload
     *
     * \param bytes the payload to store inside this object (It is moved from)
     */
    void set_payload_bytes(std::vector<uint8_t> bytes);

    /*! Parses the data out of this objects payload field into a payload_t object
     *
     * \param endianness The link endianness of the CHDR Link
     * \return The parsed payload_t object
     */
    template <typename payload_t>
    payload_t get_payload(uhd::endianness_t endianness = uhd::ENDIANNESS_LITTLE) const;

    /*! Serializes the payload object into bytes and stores it in this object's payload
     * field
     *
     * \param payload the payload object to store
     * \param endianness The link endianness of the CHDR Link
     */
    template <typename payload_t>
    void set_payload(
        payload_t payload, uhd::endianness_t endianness = uhd::ENDIANNESS_LITTLE);

    //! Return a string representation of this object
    std::string to_string() const;

    //! Return a string representation of this object and deserialize its payload
    template <typename payload_t>
    std::string to_string_with_payload(
        uhd::endianness_t endianness = uhd::ENDIANNESS_LITTLE) const;

private:
    void serialize_ptr(endianness_t endianness, void* start, void* end) const;

    static chdr_packet deserialize_ptr(uhd::rfnoc::chdr_w_t chdr_w,
        endianness_t endianness,
        const void* start,
        const void* end);

    inline void set_header_lengths()
    {
        _header.set_num_mdata(_mdata.size() / (uhd::rfnoc::chdr_w_to_bits(_chdr_w) / 64));
        _header.set_length(get_packet_len());
    }

    uhd::rfnoc::chdr_w_t _chdr_w;
    uhd::rfnoc::chdr::chdr_header _header;
    std::vector<uint8_t> _payload;
    std::optional<uint64_t> _timestamp;
    std::vector<uint64_t> _mdata;
};

}}} // namespace uhd::utils::chdr

#include <uhd/utils/chdr/chdr_packet.ipp>
