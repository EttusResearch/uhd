//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/rfnoc/chdr_types.hpp>
#include <uhd/types/endianness.hpp>
#include <boost/format.hpp>
#include <cassert>
#include <iomanip>
#include <sstream>

using namespace uhd;
using namespace uhd::rfnoc::chdr;

//----------------------------------------------------
// Utility Functions
//----------------------------------------------------

static inline constexpr uint64_t mask_u64(size_t width)
{
    return ((uint64_t(1) << width) - 1);
}

template <typename field_t>
static inline constexpr field_t get_field_u64(
    uint64_t flat_hdr, size_t offset, size_t width)
{
    return static_cast<field_t>((flat_hdr >> offset) & mask_u64(width));
}

static inline constexpr uint32_t mask_u32(size_t width)
{
    return ((uint32_t(1) << width) - 1);
}

template <typename field_t>
static inline constexpr field_t get_field_u32(
    uint32_t flat_hdr, size_t offset, size_t width)
{
    return static_cast<field_t>((flat_hdr >> offset) & mask_u32(width));
}

//----------------------------------------------------
// CHDR Header
//----------------------------------------------------
const std::string chdr_header::to_string() const
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

//----------------------------------------------------
// CHDR Control Payload
//----------------------------------------------------

void ctrl_payload::populate_header(chdr_header& header) const
{
    header.set_pkt_type(PKT_TYPE_CTRL);
    header.set_eob(false);
    header.set_eov(false);
    header.set_num_mdata(0);
}

size_t ctrl_payload::serialize(uint32_t* buff,
    size_t max_size_bytes,
    const std::function<uint32_t(uint32_t)>& conv_byte_order) const
{
    // NumData is the number of 32-bit data words actually present in the
    // packet and must match the data vector. ReqSize is the requested word
    // count (read requests only, echoed in the response) and is independent of
    // the data words present.
    UHD_ASSERT_THROW(num_data == data_vtr.size() && num_data <= MAX_DATA_WORDS);
    UHD_ASSERT_THROW(req_size <= MAX_DATA_WORDS);
    UHD_ASSERT_THROW(get_length() <= max_size_bytes);
    size_t ptr = 0;

    // Populate control header word 0: src_epid, is_ack, has_time, num_data,
    // dst_port
    buff[ptr++] = conv_byte_order(
        ((static_cast<uint32_t>(dst_port) & mask_u32(DST_PORT_WIDTH)) << DST_PORT_OFFSET)
        | ((static_cast<uint32_t>(num_data) & mask_u32(NUM_DATA_WIDTH))
            << NUM_DATA_OFFSET)
        | ((static_cast<uint32_t>(bool(timestamp) ? 1 : 0) & mask_u32(HAS_TIME_WIDTH))
            << HAS_TIME_OFFSET)
        | ((static_cast<uint32_t>(is_ack) & mask_u32(IS_ACK_WIDTH)) << IS_ACK_OFFSET)
        | ((static_cast<uint32_t>(src_epid) & mask_u32(SRC_EPID_WIDTH))
            << SRC_EPID_OFFSET));

    // Populate control header word 1: req_size, seq_num, src_port
    buff[ptr++] = conv_byte_order(
        ((static_cast<uint32_t>(src_port) & mask_u32(SRC_PORT_WIDTH)) << SRC_PORT_OFFSET)
        | ((static_cast<uint32_t>(seq_num) & mask_u32(SEQ_NUM_WIDTH)) << SEQ_NUM_OFFSET)
        | ((static_cast<uint32_t>(req_size) & mask_u32(REQ_SIZE_WIDTH))
            << REQ_SIZE_OFFSET));

    // Populate optional timestamp (low word first, then high word)
    if (bool(timestamp)) {
        buff[ptr++] = conv_byte_order(static_cast<uint32_t>(*timestamp & 0xFFFFFFFF));
        buff[ptr++] =
            conv_byte_order(static_cast<uint32_t>((*timestamp >> 32) & 0xFFFFFFFF));
    }

    // Populate op-word: address, byte_enable, op_code, status
    buff[ptr++] = conv_byte_order(
        ((static_cast<uint32_t>(address) & mask_u32(ADDRESS_WIDTH)) << ADDRESS_OFFSET)
        | ((static_cast<uint32_t>(byte_enable) & mask_u32(BYTE_ENABLE_WIDTH))
            << BYTE_ENABLE_OFFSET)
        | ((static_cast<uint32_t>(op_code) & mask_u32(OPCODE_WIDTH)) << OPCODE_OFFSET)
        | ((static_cast<uint32_t>(status) & mask_u32(STATUS_WIDTH)) << STATUS_OFFSET));

    // Append any data words present in the packet. NumData (and thus
    // data_vtr) is empty for read requests and write/sleep responses.
    for (size_t word_idx = 0; word_idx < data_vtr.size(); word_idx++) {
        buff[ptr++] = conv_byte_order(data_vtr[word_idx]);
    }

    UHD_ASSERT_THROW(ptr * sizeof(uint32_t) == get_length());
    return ptr * sizeof(uint32_t);
}

