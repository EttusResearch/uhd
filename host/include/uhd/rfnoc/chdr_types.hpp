//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/rfnoc/rfnoc_types.hpp>
#include <uhd/types/endianness.hpp>
#include <uhd/utils/byteswap.hpp>
#include <boost/format.hpp>
#include <boost/optional.hpp>
#include <deque>
#include <list>
#include <memory>
#include <vector>

namespace uhd { namespace rfnoc { namespace chdr {

enum packet_type_t {
    PKT_TYPE_MGMT         = 0x0, //! Management packet
    PKT_TYPE_STRS         = 0x1, //! Stream status
    PKT_TYPE_STRC         = 0x2, //! Stream Command
    PKT_TYPE_CTRL         = 0x4, //! Control Transaction
    PKT_TYPE_DATA_NO_TS   = 0x6, //! Data Packet without TimeStamp
    PKT_TYPE_DATA_WITH_TS = 0x7, //! Data Packet with TimeStamp
};

//----------------------------------------------------
// CHDR Header
//----------------------------------------------------

class chdr_header
{
public: // Functions
    chdr_header()                       = default;
    chdr_header(const chdr_header& rhs) = default;
    chdr_header(chdr_header&& rhs)      = default;

    //! Unpack the header from a uint64_t
    chdr_header(uint64_t flat_hdr) : _flat_hdr(flat_hdr) {}

    //! Get the virtual channel field (6 bits)
    inline uint8_t get_vc() const
    {
        return get_field<uint8_t>(_flat_hdr, VC_OFFSET, VC_WIDTH);
    }

    //! Set the virtual channel field (6 bits)
    inline void set_vc(uint8_t vc)
    {
        _flat_hdr = set_field(_flat_hdr, vc, VC_OFFSET, VC_WIDTH);
    }

    //! Get the end-of-burst flag (1 bit)
    inline bool get_eob() const
    {
        return get_field<bool>(_flat_hdr, EOB_OFFSET, EOB_WIDTH);
    }

    //! Set the end-of-burst flag (1 bit)
    inline void set_eob(bool eob)
    {
        _flat_hdr = set_field(_flat_hdr, eob, EOB_OFFSET, EOB_WIDTH);
    }

    //! Get the end-of-vector flag (1 bit)
    inline bool get_eov() const
    {
        return get_field<bool>(_flat_hdr, EOV_OFFSET, EOV_WIDTH);
    }

    //! Set the end-of-vector flag (1 bit)
    inline void set_eov(bool eov)
    {
        _flat_hdr = set_field(_flat_hdr, eov, EOV_OFFSET, EOV_WIDTH);
    }

    //! Get the packet type field (3 bits)
    inline packet_type_t get_pkt_type() const
    {
        return get_field<packet_type_t>(_flat_hdr, PKT_TYPE_OFFSET, PKT_TYPE_WIDTH);
    }

    //! Set the packet type field (3 bits)
    inline void set_pkt_type(packet_type_t pkt_type)
    {
        _flat_hdr = set_field(_flat_hdr, pkt_type, PKT_TYPE_OFFSET, PKT_TYPE_WIDTH);
    }

    //! Get number of metadata words field (5 bits)
    inline uint8_t get_num_mdata() const
    {
        return get_field<uint8_t>(_flat_hdr, NUM_MDATA_OFFSET, NUM_MDATA_WIDTH);
    }

    //! Set number of metadata words field (5 bits)
    inline void set_num_mdata(uint8_t num_mdata)
    {
        _flat_hdr = set_field(_flat_hdr, num_mdata, NUM_MDATA_OFFSET, NUM_MDATA_WIDTH);
    }

    //! Get the sequence number field (16 bits)
    inline uint16_t get_seq_num() const
    {
        return get_field<uint16_t>(_flat_hdr, SEQ_NUM_OFFSET, SEQ_NUM_WIDTH);
    }

    //! Set the sequence number field (16 bits)
    inline void set_seq_num(uint16_t seq_num)
    {
        _flat_hdr = set_field(_flat_hdr, seq_num, SEQ_NUM_OFFSET, SEQ_NUM_WIDTH);
    }

    //! Get the packet length field (16 bits)
    inline uint16_t get_length() const
    {
        return get_field<uint16_t>(_flat_hdr, LENGTH_OFFSET, LENGTH_WIDTH);
    }

    //! Set the packet length field (16 bits)
    inline void set_length(uint16_t length)
    {
        _flat_hdr = set_field(_flat_hdr, length, LENGTH_OFFSET, LENGTH_WIDTH);
    }

