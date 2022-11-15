//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "chdr_resource/rfnoc_packets_ctrl_mgmt.cpp"
#include "chdr_resource/rfnoc_packets_data.cpp"
#include <uhd/rfnoc/chdr_types.hpp>
#include <uhd/utils/chdr/chdr_packet.hpp>

namespace chdr_util  = uhd::utils::chdr;
namespace chdr_rfnoc = uhd::rfnoc::chdr;

namespace test {

constexpr uhd::rfnoc::chdr_w_t CHDR_W = uhd::rfnoc::CHDR_W_64;

chdr_util::chdr_packet make_control_packet0()
{
    auto header = chdr_rfnoc::chdr_header();
    header.set_pkt_type(chdr_rfnoc::PKT_TYPE_CTRL);
    header.set_length(24);
    header.set_dst_epid(2);
    auto payload     = chdr_rfnoc::ctrl_payload();
    payload.src_epid = 1;
    payload.data_vtr = {0};
    payload.op_code  = chdr_rfnoc::OP_READ;
    return chdr_util::chdr_packet(CHDR_W, header, payload);
}

chdr_util::chdr_packet make_control_packet1()
{
    auto header = chdr_rfnoc::chdr_header();
    header.set_pkt_type(chdr_rfnoc::PKT_TYPE_CTRL);
    header.set_length(24);
    header.set_dst_epid(1);
    auto payload     = chdr_rfnoc::ctrl_payload();
    payload.src_epid = 2;
    payload.is_ack   = true;
    payload.data_vtr = {0x12C60100};
    payload.op_code  = chdr_rfnoc::OP_READ;
    return chdr_util::chdr_packet(CHDR_W, header, payload);
}

chdr_util::chdr_packet make_mgmt_packet0()
{
    auto header = chdr_rfnoc::chdr_header();
    header.set_pkt_type(chdr_rfnoc::PKT_TYPE_MGMT);
    header.set_length(48);
    header.set_seq_num(2);
    auto payload = chdr_rfnoc::mgmt_payload();
    payload.set_header(1, (1 << 8) | (0 << 0), uhd::rfnoc::CHDR_W_64);
    auto hop1 = chdr_rfnoc::mgmt_hop_t();
    hop1.add_op(chdr_rfnoc::mgmt_op_t(chdr_rfnoc::mgmt_op_t::MGMT_OP_NOP));
    payload.add_hop(hop1);
    auto hop2 = chdr_rfnoc::mgmt_hop_t();
    hop2.add_op(chdr_rfnoc::mgmt_op_t(chdr_rfnoc::mgmt_op_t::MGMT_OP_INFO_REQ));
    hop2.add_op(chdr_rfnoc::mgmt_op_t(chdr_rfnoc::mgmt_op_t::MGMT_OP_RETURN));
    payload.add_hop(hop2);
    auto hop3 = chdr_rfnoc::mgmt_hop_t();
    hop3.add_op(chdr_rfnoc::mgmt_op_t(chdr_rfnoc::mgmt_op_t::MGMT_OP_NOP));
    payload.add_hop(hop3);
    return chdr_util::chdr_packet(CHDR_W, header, payload);
}

chdr_util::chdr_packet make_mgmt_packet1()
{
    auto header = chdr_rfnoc::chdr_header();
    header.set_pkt_type(chdr_rfnoc::PKT_TYPE_MGMT);
    header.set_length(48);
    header.set_seq_num(3);
    auto payload = chdr_rfnoc::mgmt_payload();
    payload.set_header(1, (1 << 8) | (0 << 0), uhd::rfnoc::CHDR_W_64);
    auto hop1 = chdr_rfnoc::mgmt_hop_t();
    hop1.add_op(chdr_rfnoc::mgmt_op_t(chdr_rfnoc::mgmt_op_t::MGMT_OP_NOP));
    payload.add_hop(hop1);
    auto hop2 = chdr_rfnoc::mgmt_hop_t();
    hop2.add_op(chdr_rfnoc::mgmt_op_t(chdr_rfnoc::mgmt_op_t::MGMT_OP_CFG_WR_REQ,
        chdr_rfnoc::mgmt_op_t::cfg_payload(0x1, 0x2)));
    hop2.add_op(chdr_rfnoc::mgmt_op_t(chdr_rfnoc::mgmt_op_t::MGMT_OP_RETURN));
    payload.add_hop(hop2);
    auto hop3 = chdr_rfnoc::mgmt_hop_t();
    hop3.add_op(chdr_rfnoc::mgmt_op_t(chdr_rfnoc::mgmt_op_t::MGMT_OP_NOP));
    payload.add_hop(hop3);
    return chdr_util::chdr_packet(CHDR_W, header, payload);
}
chdr_util::chdr_packet make_mgmt_packet2()
{
    auto header = chdr_rfnoc::chdr_header();
    header.set_pkt_type(chdr_rfnoc::PKT_TYPE_MGMT);
    header.set_length(56);
    header.set_seq_num(6);
    auto payload = chdr_rfnoc::mgmt_payload();
    payload.set_header(1, (1 << 8) | (0 << 0), uhd::rfnoc::CHDR_W_64);
    auto hop1 = chdr_rfnoc::mgmt_hop_t();
    hop1.add_op(chdr_rfnoc::mgmt_op_t(chdr_rfnoc::mgmt_op_t::MGMT_OP_NOP));
    payload.add_hop(hop1);
    auto hop2 = chdr_rfnoc::mgmt_hop_t();
    hop2.add_op(chdr_rfnoc::mgmt_op_t(chdr_rfnoc::mgmt_op_t::MGMT_OP_SEL_DEST,
        chdr_rfnoc::mgmt_op_t::sel_dest_payload(static_cast<uint16_t>(3))));
    payload.add_hop(hop2);
    auto hop3 = chdr_rfnoc::mgmt_hop_t();
    hop3.add_op(chdr_rfnoc::mgmt_op_t(chdr_rfnoc::mgmt_op_t::MGMT_OP_INFO_REQ));
    hop3.add_op(chdr_rfnoc::mgmt_op_t(chdr_rfnoc::mgmt_op_t::MGMT_OP_RETURN));
    payload.add_hop(hop3);
    auto hop4 = chdr_rfnoc::mgmt_hop_t();
    hop4.add_op(chdr_rfnoc::mgmt_op_t(chdr_rfnoc::mgmt_op_t::MGMT_OP_NOP));
    payload.add_hop(hop4);
    return chdr_util::chdr_packet(CHDR_W, header, payload);
}

chdr_util::chdr_packet make_strs_packet0()
{
    auto header = chdr_rfnoc::chdr_header();
    header.set_pkt_type(chdr_rfnoc::PKT_TYPE_STRS);
    header.set_length(40);
    header.set_dst_epid(2);
    auto payload           = chdr_rfnoc::strs_payload();
    payload.capacity_bytes = 163840;
    payload.src_epid       = 3;
    payload.capacity_pkts  = 16777215;
    return chdr_util::chdr_packet(CHDR_W, header, payload);
}

chdr_util::chdr_packet make_strs_packet1()
{
    auto header = chdr_rfnoc::chdr_header();
    header.set_pkt_type(chdr_rfnoc::PKT_TYPE_STRS);
    header.set_length(40);
    header.set_dst_epid(2);
    auto payload             = chdr_rfnoc::strs_payload();
    payload.capacity_bytes   = 163840;
    payload.src_epid         = 3;
    payload.xfer_count_pkts  = 1;
    payload.capacity_pkts    = 16777215;
    payload.xfer_count_bytes = 8000;
    return chdr_util::chdr_packet(CHDR_W, header, payload);
}

chdr_util::chdr_packet make_strc_packet0()
{
    auto header = chdr_rfnoc::chdr_header();
    header.set_pkt_type(chdr_rfnoc::PKT_TYPE_STRC);
    header.set_length(24);
    header.set_dst_epid(3);
    auto payload      = chdr_rfnoc::strc_payload();
    payload.num_pkts  = 16777215;
    payload.op_code   = chdr_rfnoc::STRC_INIT;
    payload.src_epid  = 2;
    payload.num_bytes = 5120;
    return chdr_util::chdr_packet(CHDR_W, header, payload);
}

chdr_util::chdr_packet make_strc_packet1()
{
    auto header = chdr_rfnoc::chdr_header();
    header.set_pkt_type(chdr_rfnoc::PKT_TYPE_STRC);
    header.set_length(24);
    header.set_dst_epid(3);
    auto payload      = chdr_rfnoc::strc_payload();
    payload.num_pkts  = 21;
    payload.op_code   = chdr_rfnoc::STRC_RESYNC;
    payload.src_epid  = 2;
    payload.num_bytes = 168000;
    return chdr_util::chdr_packet(CHDR_W, header, payload);
}

chdr_util::chdr_packet make_data_packet0()
{
    auto header = chdr_rfnoc::chdr_header();
    header.set_pkt_type(chdr_rfnoc::PKT_TYPE_DATA_WITH_TS);
    header.set_length(8000);
    header.set_dst_epid(3);
    header.set_seq_num(1);
    boost::optional<uint64_t> timestamp(0x7C40C83);
    uint8_t* data_src;
    size_t data_len;
    std::tie(data_src, data_len) = data_packet::peer1[9];
    std::vector<uint8_t> data(data_src + (2 * 8), data_src + data_len);
    chdr_util::chdr_packet packet(CHDR_W, header, data, timestamp);
    return packet;
}

chdr_util::chdr_packet make_data_packet1()
{
    auto header = chdr_rfnoc::chdr_header();
    header.set_pkt_type(chdr_rfnoc::PKT_TYPE_DATA_WITH_TS);
    header.set_length(4252);
    header.set_dst_epid(3);
    header.set_eob(true);
    header.set_seq_num(1716);
    boost::optional<uint64_t> timestamp(0x21452B97);
    uint8_t* data_src;
    size_t data_len;
    std::tie(data_src, data_len) = data_packet::eob_packet_data;
    std::vector<uint8_t> data(data_src + (2 * 8), data_src + data_len);
    chdr_util::chdr_packet packet(CHDR_W, header, data, timestamp);
    return packet;
}

} // namespace test

chdr_util::chdr_packet packets[] = {test::make_control_packet0(),
    test::make_control_packet1(),
    test::make_mgmt_packet0(),
    test::make_mgmt_packet1(),
    test::make_mgmt_packet2(),
    test::make_strs_packet0(),
    test::make_strs_packet1(),
    test::make_strc_packet0(),
    test::make_strc_packet1(),
    test::make_data_packet0(),
    test::make_data_packet1()};