void ctrl_payload::deserialize(const uint32_t* buff,
    size_t buff_size,
    const std::function<uint32_t(uint32_t)>& conv_byte_order)
{
    size_t ptr = 0;

    // Minimum: ctrl header (2 words) + op-word (1 word)
    UHD_ASSERT_THROW(buff_size >= 3);

    // Read control header word 0: src_epid, is_ack, has_time, num_data,
    // dst_port
    const uint32_t hdr_lo = conv_byte_order(buff[ptr++]);
    const size_t num_data_from_hdr =
        get_field_u32<size_t>(hdr_lo, NUM_DATA_OFFSET, NUM_DATA_WIDTH);
    UHD_ASSERT_THROW(num_data_from_hdr <= MAX_DATA_WORDS);
    num_data = num_data_from_hdr;
    dst_port = get_field_u32<uint16_t>(hdr_lo, DST_PORT_OFFSET, DST_PORT_WIDTH);
    src_epid = get_field_u32<uint16_t>(hdr_lo, SRC_EPID_OFFSET, SRC_EPID_WIDTH);
    const bool has_time = get_field_u32<bool>(hdr_lo, HAS_TIME_OFFSET, HAS_TIME_WIDTH);
    is_ack              = get_field_u32<bool>(hdr_lo, IS_ACK_OFFSET, IS_ACK_WIDTH);

    // Read control header word 1: req_size, seq_num, src_port
    const uint32_t hdr_hi = conv_byte_order(buff[ptr++]);
    src_port = get_field_u32<uint16_t>(hdr_hi, SRC_PORT_OFFSET, SRC_PORT_WIDTH);
    seq_num  = get_field_u32<uint8_t>(hdr_hi, SEQ_NUM_OFFSET, SEQ_NUM_WIDTH);
    req_size = get_field_u32<size_t>(hdr_hi, REQ_SIZE_OFFSET, REQ_SIZE_WIDTH);

    // Read optional timestamp (low word first, then high word)
    if (has_time) {
        UHD_ASSERT_THROW(buff_size >= ptr + 2);
        const uint32_t ts_lo = conv_byte_order(buff[ptr++]);
        const uint32_t ts_hi = conv_byte_order(buff[ptr++]);
        timestamp = (static_cast<uint64_t>(ts_hi) << 32) | static_cast<uint64_t>(ts_lo);
    } else {
        timestamp = {};
    }

    // Read op-word: address, byte_enable, op_code, status
    UHD_ASSERT_THROW(buff_size >= ptr + 1);
    const uint32_t op_lo = conv_byte_order(buff[ptr++]);
    address              = get_field_u32<uint32_t>(op_lo, ADDRESS_OFFSET, ADDRESS_WIDTH);
    byte_enable = get_field_u32<uint8_t>(op_lo, BYTE_ENABLE_OFFSET, BYTE_ENABLE_WIDTH);
    op_code     = get_field_u32<ctrl_opcode_t>(op_lo, OPCODE_OFFSET, OPCODE_WIDTH);
    status      = get_field_u32<ctrl_status_t>(op_lo, STATUS_OFFSET, STATUS_WIDTH);

    // NumData is the number of data words actually present in the packet. Read
    // the data words that follow the op-word, bounded by the available buffer
    // to tolerate trailing padding (e.g. on wide CHDR buses). NumData is 0 for
    // read requests and write/sleep responses, so no data words are read.
    const size_t num_data_present = buff_size - ptr;
    const size_t actual_words     = std::min(num_data_present, num_data_from_hdr);
    data_vtr.resize(actual_words);
    for (size_t word_idx = 0; word_idx < actual_words; word_idx++) {
        data_vtr[word_idx] = conv_byte_order(buff[ptr++]);
    }

    UHD_ASSERT_THROW(ptr <= buff_size);
}