    //! Get the destination EPID field (16 bits)
    inline uint16_t get_dst_epid() const
    {
        return get_field<uint16_t>(_flat_hdr, DST_EPID_OFFSET, DST_EPID_WIDTH);
    }

    //! Set the destination EPID field (16 bits)
    inline void set_dst_epid(uint16_t dst_epid)
    {
        _flat_hdr = set_field(_flat_hdr, dst_epid, DST_EPID_OFFSET, DST_EPID_WIDTH);
    }

    //! Pack the header into a uint64_t
    inline uint64_t pack() const
    {
        return _flat_hdr;
    }

    //! Pack the header into a uint64_t as an implicit cast
    inline operator uint64_t() const
    {
        return pack();
    }

    //! Comparison operator (==)
    inline bool operator==(const chdr_header& rhs) const
    {
        return _flat_hdr == rhs._flat_hdr;
    }

    //! Comparison operator (!=)
    inline bool operator!=(const chdr_header& rhs) const
    {
        return _flat_hdr != rhs._flat_hdr;
    }

    //! Assignment operator (=) from a chdr_header
    inline const chdr_header& operator=(const chdr_header& rhs)
    {
        _flat_hdr = rhs._flat_hdr;
        return *this;
    }

    //! Assignment operator (=) from a uint64_t
    inline const chdr_header& operator=(const uint64_t& rhs)
    {
        _flat_hdr = rhs;
        return *this;
    }

    //! Return a string representation of this object
    inline const std::string to_string() const
    {
        // The static_casts are because vc and num_mdata are uint8_t -> unsigned char
        // For some reason, despite the %u meaning unsigned int, boost still formats them
        // as chars
        return str(boost::format("chdr_header{vc:%u, eob:%c, eov:%c, pkt_type:%u, "
                                 "num_mdata:%u, seq_num:%u, length:%u, dst_epid:%u}\n")
                   % static_cast<uint16_t>(get_vc()) % (get_eob() ? 'Y' : 'N')
                   % (get_eov() ? 'Y' : 'N') % get_pkt_type()
                   % static_cast<uint16_t>(get_num_mdata()) % get_seq_num() % get_length()
                   % get_dst_epid());
    }

private:
    // The flattened representation of the header stored in host order
    uint64_t _flat_hdr = 0;

    static constexpr size_t VC_WIDTH        = 6;
    static constexpr size_t EOB_WIDTH       = 1;
    static constexpr size_t EOV_WIDTH       = 1;
    static constexpr size_t PKT_TYPE_WIDTH  = 3;
    static constexpr size_t NUM_MDATA_WIDTH = 5;
    static constexpr size_t SEQ_NUM_WIDTH   = 16;
    static constexpr size_t LENGTH_WIDTH    = 16;
    static constexpr size_t DST_EPID_WIDTH  = 16;

    static constexpr size_t VC_OFFSET        = 58;
    static constexpr size_t EOB_OFFSET       = 57;
    static constexpr size_t EOV_OFFSET       = 56;
    static constexpr size_t PKT_TYPE_OFFSET  = 53;
    static constexpr size_t NUM_MDATA_OFFSET = 48;
    static constexpr size_t SEQ_NUM_OFFSET   = 32;
    static constexpr size_t LENGTH_OFFSET    = 16;
    static constexpr size_t DST_EPID_OFFSET  = 0;

    static inline uint64_t mask(const size_t width)
    {
        return ((uint64_t(1) << width) - 1);
    }

    template <typename field_t>
    static inline field_t get_field(
        const uint64_t flat_hdr, const size_t offset, const size_t width)
    {
        return static_cast<field_t>((flat_hdr >> offset) & mask(width));
    }

