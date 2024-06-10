//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/rfnoc/chdr_types.hpp>
#include <uhd/rfnoc/constants.hpp>
#include <uhd/types/endianness.hpp>
#include <uhd/utils/byteswap.hpp>
#include <limits>

namespace uhd { namespace rfnoc { namespace chdr {

//----------------------------------------------------
// Generic CHDR Packet Container
//----------------------------------------------------

//! A container class that wraps a generic buffer that contains a CHDR packet. The
//  container provides a high level API to read and write the header, metadata and payload
//  of the packet. The payload can be accessed as a generic buffer using this interface.
//
class chdr_packet_writer
{
public:
    //! A unique pointer to a const chdr_packet_writer. Useful as a read-only interface.
    typedef std::unique_ptr<const chdr_packet_writer> cuptr;
    //! A unique pointer to a non-const chdr_packet_writer. Useful as a read-write
    //! interface.
    typedef std::unique_ptr<chdr_packet_writer> uptr;

    virtual ~chdr_packet_writer() = 0;

    /*! Updates the underlying storage of this packet. This is a const method and is
     *  only useful for read-only (RX) access.
     *
     * \param pkt_buff Pointer to a buffer that contains the RX packet
     */
    virtual void refresh(const void* pkt_buff) const = 0;

    /*! Updates the underlying storage of this packet, and populates it with the specified
     *  arguments. This is a non-const method and is useful for read-write (TX) access.
     *
     * \param pkt_buff Pointer to a buffer that should be populated with the TX packet
     * \param header The CHDR header to fill into the TX packet
     * \param timestamp The timestamp to fill into the TX packet (if requested)
     */
    virtual void refresh(void* pkt_buff, chdr_header& header, uint64_t timestamp = 0) = 0;

    /*! Updates the CHDR header with the written payload size
     *
     * \param payload_size_bytes The payload size in bytes
     */
    virtual void update_payload_size(size_t payload_size_bytes) = 0;

    /*! Returns a class that represents the contents of the CHDR header
     *
     * \return The CHDR header
     */
    virtual chdr_header get_chdr_header() const = 0;

    /*! Returns the timestamp in the packet as an optional value
     *
     * \return A boost::optional which if initialized has the timestamp
     */
    virtual boost::optional<uint64_t> get_timestamp() const = 0;

    /*! Returns the endianness of the metadata and payload buffers
     *
     * \return The byte order as a uhd::endianness_t
     */
    virtual endianness_t get_byte_order() const = 0;

    /*! Returns the maximum transfer unit in bytes
     *
     * \return The maximum transfer unit in bytes
     */
    virtual size_t get_mtu_bytes() const = 0;

    /*! Returns the metadata size in bytes
     *
     * \return The size in bytes
     */
    virtual size_t get_mdata_size() const = 0;

    /*! Returns a const void pointer to the metadata section in the packet
     *
     * \return A pointer to the metadata
     */
    virtual const void* get_mdata_const_ptr() const = 0;

    /*! Returns a non-const void pointer to the metadata section in the packet
     *
     * \return A pointer to the metadata
     */
    virtual void* get_mdata_ptr() = 0;

    /*! Returns the payload size in bytes
     *
     * \return The size in bytes
     */
    virtual size_t get_payload_size() const = 0;

    /*! Returns a const void pointer to the payload section in the packet
     *
     * \return A pointer to the payload
     */
    virtual const void* get_payload_const_ptr() const = 0;

    /*! Returns a non-const void pointer to the payload section in the packet
     *
     * \return A pointer to the payload
     */
    virtual void* get_payload_ptr() = 0;

    /*! Return the payload offset in bytes for a given type and num_mdata
     *
     * \param pkt_type The packet type for calculation
     * \param num_mdata The number of metadata words for calculation
     * \return The offset of the payload in a packet with the given params
     */
    virtual size_t calculate_payload_offset(
        const packet_type_t pkt_type, const uint8_t num_mdata = 0) const = 0;

    //! Shortcut to return the const metadata pointer cast as a specific type
    template <typename data_t>
    inline const data_t* get_mdata_const_ptr_as() const
    {
        return reinterpret_cast<const data_t*>(get_mdata_const_ptr());
    }

    //! Shortcut to return the non-const metadata pointer cast as a specific type
    template <typename data_t>
    inline data_t* get_mdata_ptr_as()
    {
        return reinterpret_cast<data_t*>(get_mdata_ptr());
    }

    //! Shortcut to return the const payload pointer cast as a specific type
    template <typename data_t>
    inline const data_t* get_payload_const_ptr_as() const
    {
        return reinterpret_cast<const data_t*>(get_payload_const_ptr());
    }

    //! Shortcut to return the non-const payload pointer cast as a specific type
    template <typename data_t>
    inline data_t* get_payload_ptr_as()
    {
        return reinterpret_cast<data_t*>(get_payload_ptr());
    }

    //! Return a function to convert a word of type data_t to host order
    template <typename data_t>
    std::function<data_t(data_t)> conv_to_host() const
    {
        return (get_byte_order() == uhd::ENDIANNESS_BIG) ? uhd::ntohx<data_t>
                                                         : uhd::wtohx<data_t>;
    }