size_t ctrl_payload::get_length() const
{
    // Control packets are always a multiple of 32 bits (4 bytes). The control
    // header is 64 bits (2 words), timestamp is 64 bits (2 words) if present,
    // the op-word is 32 bits (1 word), and data words are 32 bits each. Read
    // requests and write/sleep responses carry no data words on the wire
    // (data_vtr is empty).
    size_t num_32bit_words = 2; // ctrl header = 64 bits = two 32-bit words
    if (this->has_timestamp()) {
        num_32bit_words += 2; // timestamp = 64 bits
    }
    num_32bit_words += 1 + this->data_vtr.size(); // op-word + data
    return num_32bit_words * sizeof(uint32_t);
}

bool ctrl_payload::operator==(const ctrl_payload& rhs) const
{
    return (dst_port == rhs.dst_port) && (src_port == rhs.src_port)
           && (seq_num == rhs.seq_num) && (timestamp == rhs.timestamp)
           && (is_ack == rhs.is_ack) && (src_epid == rhs.src_epid)
           && (address == rhs.address) && (data_vtr == rhs.data_vtr)
           && (byte_enable == rhs.byte_enable) && (op_code == rhs.op_code)
           && (status == rhs.status) && (num_data == rhs.num_data)
           && (req_size == rhs.req_size);
}

std::string ctrl_payload::to_string() const
{
    return str(boost::format(
                   "ctrl_payload{dst_port:%d, src_port:%d, seq_num:%d, timestamp:%s, "
                   "is_ack:%s, src_epid:%d, address:0x%05x, byte_enable:0x%x, "
                   "op_code:%d, status:%d, num_data:%d, req_size:%d")
               % dst_port % src_port % int(seq_num)
               % (bool(timestamp) ? str(boost::format("0x%016x") % *timestamp)
                                  : std::string("<not present>"))
               % (is_ack ? "true" : "false") % src_epid % address % int(byte_enable)
               % op_code % status % int(num_data) % int(req_size))
           +
           [&]() {
               std::ostringstream oss;
               for (size_t i = 0; i < data_vtr.size(); i++) {
                   oss << " data[" << i << "]:0x" << std::hex << std::setw(8)
                       << std::setfill('0') << data_vtr[i];
               }
               return oss.str();
           }()
           + "}\n";
}

//----------------------------------------------------
// CHDR Stream Status Payload
//----------------------------------------------------

void strs_payload::populate_header(chdr_header& header) const
{
    header.set_pkt_type(PKT_TYPE_STRS);
    header.set_eob(false);
    header.set_eov(false);
    header.set_num_mdata(0);
}