    template <typename field_t>
    static inline uint64_t set_field(const uint64_t old_val,
        const field_t field,
        const size_t offset,
        const size_t width)
    {
        return (old_val & ~(mask(width) << offset))
               | ((static_cast<uint64_t>(field) & mask(width)) << offset);
    }
};


//----------------------------------------------------
// CHDR Control Packet Payload
//----------------------------------------------------

enum ctrl_status_t {
    CMD_OKAY    = 0x0, //! Transaction successful
    CMD_CMDERR  = 0x1, //! Slave asserted a command error
    CMD_TSERR   = 0x2, //! Slave asserted a time stamp error
    CMD_WARNING = 0x3, //! Slave asserted non-critical error
};

enum ctrl_opcode_t {
    OP_SLEEP       = 0x0,
    OP_WRITE       = 0x1,
    OP_READ        = 0x2,
    OP_READ_WRITE  = 0x3,
    OP_BLOCK_WRITE = 0x4,
    OP_BLOCK_READ  = 0x5,
    OP_POLL        = 0x6,
    OP_USER1       = 0xA,
    OP_USER2       = 0xB,
    OP_USER3       = 0xC,
    OP_USER4       = 0xD,
    OP_USER5       = 0xE,
    OP_USER6       = 0xF,
};

class UHD_API ctrl_payload
{
public: // Members
    //! Destination port for transaction (10 bits)
    uint16_t dst_port = 0;
    //! Source port for transaction (10 bits)
    uint16_t src_port = 0;
    //! Sequence number (6 bits)
    uint8_t seq_num = 0;
    //! Has Time Flag (1 bit) and timestamp (64 bits)
    boost::optional<uint64_t> timestamp = boost::none;
    //! Is Acknowledgment Flag (1 bit)
    bool is_ack = false;
    //! Source endpoint ID of transaction (16 bits)
    uint16_t src_epid = 0;
    //! Address for transaction (20 bits)
    uint32_t address = 0;
    //! Data for transaction (vector of 32 bits)
    std::vector<uint32_t> data_vtr = {0};
    //! Byte-enable mask for transaction (4 bits)
    uint8_t byte_enable = 0xF;
    //! Operation code (4 bits)
    ctrl_opcode_t op_code = OP_SLEEP;
    //! Transaction status (4 bits)
    ctrl_status_t status = CMD_OKAY;

public: // Functions
    ctrl_payload()                        = default;
    ctrl_payload(const ctrl_payload& rhs) = default;
    ctrl_payload(ctrl_payload&& rhs)      = default;

    ctrl_payload& operator=(const ctrl_payload& rhs) = default;

    //! Populate the header for this type of packet
    void populate_header(chdr_header& header) const;

    //! Serialize the payload to a uint64_t buffer
    size_t serialize(uint64_t* buff,
        size_t max_size_bytes,
        const std::function<uint64_t(uint64_t)>& conv_byte_order) const;

    //! Serialize the payload to a uint64_t buffer (no conversion function)
    template <endianness_t endianness>
    size_t serialize(uint64_t* buff, size_t max_size_bytes) const
    {
        auto conv_byte_order = [](uint64_t x) -> uint64_t {
            return (endianness == uhd::ENDIANNESS_BIG) ? uhd::htonx<uint64_t>(x)
                                                       : uhd::htowx<uint64_t>(x);
        };
        return serialize(buff, max_size_bytes, conv_byte_order);
    }

    //! Deserialize the payload from a uint64_t buffer
    //! \param buff Buffer to deserialize the payload from
    //! \param buff_size Number of elements in the buffer
    //! \param conv_byte_order Byte order converter function (buffer to host endianness)
    void deserialize(const uint64_t* buff,
        size_t buff_size,
        const std::function<uint64_t(uint64_t)>& conv_byte_order);

    //! Deserialize the payload from a uint64_t buffer (no conversion function)
    //! \param buff Buffer to deserialize the payload from
    //! \param buff_size Number of elements in the buffer
    template <endianness_t endianness>
    void deserialize(const uint64_t* buff, size_t buff_size)
    {
        auto conv_byte_order = [](uint64_t x) -> uint64_t {
            return (endianness == uhd::ENDIANNESS_BIG) ? uhd::ntohx<uint64_t>(x)
                                                       : uhd::wtohx<uint64_t>(x);
        };
        deserialize(buff, buff_size, conv_byte_order);
    }

    //! Get the serialized size of this payload in 64 bit words
    size_t get_length() const;

    // Return whether or not we have a valid timestamp
    bool has_timestamp() const
    {
        return bool(timestamp);
    }

    //! Comparison operator (==)
    bool operator==(const ctrl_payload& rhs) const;

    //! Comparison operator (!=)
    inline bool operator!=(const ctrl_payload& rhs) const
    {
        return !(*this == rhs);
    }