    //! Return a function to convert a word of type data_t from host order
    template <typename data_t>
    std::function<data_t(data_t)> conv_from_host() const
    {
        return (get_byte_order() == uhd::ENDIANNESS_BIG) ? uhd::htonx<data_t>
                                                         : uhd::htowx<data_t>;
    }
};

//----------------------------------------------------
// Container for specific CHDR Packets
//----------------------------------------------------

//! A container class that wraps a generic buffer that contains a CHDR packet. The
//  container provides a high level API to read and write the header, metadata and payload
//  of the packet. The payload can be accessed as a specific type that will be serialized
//  and deserialized appropriately.
//
template <typename payload_t>
class chdr_packet_writer_specific
{
public:
    //! A unique pointer to a const chdr_packet_writer. Useful as a read-only interface.
    typedef std::unique_ptr<const chdr_packet_writer_specific<payload_t>> cuptr;
    //! A unique pointer to a non-const chdr_packet_writer. Useful as a read-write
    //! interface.
    typedef std::unique_ptr<chdr_packet_writer_specific<payload_t>> uptr;

    chdr_packet_writer_specific(chdr_packet_writer::uptr chdr_pkt)
        : _chdr_pkt(std::move(chdr_pkt))
    {
    }
    ~chdr_packet_writer_specific() = default;

    //! Updates the underlying storage of this packet. This is a const method and is
    //  only useful for read-only access.
    inline void refresh(const void* pkt_buff) const
    {
        _chdr_pkt->refresh(pkt_buff);
    }

    //! Updates the underlying storage of this packet, and populates it with the specified
    //  arguments. This is a non-const method and is useful for read-write access.
    inline void refresh(void* pkt_buff, chdr_header& header, const payload_t& payload)
    {
        payload.populate_header(header);
        _chdr_pkt->refresh(pkt_buff, header);
        size_t bytes_copied = payload.serialize(_chdr_pkt->get_payload_ptr_as<uint64_t>(),
            _chdr_pkt->get_mtu_bytes(),
            _chdr_pkt->conv_from_host<uint64_t>());
        _chdr_pkt->update_payload_size(bytes_copied);
        header = _chdr_pkt->get_chdr_header();
    }

    //! Returns a class that represents the contents of the CHDR header
    inline chdr_header get_chdr_header() const
    {
        return _chdr_pkt->get_chdr_header();
    }

    //! Returns a class that represents the contents of the CHDR payload
    inline payload_t get_payload() const
    {
        payload_t payload;
        payload.deserialize(_chdr_pkt->get_payload_const_ptr_as<uint64_t>(),
            _chdr_pkt->get_payload_size() / sizeof(uint64_t),
            _chdr_pkt->conv_to_host<uint64_t>());
        return payload;
    }

    //! Fills the CHDR payload into the specified parameter
    inline void fill_payload(payload_t& payload) const
    {
        payload.deserialize(_chdr_pkt->get_payload_const_ptr_as<uint64_t>(),
            _chdr_pkt->get_payload_size() / sizeof(uint64_t),
            _chdr_pkt->conv_to_host<uint64_t>());
    }

private:
    chdr_packet_writer::uptr _chdr_pkt;
};

//----------------------------------------------------
// Specific CHDR packet types
//----------------------------------------------------

//! CHDR control packet
typedef chdr_packet_writer_specific<ctrl_payload> chdr_ctrl_packet;

//! CHDR stream status packet
typedef chdr_packet_writer_specific<strs_payload> chdr_strs_packet;

//! CHDR stream command packet
typedef chdr_packet_writer_specific<strc_payload> chdr_strc_packet;

//! CHDR management packet
typedef chdr_packet_writer_specific<mgmt_payload> chdr_mgmt_packet;

//----------------------------------------------------
// CHDR packet factory
//----------------------------------------------------

//! A copyable and movable factory class that is capable of generating generic and
//! specific CHDR packet containers.
//
class chdr_packet_factory
{
public:
    //! A parametrized ctor that takes in all the info required to generate a CHDR packet
    //
    // \param chdr_w The CHDR width of the remote device
    // \param endianness The endianness of the link being used (e.g., Ethernet
    //                   typically uses big-endian, PCIe typically uses
    //                   little-endian). Note: The host endianness is
    //                   automatically derived.
    chdr_packet_factory(chdr_w_t chdr_w, endianness_t endianness);
    chdr_packet_factory()                               = delete;
    chdr_packet_factory(const chdr_packet_factory& rhs) = default;
    chdr_packet_factory(chdr_packet_factory&& rhs)      = default;

    //! Makes a generic CHDR packet and transfers ownership to the client
    chdr_packet_writer::uptr make_generic(
        size_t mtu_bytes = std::numeric_limits<size_t>::max()) const;

    //! Makes a CHDR control packet and transfers ownership to the client
    chdr_ctrl_packet::uptr make_ctrl(
        size_t mtu_bytes = std::numeric_limits<size_t>::max()) const;

    //! Makes a CHDR stream status packet and transfers ownership to the client
    chdr_strs_packet::uptr make_strs(
        size_t mtu_bytes = std::numeric_limits<size_t>::max()) const;

    //! Makes a CHDR stream cmd packet and transfers ownership to the client
    chdr_strc_packet::uptr make_strc(
        size_t mtu_bytes = std::numeric_limits<size_t>::max()) const;

    //! Makes a CHDR management packet and transfers ownership to the client
    chdr_mgmt_packet::uptr make_mgmt(
        size_t mtu_bytes = std::numeric_limits<size_t>::max()) const;

    //! Get the CHDR width
    inline chdr_w_t get_chdr_w() const
    {
        return _chdr_w;
    }

    //! Get the link endianness
    inline endianness_t get_endianness() const
    {
        return _endianness;
    }

    //! Get the protocol version for RFNoC and the CHDR format
    inline uint16_t get_protover() const
    {
        return RFNOC_PROTO_VER;
    }

private:
    const chdr_w_t _chdr_w;
    const endianness_t _endianness;
};

}}} // namespace uhd::rfnoc::chdr