size_t strs_payload::serialize(uint64_t* buff,
    size_t max_size_bytes,
    const std::function<uint64_t(uint64_t)>& conv_byte_order) const
{
    UHD_ASSERT_THROW(max_size_bytes >= (4 * sizeof(uint64_t)));

    // Populate first word
    buff[0] = conv_byte_order(
        ((static_cast<uint64_t>(src_epid) & mask_u64(SRC_EPID_WIDTH)) << SRC_EPID_OFFSET)
        | ((static_cast<uint64_t>(status) & mask_u64(STATUS_WIDTH)) << STATUS_OFFSET)
        | ((static_cast<uint64_t>(capacity_bytes) & mask_u64(CAPACITY_BYTES_WIDTH))
            << CAPACITY_BYTES_OFFSET));

    // Populate second word
    buff[1] = conv_byte_order(
        ((static_cast<uint64_t>(capacity_pkts) & mask_u64(CAPACITY_PKTS_WIDTH))
            << CAPACITY_PKTS_OFFSET)
        | ((static_cast<uint64_t>(xfer_count_pkts) & mask_u64(XFER_COUNT_PKTS_WIDTH))
            << XFER_COUNT_PKTS_OFFSET));

    // Populate third word
    buff[2] = conv_byte_order(xfer_count_bytes);

    // Populate fourth word
    buff[3] = conv_byte_order(
        ((static_cast<uint64_t>(buff_info) & mask_u64(BUFF_INFO_WIDTH))
            << BUFF_INFO_OFFSET)
        | ((static_cast<uint64_t>(status_info.info) & mask_u64(STATUS_INFO_WIDTH))
            << STATUS_INFO_OFFSET));

    // Return bytes written
    return (4 * sizeof(uint64_t));
}

void strs_payload::deserialize(const uint64_t* buff,
    size_t buff_size,
    const std::function<uint64_t(uint64_t)>& conv_byte_order)
{
    UHD_ASSERT_THROW(buff_size >= 4);

    // Read first word
    uint64_t word0 = conv_byte_order(buff[0]);
    src_epid       = get_field_u64<uint16_t>(word0, SRC_EPID_OFFSET, SRC_EPID_WIDTH);
    status         = get_field_u64<strs_status_t>(word0, STATUS_OFFSET, STATUS_WIDTH);
    capacity_bytes =
        get_field_u64<uint64_t>(word0, CAPACITY_BYTES_OFFSET, CAPACITY_BYTES_WIDTH);

    // Read second word
    uint64_t word1 = conv_byte_order(buff[1]);
    capacity_pkts =
        get_field_u64<uint32_t>(word1, CAPACITY_PKTS_OFFSET, CAPACITY_PKTS_WIDTH);
    xfer_count_pkts =
        get_field_u64<uint64_t>(word1, XFER_COUNT_PKTS_OFFSET, XFER_COUNT_PKTS_WIDTH);

    // Read third word
    xfer_count_bytes = conv_byte_order(buff[2]);

    // Read fourth word
    uint64_t word3 = conv_byte_order(buff[3]);
    buff_info      = get_field_u64<uint16_t>(word3, BUFF_INFO_OFFSET, BUFF_INFO_WIDTH);
    status_info.info =
        get_field_u64<uint64_t>(word3, STATUS_INFO_OFFSET, STATUS_INFO_WIDTH);
}

size_t strs_payload::get_length() const
{
    return 4 * sizeof(uint64_t);
}

bool strs_payload::operator==(const strs_payload& rhs) const
{
    return (src_epid == rhs.src_epid) && (status == rhs.status)
           && (capacity_bytes == rhs.capacity_bytes)
           && (capacity_pkts == rhs.capacity_pkts)
           && (xfer_count_pkts == rhs.xfer_count_pkts)
           && (xfer_count_bytes == rhs.xfer_count_bytes) && (buff_info == rhs.buff_info)
           && (status_info.info == rhs.status_info.info);
}

std::string strs_payload::to_string() const
{
    return str(boost::format("strs_payload{src_epid:%lu, status:%d, capacity_bytes:%lu, "
                             "capacity_pkts:%lu, "
                             "xfer_count_pkts:%lu, xfer_count_bytes:%lu, "
                             "buff_info:0x%x, status_info:0x%x}\n")
               % src_epid % int(status) % capacity_bytes % capacity_pkts % xfer_count_pkts
               % xfer_count_bytes % buff_info % status_info.info);
}

