//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/rfnoc/chdr_types.hpp>
#include <boost/optional.hpp>
#include <memory>
#include <vector>

namespace uhd { namespace utils { namespace chdr {

/*! A Generic class that represents a CHDR Packet
 *
 * Whether the packet has a specific type of payload is not specified
 */
class UHD_API chdr_packet
{
public:
    /*! Constructs a CHDR Packet from a header and a payload
     *
     * timestamp and metadata are optional and will be empty if omitted
     */
    template <typename payload_t>
    chdr_packet(uhd::rfnoc::chdr_w_t chdr_w,
        uhd::rfnoc::chdr::chdr_header header,
        payload_t payload,
        boost::optional<uint64_t> timestamp = boost::none,
        std::vector<uint64_t> metadata      = {});

    /*! Construct a CHDR Packet from a header and raw payload words
     *
     * timestamp and metadata are optional and will be empty if omitted
     */
    chdr_packet(uhd::rfnoc::chdr_w_t chdr_w,
        uhd::rfnoc::chdr::chdr_header header,
        std::vector<uint8_t> payload_data,
        boost::optional<uint64_t> timestamp = boost::none,
        std::vector<uint64_t> mdata         = {});

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
     * \return A boost::optional which if initialized has the timestamp
     */
    boost::optional<uint64_t> get_timestamp() const;

    /*! Sets the timestamp in the packet
     *
     * \param timestamp the timestamp to set, or boost::none for no timestamp
     */
    void set_timestamp(boost::optional<uint64_t> timestamp);

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
    boost::optional<uint64_t> _timestamp;
    std::vector<uint64_t> _mdata;
};

}}} // namespace uhd::utils::chdr

#include <uhd/utils/chdr/chdr_packet.ipp>