    //! Return a string representation of this object
    const std::string to_string() const;

private:
    static constexpr size_t DST_PORT_WIDTH    = 10;
    static constexpr size_t SRC_PORT_WIDTH    = 10;
    static constexpr size_t NUM_DATA_WIDTH    = 4;
    static constexpr size_t SEQ_NUM_WIDTH     = 6;
    static constexpr size_t HAS_TIME_WIDTH    = 1;
    static constexpr size_t IS_ACK_WIDTH      = 1;
    static constexpr size_t SRC_EPID_WIDTH    = 16;
    static constexpr size_t ADDRESS_WIDTH     = 20;
    static constexpr size_t BYTE_ENABLE_WIDTH = 4;
    static constexpr size_t OPCODE_WIDTH      = 4;
    static constexpr size_t STATUS_WIDTH      = 2;

    // Offsets assume 64-bit alignment
    static constexpr size_t DST_PORT_OFFSET    = 0;
    static constexpr size_t SRC_PORT_OFFSET    = 10;
    static constexpr size_t NUM_DATA_OFFSET    = 20;
    static constexpr size_t SEQ_NUM_OFFSET     = 24;
    static constexpr size_t HAS_TIME_OFFSET    = 30;
    static constexpr size_t IS_ACK_OFFSET      = 31;
    static constexpr size_t SRC_EPID_OFFSET    = 32;
    static constexpr size_t ADDRESS_OFFSET     = 0;
    static constexpr size_t BYTE_ENABLE_OFFSET = 20;
    static constexpr size_t OPCODE_OFFSET      = 24;
    static constexpr size_t STATUS_OFFSET      = 30;
    static constexpr size_t LO_DATA_OFFSET     = 0;
    static constexpr size_t HI_DATA_OFFSET     = 32;
};

//----------------------------------------------------
// CHDR Stream Status Packet Payload
//----------------------------------------------------

enum strs_status_t {
    STRS_OKAY    = 0x0, //! No error
    STRS_CMDERR  = 0x1, //! A stream command signalled an error
    STRS_SEQERR  = 0x2, //! Packet out of sequence (sequence error)
    STRS_DATAERR = 0x3, //! Data integrity check failed
    STRS_RTERR   = 0x4, //! Unexpected destination (routing error)
};

class UHD_API strs_payload
{
public: // Members
    //! The source EPID for the stream (16 bits)
    uint16_t src_epid = 0;
    //! The status of the stream (4 bits)
    strs_status_t status = STRS_OKAY;
    //! Buffer capacity in bytes (40 bits)
    uint64_t capacity_bytes = 0;
    //! Buffer capacity in packets (24 bits)
    uint32_t capacity_pkts = 0;
    //! Transfer count in bytes (64 bits)
    uint64_t xfer_count_bytes = 0;
    //! Transfer count in packets (40 bits)
    uint64_t xfer_count_pkts = 0;
    //! Buffer info (16 bits)
    uint16_t buff_info = 0;
    //! Extended status info (48 bits)
    uint64_t status_info = 0;

public: // Functions
    strs_payload()                        = default;
    strs_payload(const strs_payload& rhs) = default;
    strs_payload(strs_payload&& rhs)      = default;

    strs_payload& operator=(const strs_payload& rhs) = default;

    //! Populate the header for this type of packet
    void populate_header(chdr_header& header) const;

    //! Serialize the payload to a uint64_t buffer
    size_t serialize(uint64_t* buff,
        size_t max_size_bytes,
        const std::function<uint64_t(uint64_t)>& conv_byte_order) const;

    //! Serialize the payload to a uint64_t buffer (no conversion function)
    template <endianness_t endianness>
    size_t serialize(uint64_t* buff, size_t max_size_bytes) const
    {
        auto conv_byte_order = [](uint64_t x) -> uint64_t {
            return (endianness == uhd::ENDIANNESS_BIG) ? uhd::htonx<uint64_t>(x)
                                                       : uhd::htowx<uint64_t>(x);
        };
        return serialize(buff, max_size_bytes, conv_byte_order);
    }

    //! Deserialize the payload from a uint64_t buffer
    //! \param buff Buffer to deserialize the payload from
    //! \param buff_size Number of elements in the buffer
    //! \param conv_byte_order Byte order converter function (buffer to host endianness)
    void deserialize(const uint64_t* buff,
        size_t buff_size,
        const std::function<uint64_t(uint64_t)>& conv_byte_order);

    //! Deserialize the payload from a uint64_t buffer (no conversion function)
    //! \param buff Buffer to deserialize the payload from
    //! \param buff_size Number of elements in the buffer
    template <endianness_t endianness>
    void deserialize(const uint64_t* buff, size_t buff_size)
    {
        auto conv_byte_order = [](uint64_t x) -> uint64_t {
            return (endianness == uhd::ENDIANNESS_BIG) ? uhd::ntohx<uint64_t>(x)
                                                       : uhd::wtohx<uint64_t>(x);
        };
        deserialize(buff, buff_size, conv_byte_order);
    }