//----------------------------------------------------
// CHDR Stream Command Payload
//----------------------------------------------------

void strc_payload::populate_header(chdr_header& header) const
{
    header.set_pkt_type(PKT_TYPE_STRC);
    header.set_eob(false);
    header.set_eov(false);
    header.set_num_mdata(0);
}

size_t strc_payload::serialize(uint64_t* buff,
    size_t max_size_bytes,
    const std::function<uint64_t(uint64_t)>& conv_byte_order) const
{
    UHD_ASSERT_THROW(max_size_bytes >= (2 * sizeof(uint64_t)));

    // Populate first word
    buff[0] = conv_byte_order(
        ((static_cast<uint64_t>(src_epid) & mask_u64(SRC_EPID_WIDTH)) << SRC_EPID_OFFSET)
        | ((static_cast<uint64_t>(op_code) & mask_u64(OP_CODE_WIDTH)) << OP_CODE_OFFSET)
        | ((static_cast<uint64_t>(op_data) & mask_u64(OP_DATA_WIDTH)) << OP_DATA_OFFSET)
        | ((static_cast<uint64_t>(num_pkts) & mask_u64(NUM_PKTS_WIDTH))
            << NUM_PKTS_OFFSET));

    // Populate second word
    buff[1] = conv_byte_order(num_bytes);

    // Return bytes written
    return (2 * sizeof(uint64_t));
}

void strc_payload::deserialize(const uint64_t* buff,
    size_t buff_size,
    const std::function<uint64_t(uint64_t)>& conv_byte_order)
{
    UHD_ASSERT_THROW(buff_size >= 2);

    // Read first word
    uint64_t word0 = conv_byte_order(buff[0]);
    src_epid       = get_field_u64<uint16_t>(word0, SRC_EPID_OFFSET, SRC_EPID_WIDTH);
    op_code        = get_field_u64<strc_op_code_t>(word0, OP_CODE_OFFSET, OP_CODE_WIDTH);
    op_data        = get_field_u64<uint8_t>(word0, OP_DATA_OFFSET, OP_DATA_WIDTH);
    num_pkts       = get_field_u64<uint64_t>(word0, NUM_PKTS_OFFSET, NUM_PKTS_WIDTH);
    // Read second word
    num_bytes = conv_byte_order(buff[1]);
}

size_t strc_payload::get_length() const
{
    return 2 * sizeof(uint64_t);
}

bool strc_payload::operator==(const strc_payload& rhs) const
{
    return (src_epid == rhs.src_epid) && (op_code == rhs.op_code)
           && (op_data == rhs.op_data) && (num_pkts == rhs.num_pkts)
           && (num_bytes == rhs.num_bytes);
}

std::string strc_payload::to_string() const
{
    return str(boost::format("strc_payload{src_epid:%lu, op_code:%d, op_data:0x%x, "
                             "num_pkts:%lu, num_bytes:%lu}\n")
               % src_epid % int(op_code) % int(op_data) % num_pkts % num_bytes);
}

//----------------------------------------------------
// CHDR Management Payload
//----------------------------------------------------