    //! Get the serialized size of this payload in 64 bit words
    size_t get_length() const;

    //! Comparison operator (==)
    bool operator==(const strs_payload& rhs) const;

    //! Comparison operator (!=)
    inline bool operator!=(const strs_payload& rhs) const
    {
        return !(*this == rhs);
    }

    //! Return a string representation of this object
    const std::string to_string() const;

private:
    static constexpr size_t SRC_EPID_WIDTH        = 16;
    static constexpr size_t STATUS_WIDTH          = 4;
    static constexpr size_t CAPACITY_BYTES_WIDTH  = 40;
    static constexpr size_t CAPACITY_PKTS_WIDTH   = 24;
    static constexpr size_t XFER_COUNT_PKTS_WIDTH = 40;
    static constexpr size_t BUFF_INFO_WIDTH       = 16;
    static constexpr size_t STATUS_INFO_WIDTH     = 48;

    // Offsets assume 64-bit alignment
    static constexpr size_t SRC_EPID_OFFSET        = 0;
    static constexpr size_t STATUS_OFFSET          = 16;
    static constexpr size_t CAPACITY_BYTES_OFFSET  = 24;
    static constexpr size_t CAPACITY_PKTS_OFFSET   = 0;
    static constexpr size_t XFER_COUNT_PKTS_OFFSET = 24;
    static constexpr size_t BUFF_INFO_OFFSET       = 0;
    static constexpr size_t STATUS_INFO_OFFSET     = 16;
};

//----------------------------------------------------
// CHDR Stream Command Packet Payload
//----------------------------------------------------

enum strc_op_code_t {
    STRC_INIT   = 0x0, //! Initialize stream
    STRC_PING   = 0x1, //! Trigger a stream status response
    STRC_RESYNC = 0x2, //! Re-synchronize flow control
};

class UHD_API strc_payload
{
public: // Members
    //! The source EPID for the stream (16 bits)
    uint16_t src_epid = 0;
    //! Operation code for the command (4 bits)
    strc_op_code_t op_code = STRC_INIT;
    //! Data associated with the operation (4 bits)
    uint8_t op_data = 0;
    //! Number of packets to use for operation (40 bits)
    uint64_t num_pkts = 0;
    //! Number of bytes to use for operation (64 bits)
    uint64_t num_bytes = 0;
    //! Worst-case size of a strc packet (including header)
    static constexpr size_t MAX_PACKET_SIZE = 128;

public: // Functions
    strc_payload()                        = default;
    strc_payload(const strc_payload& rhs) = default;
    strc_payload(strc_payload&& rhs)      = default;

    strc_payload& operator=(const strc_payload& rhs) = default;

    //! Populate the header for this type of packet
    void populate_header(chdr_header& header) const;

    //! Serialize the payload to a uint64_t buffer
    size_t serialize(uint64_t* buff,
        size_t max_size_bytes,
        const std::function<uint64_t(uint64_t)>& conv_byte_order) const;

    //! Serialize the payload to a uint64_t buffer (no conversion function)
    template <endianness_t endianness>
    size_t serialize(uint64_t* buff, size_t max_size_bytes) const
    {
        auto conv_byte_order = [](uint64_t x) -> uint64_t {
            return (endianness == uhd::ENDIANNESS_BIG) ? uhd::htonx<uint64_t>(x)
                                                       : uhd::htowx<uint64_t>(x);
        };
        return serialize(buff, max_size_bytes, conv_byte_order);
    }

    //! Deserialize the payload from a uint64_t buffer
    //! \param buff Buffer to deserialize the payload from
    //! \param buff_size Number of elements in the buffer
    //! \param conv_byte_order Byte order converter function (buffer to host endianness)
    void deserialize(const uint64_t* buff,
        size_t buff_size,
        const std::function<uint64_t(uint64_t)>& conv_byte_order);

    //! Deserialize the payload from a uint64_t buffer (no conversion function)
    //! \param buff Buffer to deserialize the payload from
    //! \param buff_size Number of elements in the buffer
    template <endianness_t endianness>
    void deserialize(const uint64_t* buff, size_t buff_size)
    {
        auto conv_byte_order = [](uint64_t x) -> uint64_t {
            return (endianness == uhd::ENDIANNESS_BIG) ? uhd::ntohx<uint64_t>(x)
                                                       : uhd::wtohx<uint64_t>(x);
        };
        deserialize(buff, buff_size, conv_byte_order);
    }

    //! Get the serialized size of this payload in 64 bit words
    size_t get_length() const;

    //! Comparison operator (==)
    bool operator==(const strc_payload& rhs) const;

    //! Comparison operator (!=)
    inline bool operator!=(const strc_payload& rhs) const
    {
        return !(*this == rhs);
    }

    //! Return a string representation of this object
    const std::string to_string() const;

private:
    static constexpr size_t SRC_EPID_WIDTH = 16;
    static constexpr size_t OP_CODE_WIDTH  = 4;
    static constexpr size_t OP_DATA_WIDTH  = 4;
    static constexpr size_t NUM_PKTS_WIDTH = 40;

    // Offsets assume 64-bit alignment
    static constexpr size_t SRC_EPID_OFFSET = 0;
    static constexpr size_t OP_CODE_OFFSET  = 16;
    static constexpr size_t OP_DATA_OFFSET  = 20;
    static constexpr size_t NUM_PKTS_OFFSET = 24;
};

//----------------------------------------------------
// CHDR Management Packet Payload
//----------------------------------------------------

//! A class that represents a single management operation
//  An operation consists of an operation code and some
//  payload associated with that operation.
class UHD_API mgmt_op_t
{
public:
    // Operation code
    // Note that a management packet has 8 bits available for op codes. The
    // values for these enums are used to construct the packets, so these values
    // must match the values in rfnoc_chdr_internal_utils.vh.
    enum op_code_t {
        //! Do nothing
        MGMT_OP_NOP = 0,
        //! Advertise this operation to the outside logic
        MGMT_OP_ADVERTISE = 1,
        //! Select the next destination for routing
        MGMT_OP_SEL_DEST = 2,
        //! Return the management packet back to its source
        MGMT_OP_RETURN = 3,
        //! Request information about the current node
        MGMT_OP_INFO_REQ = 4,
        //! A response to an information request
        MGMT_OP_INFO_RESP = 5,
        //! Perform a configuration write on the node
        MGMT_OP_CFG_WR_REQ = 6,
        //! Perform a configuration read on the node
        MGMT_OP_CFG_RD_REQ = 7,
        //! A response to a configuration read
        MGMT_OP_CFG_RD_RESP = 8
    };

    //! The payload for an operation is 48 bits wide.
    using payload_t = uint64_t;

    //! An interpretation class for the payload for MGMT_OP_SEL_DEST
    struct sel_dest_payload
    {
        const uint16_t dest;

        sel_dest_payload(uint16_t dest_) : dest(dest_) {}
        sel_dest_payload(payload_t payload_) : dest(static_cast<uint16_t>(payload_)) {}
        operator payload_t() const
        {
            return static_cast<payload_t>(dest);
        }
    };

    //! An interpretation class for the payload for MGMT_OP_CFG_WR_REQ,
    //! MGMT_OP_CFG_RD_REQ and MGMT_OP_CFG_RD_RESP
    struct cfg_payload
    {
        const uint16_t addr;
        const uint32_t data;

        cfg_payload(uint16_t addr_, uint32_t data_ = 0) : addr(addr_), data(data_) {}
        cfg_payload(payload_t payload_)
            : addr(static_cast<uint16_t>(payload_ >> 0))
            , data(static_cast<uint32_t>(payload_ >> 16))
        {
        }
        operator payload_t() const
        {
            return ((static_cast<payload_t>(data) << 16) | static_cast<payload_t>(addr));
        }
    };

    //! An interpretation class for the payload for MGMT_OP_INFO_RESP
    struct node_info_payload
    {
        const uint16_t device_id;
        const uint8_t node_type;
        const uint16_t node_inst;
        const uint32_t ext_info;

        node_info_payload(uint16_t device_id_,
            uint8_t node_type_,
            uint16_t node_inst_,
            uint32_t ext_info_)
            : device_id(device_id_)
            , node_type(node_type_)
            , node_inst(node_inst_)
            , ext_info(ext_info_)
        {
        }
        node_info_payload(payload_t payload_)
            : device_id(static_cast<uint16_t>(payload_ >> 0))
            , node_type(static_cast<uint8_t>((payload_ >> 16) & 0xF))
            , node_inst(static_cast<uint16_t>((payload_ >> 20) & 0x3FF))
            , ext_info(static_cast<uint32_t>((payload_ >> 30) & 0x3FFFF))
        {
        }
        operator payload_t() const
        {
            return ((static_cast<payload_t>(device_id) << 0)
                    | (static_cast<payload_t>(node_type & 0xF) << 16)
                    | (static_cast<payload_t>(node_inst & 0x3FF) << 20)
                    | (static_cast<payload_t>(ext_info & 0x3FFFF) << 30));
        }
    };