std::string mgmt_op_t::to_string() const
{
    std::stringstream stream;
    switch (get_op_code()) {
        case mgmt_op_t::MGMT_OP_NOP:
            stream << "NOP";
            break;
        case mgmt_op_t::MGMT_OP_ADVERTISE:
            stream << "ADVERTISE";
            break;
        case mgmt_op_t::MGMT_OP_SEL_DEST:
            stream << "SEL_DEST";
            break;
        case mgmt_op_t::MGMT_OP_RETURN:
            stream << "RETURN";
            break;
        case mgmt_op_t::MGMT_OP_INFO_REQ:
            stream << "INFO_REQ";
            break;
        case mgmt_op_t::MGMT_OP_INFO_RESP:
            stream << "INFO_RESP";
            break;
        case mgmt_op_t::MGMT_OP_CFG_WR_REQ:
            stream << "CFG_WR_REQ";
            break;
        case mgmt_op_t::MGMT_OP_CFG_RD_REQ:
            stream << "CFG_RD_REQ";
            break;
        case mgmt_op_t::MGMT_OP_CFG_RD_RESP:
            stream << "CFG_RD_RESP";
            break;
        default:
            UHD_THROW_INVALID_CODE_PATH();
    }
    stream << ": ";
    switch (get_op_code()) {
        case mgmt_op_t::MGMT_OP_SEL_DEST: {
            mgmt_op_t::sel_dest_payload payload = static_cast<uint64_t>(get_op_payload());
            stream << "dest:" << payload.dest;
            break;
        }
        case mgmt_op_t::MGMT_OP_CFG_WR_REQ:
        case mgmt_op_t::MGMT_OP_CFG_RD_REQ:
        case mgmt_op_t::MGMT_OP_CFG_RD_RESP: {
            mgmt_op_t::cfg_payload payload = static_cast<uint64_t>(get_op_payload());
            stream << str(
                boost::format("addr:0x%08x, data:0x%08x") % payload.addr % payload.data);
            break;
        }
        case mgmt_op_t::MGMT_OP_INFO_REQ:
        case mgmt_op_t::MGMT_OP_INFO_RESP: {
            mgmt_op_t::node_info_payload payload =
                static_cast<uint64_t>(get_op_payload());
            stream << "device_id:" << payload.device_id
                   << ", node_type:" << payload.node_type
                   << ", node_inst:" << payload.node_inst
                   << ", ext_info:" << payload.ext_info;
            break;
        }
        default: {
            stream << "-";
            break;
        }
    }
    stream << "\n";
    return stream.str();
}

//! Serialize this hop into a list of 64-bit words
size_t mgmt_hop_t::serialize(std::vector<uint64_t>& target,
    const std::function<uint64_t(uint64_t)>& conv_byte_order,
    const size_t padding_size) const
{
    for (size_t i = 0; i < get_num_ops(); i++) {
        target.push_back(
            conv_byte_order((static_cast<uint64_t>(_ops.at(i).get_op_payload()) << 16)
                            | (static_cast<uint64_t>(_ops.at(i).get_op_code()) << 8)
                            | (static_cast<uint64_t>(get_num_ops() - i - 1) << 0)));
        for (size_t j = 0; j < padding_size; j++) {
            target.push_back(uint64_t(0));
        }
    }
    return get_num_ops();
}

//! Deserialize this hop into from list of 64-bit words
void mgmt_hop_t::deserialize(std::list<uint64_t>& src,
    const std::function<uint64_t(uint64_t)>& conv_byte_order,
    const size_t padding_size)
{
    _ops.clear();
    size_t ops_remaining = 0;
    do {
        // TODO: Change this to a legit exception
        UHD_ASSERT_THROW(!src.empty());

        uint64_t op_word = conv_byte_order(src.front());
        ops_remaining    = static_cast<size_t>(op_word & 0xFF);
        mgmt_op_t op(static_cast<mgmt_op_t::op_code_t>((op_word >> 8) & 0xFF),
            static_cast<uint64_t>((op_word >> 16)),
            static_cast<uint8_t>(op_word & 0xFF));
        _ops.push_back(op);
        src.pop_front();
        for (size_t i = 0; i < padding_size; i++) {
            src.pop_front();
        }

    } while (ops_remaining > 0);
}

std::string mgmt_hop_t::to_string() const
{
    std::stringstream stream;
    for (size_t op_index = 0; op_index < get_num_ops(); op_index++) {
        if (op_index == 0) {
            stream << " -> ";
        } else {
            stream << "    ";
        }
        const mgmt_op_t& op = get_op(op_index);
        stream << op.to_string();
    }
    return stream.str();
}

void mgmt_payload::populate_header(chdr_header& header) const
{
    header.set_pkt_type(PKT_TYPE_MGMT);
    header.set_eob(false);
    header.set_eov(false);
    header.set_num_mdata(0);
    header.set_vc(0);
    header.set_dst_epid(0);
}