    mgmt_op_t(const op_code_t op_code, const payload_t op_payload = 0,
        const uint8_t ops_pending = 0)
        : _op_code(op_code), _op_payload(op_payload), _ops_pending(ops_pending)
    {
    }
    mgmt_op_t(const mgmt_op_t& rhs) = default;

    //! Get the ops pending for this transaction
    //  Note that ops_pending is not used by UHD, since it can infer this value
    //  from the ops vector in mgmt_hop_t. It is needed only by the CHDR
    //  dissector.
    inline uint8_t get_ops_pending() const
    {
        return _ops_pending;
    }

    //! Get the op-code for this transaction
    inline op_code_t get_op_code() const
    {
        return _op_code;
    }

    //! Get the payload for this transaction
    inline uint64_t get_op_payload() const
    {
        return _op_payload;
    }

    //! Comparison operator (==)
    inline bool operator==(const mgmt_op_t& rhs) const
    {
        return (_op_code == rhs._op_code) && (_op_payload == rhs._op_payload);
    }

    //! Return a string representation of this object
    const std::string to_string() const;

private:
    op_code_t _op_code;
    payload_t _op_payload;
    uint8_t _ops_pending;
};

//! A class that represents a single management hop
//  A hop is a collection for management transactions for
//  a single node.
class UHD_API mgmt_hop_t
{
public:
    mgmt_hop_t()                      = default;
    mgmt_hop_t(const mgmt_hop_t& rhs) = default;

    //! Add a management operation to this hop.
    //  Operations are added to the hop in FIFO order and executed in FIFO order.
    inline void add_op(const mgmt_op_t& op)
    {
        _ops.push_back(op);
    }

    //! Get the number of management operations in this hop
    inline size_t get_num_ops() const
    {
        return _ops.size();
    }

    //! Get the n'th operation in the hop
    inline const mgmt_op_t& get_op(size_t i) const
    {
        return _ops.at(i);
    }

    //! Serialize the payload to a uint64_t buffer
    //  The RFNoC Specification section 2.2.6 specifies that for chdr widths
    //  greater than 64, all MSBs are 0, so we pad out the hop based on the width
    size_t serialize(std::vector<uint64_t>& target,
        const std::function<uint64_t(uint64_t)>& conv_byte_order,
        const size_t padding_size) const;

    //! Deserialize the payload from a uint64_t buffer
    //  The RFNoC Specification section 2.2.6 specifies that for chdr widths
    //  greater than 64, all MSBs are 0, so we remove padding based on the width
    void deserialize(std::list<uint64_t>& src,
        const std::function<uint64_t(uint64_t)>& conv_byte_order,
        const size_t padding_size);

    //! Comparison operator (==)
    inline bool operator==(const mgmt_hop_t& rhs) const
    {
        return _ops == rhs._ops;
    }

    //! Return a string representation of this object
    const std::string to_string() const;

private:
    std::vector<mgmt_op_t> _ops;
};

//! A class that represents a complete multi-hop management transaction
//  A transaction is a collection of hops, where each hop is a collection
//  of management transactions.
class UHD_API mgmt_payload
{
public:
    mgmt_payload()                        = default;
    mgmt_payload(const mgmt_payload& rhs) = default;
    mgmt_payload(mgmt_payload&& rhs)      = default;

    mgmt_payload& operator=(const mgmt_payload& rhs) = default;

    inline void set_header(sep_id_t src_epid, uint16_t protover, chdr_w_t chdr_w)
    {
        set_src_epid(src_epid);
        set_chdr_w(chdr_w);
        set_proto_ver(protover);
    }

    //! Add a management hop to this transaction
    //  Hops are added to the hop in FIFO order and executed in FIFO order.
    inline void add_hop(const mgmt_hop_t& hop)
    {
        _hops.push_back(hop);
    }

    //! Get the number of management hops in this hop
    inline size_t get_num_hops() const
    {
        return _hops.size();
    }

    //! Get the n'th hop in the transaction
    inline const mgmt_hop_t& get_hop(size_t i) const
    {
        return _hops.at(i);
    }

    //! Pop the first hop of the transaction and return it
    inline mgmt_hop_t pop_hop()
    {
        auto hop = _hops.front();
        _hops.pop_front();
        return hop;
    }

    inline size_t get_size_bytes() const
    {
        size_t num_lines = 1; /* header */
        for (const auto& hop : _hops) {
            num_lines += hop.get_num_ops();
        }
        return num_lines * (chdr_w_to_bits(_chdr_w) / 8);
    }

    //! Populate the header for this type of packet
    void populate_header(chdr_header& header) const;

    //! Serialize the payload to a uint64_t buffer
    size_t serialize(uint64_t* buff,
        size_t max_size_bytes,
        const std::function<uint64_t(uint64_t)>& conv_byte_order) const;

    //! Serialize the payload to a uint64_t buffer (no conversion function)
    template <endianness_t endianness>
    size_t serialize(uint64_t* buff, size_t max_size_bytes) const
    {
        auto conv_byte_order = [](uint64_t x) -> uint64_t {
            return (endianness == uhd::ENDIANNESS_BIG) ? uhd::htonx<uint64_t>(x)
                                                       : uhd::htowx<uint64_t>(x);
        };
        return serialize(buff, max_size_bytes, conv_byte_order);
    }

    //! Deserialize the payload from a uint64_t buffer
    //! \param buff Buffer to deserialize the payload from
    //! \param buff_size Number of elements in the buffer
    //! \param conv_byte_order Byte order converter function (buffer to host endianness)
    void deserialize(const uint64_t* buff,
        size_t buff_size,
        const std::function<uint64_t(uint64_t)>& conv_byte_order);

    //! Deserialize the payload from a uint64_t buffer (no conversion function)
    //! \param buff Buffer to deserialize the payload from
    //! \param buff_size Number of elements in the buffer
    template <endianness_t endianness>
    void deserialize(const uint64_t* buff, size_t buff_size)
    {
        auto conv_byte_order = [](uint64_t x) -> uint64_t {
            return (endianness == uhd::ENDIANNESS_BIG) ? uhd::ntohx<uint64_t>(x)
                                                       : uhd::wtohx<uint64_t>(x);
        };
        deserialize(buff, buff_size, conv_byte_order);
    }

    //! Get the serialized size of this payload in 64 bit words
    size_t get_length() const;

    //! Return a string representation of this object
    const std::string to_string() const;

    //! Return a string representaiton of the hops contained by this object
    const std::string hops_to_string() const;

    //! Return the source EPID for this transaction
    inline sep_id_t get_src_epid() const
    {
        return _src_epid;
    }

    //! Set the source EPID for this transaction
    inline void set_src_epid(sep_id_t src_epid)
    {
        _src_epid = src_epid;
    }

    //! Comparison operator (==)
    bool operator==(const mgmt_payload& rhs) const;

    //! Return the CHDR_W for this transaction
    inline chdr_w_t get_chdr_w() const
    {
        return _chdr_w;
    }

    //! Set the CHDR_W for this transaction
    inline void set_chdr_w(chdr_w_t chdr_w)
    {
        _chdr_w       = chdr_w;
        _padding_size = (chdr_w_to_bits(_chdr_w) / 64) - 1;
    }

    //! Return the protocol version for this transaction
    inline uint16_t get_proto_ver() const
    {
        return _protover;
    }

    //! Set the protocol version for this transaction
    inline void set_proto_ver(uint16_t proto_ver)
    {
        _protover = proto_ver;
    }

private:
    sep_id_t _src_epid   = 0;
    uint16_t _protover   = 0;
    chdr_w_t _chdr_w     = CHDR_W_64;
    size_t _padding_size = 0;
    std::deque<mgmt_hop_t> _hops;
};

//! Conversion from payload_t to pkt_type
template <typename payload_t>
constexpr packet_type_t payload_to_packet_type();

template <>
constexpr packet_type_t payload_to_packet_type<ctrl_payload>()
{
    return PKT_TYPE_CTRL;
}

template <>
constexpr packet_type_t payload_to_packet_type<mgmt_payload>()
{
    return PKT_TYPE_MGMT;
}

template <>
constexpr packet_type_t payload_to_packet_type<strc_payload>()
{
    return PKT_TYPE_STRC;
}

template <>
constexpr packet_type_t payload_to_packet_type<strs_payload>()
{
    return PKT_TYPE_STRS;
}

}}} // namespace uhd::rfnoc::chdr