size_t mgmt_payload::serialize(uint64_t* buff,
    size_t max_size_bytes,
    const std::function<uint64_t(uint64_t)>& conv_byte_order) const
{
    std::vector<uint64_t> target;
    target.reserve(_padding_size + 1);
    // Insert header
    target.push_back(conv_byte_order(
        (static_cast<uint64_t>(_protover) << 48)
        | (static_cast<uint64_t>(static_cast<uint8_t>(_chdr_w) & 0x7) << 45)
        | (static_cast<uint64_t>(get_num_hops() & 0x3FF) << 16)
        | (static_cast<uint64_t>(_src_epid) << 0)));
    // According to the RFNoC specification section 2.2.6, the MSBs are 0 for
    // all widths greater than 64. This logic adds the padding.
    for (size_t i = 0; i < _padding_size; i++) {
        target.push_back(uint64_t(0));
    }

    // Insert data from each hop
    for (const auto& hop : _hops) {
        hop.serialize(target, conv_byte_order, _padding_size);
    }
    UHD_ASSERT_THROW(target.size() * sizeof(uint64_t) <= max_size_bytes);

    // We use a vector and copy just for ease of implementation
    // These transactions are not performance critical
    std::copy(target.begin(), target.end(), buff);
    return (target.size() * sizeof(uint64_t));
}

void mgmt_payload::deserialize(const uint64_t* buff,
    size_t buff_size,
    const std::function<uint64_t(uint64_t)>& conv_byte_order)
{
    UHD_ASSERT_THROW(buff_size > 1);

    // We use a list and copy just for ease of implementation
    // These transactions are not performance critical
    std::list<uint64_t> src_list(buff, buff + (buff_size * (_padding_size + 1)));

    _hops.clear();

    // Deframe the header
    uint64_t hdr = conv_byte_order(src_list.front());
    _hops.resize(static_cast<size_t>((hdr >> 16) & 0x3FF));
    _src_epid     = static_cast<sep_id_t>(hdr & 0xFFFF);
    _chdr_w       = static_cast<chdr_w_t>((hdr >> 45) & 0x7);
    _protover     = static_cast<uint16_t>((hdr >> 48) & 0xFFFF);
    _padding_size = (chdr_w_to_bits(_chdr_w) / 64) - 1;
    src_list.pop_front();
    // According to the RFNoC specification section 2.2.6, the MSBs are 0 for
    // all widths greater than 64. This logic removes the padding.
    for (size_t i = 0; i < _padding_size; i++) {
        src_list.pop_front();
    }

    // Populate all hops
    for (size_t i = 0; i < get_num_hops(); i++) {
        _hops[i].deserialize(src_list, conv_byte_order, _padding_size);
    }
}

size_t mgmt_payload::get_length() const
{
    size_t length = 1 + _padding_size; /* header */
    for (const auto& hop : this->_hops) {
        length += hop.get_num_ops() + _padding_size;
    }
    return length * sizeof(uint64_t);
}

std::string mgmt_payload::to_string() const
{
    return str(boost::format(
                   "mgmt_payload{src_epid:%lu, chdr_w:%d, protover:0x%x, num_hops:%lu}\n")
               % _src_epid % int(_chdr_w) % _protover % _hops.size());
}

std::string mgmt_payload::hops_to_string() const
{
    std::stringstream stream;
    for (size_t hop_index = 0; hop_index < get_num_hops(); hop_index++) {
        const mgmt_hop_t& hop = get_hop(hop_index);
        stream << hop.to_string();
    }
    return stream.str();
}


bool mgmt_payload::operator==(const mgmt_payload& rhs) const
{
    return (_src_epid == rhs._src_epid) && (_protover == rhs._protover)
           && (_chdr_w == rhs._chdr_w) && (_hops == rhs._hops)
           && (_padding_size == rhs._padding_size);
}
