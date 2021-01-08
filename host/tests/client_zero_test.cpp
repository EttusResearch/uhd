//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/rfnoc/mock_block.hpp>
#include <uhd/rfnoc/register_iface.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/rfnoc/client_zero.hpp>
#include <uhdlib/utils/narrow.hpp>
#include <unordered_map>
#include <boost/test/unit_test.hpp>
#include <cstdint>
#include <cstring>
#include <memory>

using namespace uhd;
using namespace uhd::rfnoc;


class client_zero_test_iface : public mock_reg_iface_t
{
public:
    client_zero_test_iface(uint16_t device_id) : mock_reg_iface_t()
    {
        read_memory[0x0000] = (0x12C6 << 16) | GLOBAL_PROTOVER;
        read_memory[0x0008] = NUM_EDGES;
        read_memory[0x0004] = (STATIC_ROUTER_PRESENT << 31) | (CHDR_XBAR_PRESENT << 30)
                              | (NUM_XPORTS << 20) | ((NUM_BLOCKS & 0X3FF) << 10)
                              | NUM_STREAM_ENDPOINTS;
        read_memory[0x0010] = NUM_XPORTS;
        read_memory[0x000C] = device_id | (DEVICE_TYPE << 16);
    }

    ~client_zero_test_iface() override = default;

    /**************************************************************************
     * Test API
     *************************************************************************/
    size_t register_block(const size_t num_in,
        const size_t num_out,
        const size_t ctrl_fifo_size,
        const size_t ctrl_max_async_msgs,
        const size_t mtu,
        const uint32_t noc_id)
    {
        const uint32_t block_offset =
            (1 + NUM_STREAM_ENDPOINTS + num_registered_blocks) * SLOT_OFFSET;
        read_memory[block_offset + 0] =
            (BLOCK_PROTOVER & 0x3F) | (num_in & 0x3F) << 6 | (num_out & 0x3F) << 12
            | (ctrl_fifo_size & 0x3F) << 18 | (ctrl_max_async_msgs & 0xFF) << 24;
        read_memory[block_offset + 4] = noc_id;
        read_memory[block_offset + 8] = (mtu & 0x3F) << 2
                                        | 0x2; // flush flags: active is low, done is high
        num_registered_blocks++;
        set_port_cnt_reg(num_registered_blocks);
        return num_registered_blocks - 1;
    }

    void add_connection(
        uint16_t src_blk, uint8_t src_port, uint16_t dst_blk, uint8_t dst_port)
    {
        num_connections++;
        read_memory[EDGE_TABLE_OFFSET] = num_connections & 0x3FF;
        read_memory[EDGE_TABLE_OFFSET + num_connections * 4] =
            (src_blk & 0x3F) << 22 | (src_port & 0x3F) << 16 | (dst_blk & 0x3FF) << 6
            | (dst_port & 0x3F) << 0;
        read_memory[0x0008] = num_connections;
    }

    void set_port_cnt_reg(uint16_t num_blocks)
    {
        read_memory[0x0004] = (STATIC_ROUTER_PRESENT << 31) | (CHDR_XBAR_PRESENT << 30)
                              | (NUM_XPORTS << 20) | ((num_blocks & 0x3FF) << 10)
                              | NUM_STREAM_ENDPOINTS;
    }

    void _poke_cb(
        uint32_t addr, uint32_t data, uhd::time_spec_t /*time*/, bool /*ack*/) override
    {
        if (addr < (1 + NUM_STREAM_ENDPOINTS) * SLOT_OFFSET
            || (addr % SLOT_OFFSET != 0 && addr % SLOT_OFFSET != 4)) {
            UHD_LOG_WARNING("MOCK_REG_IFACE",
                "Client Zero only requires pokes to block flush and reset addresses! "
                "Address "
                    << addr << " is not supposed to be poked.");
            UHD_THROW_INVALID_CODE_PATH();
        }

        const size_t blockno = (addr / SLOT_OFFSET) - 1 - NUM_STREAM_ENDPOINTS;

        if ((addr % SLOT_OFFSET) == 0) {
            UHD_LOG_INFO("MOCK_REG_IFACE",
                "Block: " << blockno << " Set flush timeout to " << data);
            last_flush_timeout = data;
            return;
        }
        if ((addr % SLOT_OFFSET) == 4) {
            UHD_LOG_INFO("MOCK_REG_IFACE",
                "Block: " << blockno << " Set flush/reset bits to flush=" << (data & 0x1)
                          << ",ctrl_rst=" << ((data >> 1) & 0x1)
                          << ",chdr_rst=" << ((data >> 2) & 0x1));
        }
    }

    void _peek_cb(uint32_t addr, time_spec_t /*time*/) override
    {
        if (read_memory.count(addr) == 0) {
            std::cout << "Bad peek32, addr=" << addr << std::endl;
            throw uhd::index_error("Bad peek32 in unittest");
        }
    }

    uint32_t last_flush_timeout = 0;

    /**************************************************************************
     * Memory banks and defaults
     *************************************************************************/
    uint16_t GLOBAL_PROTOVER      = 23; // 16 bits
    uint8_t BLOCK_PROTOVER        = 42; // 6 bits
    uint8_t STATIC_ROUTER_PRESENT = 1; // 1 bit
    uint8_t CHDR_XBAR_PRESENT     = 1; // 1 bit
    uint16_t NUM_XPORTS           = 3 & 0x3FF; // 10 bits
    uint16_t NUM_STREAM_ENDPOINTS = 2 & 0x3FF; // 10 bits
    uint16_t NUM_EDGES            = 6 & 0xFFF; // 12 bits, just a default
    uint16_t NUM_BLOCKS           = 4 & 0xFFF; // 12 bits, just a default
    uint16_t DEVICE_TYPE          = 0xABCD;

    static constexpr uint32_t SLOT_OFFSET       = 512 / 8; // 512 bits per slot
    static constexpr uint32_t EDGE_TABLE_OFFSET = 0x10000;

private:
    size_t num_registered_blocks = 0;
    uint32_t num_connections     = 0;
};

constexpr uint32_t client_zero_test_iface::EDGE_TABLE_OFFSET;
constexpr uint32_t client_zero_test_iface::SLOT_OFFSET;


BOOST_AUTO_TEST_CASE(simple_read_if_chdr_pkt)
{
    constexpr uint16_t DEVICE_ID           = 0xBEEF;
    constexpr uint16_t CTRL_FIFO_SIZE      = 5; // in words
    constexpr uint16_t CTRL_MAX_ASYNC_MSGS = 2;
    constexpr uint16_t MTU                 = 40; // FIXME in words?
    auto mock_reg_iface = std::make_shared<client_zero_test_iface>(DEVICE_ID);

    // Prime the pump: We add some blocks and connections
    size_t sep0_id   = 0;
    size_t sep1_id   = 1;
    size_t radio0_id = mock_reg_iface->register_block(
        1, 1, CTRL_FIFO_SIZE, CTRL_MAX_ASYNC_MSGS, MTU, 0x12AD1000);
    size_t ddc0_id = mock_reg_iface->register_block(
        1, 1, CTRL_FIFO_SIZE, CTRL_MAX_ASYNC_MSGS, MTU, 0xDDC00000);
    size_t duc0_id = mock_reg_iface->register_block(
        1, 1, CTRL_FIFO_SIZE, CTRL_MAX_ASYNC_MSGS, MTU, 0xD11C0000);
    size_t radio1_id = mock_reg_iface->register_block(
        1, 1, CTRL_FIFO_SIZE, CTRL_MAX_ASYNC_MSGS, MTU, 0x12AD1000);
    size_t ddc1_id = mock_reg_iface->register_block(
        1, 1, CTRL_FIFO_SIZE, CTRL_MAX_ASYNC_MSGS, MTU, 0xDDC00000);
    size_t duc1_id = mock_reg_iface->register_block(
        1, 1, CTRL_FIFO_SIZE, CTRL_MAX_ASYNC_MSGS, MTU, 0xD11C0000);
    // Connect SEP -> DUC -> RADIO -> DDC -> SEP
    mock_reg_iface->add_connection(sep0_id, 0, duc0_id, 0);
    mock_reg_iface->add_connection(duc0_id, 0, radio0_id, 0);
    mock_reg_iface->add_connection(radio0_id, 0, ddc0_id, 0);
    mock_reg_iface->add_connection(ddc0_id, 0, sep0_id, 0);
    mock_reg_iface->add_connection(sep1_id, 0, duc1_id, 0);
    mock_reg_iface->add_connection(duc1_id, 0, radio1_id, 0);
    mock_reg_iface->add_connection(radio1_id, 0, ddc1_id, 0);
    mock_reg_iface->add_connection(ddc1_id, 0, sep1_id, 0);
    constexpr size_t num_edges = 8; // Number of lines above

    auto mock_client0 = std::make_shared<uhd::rfnoc::detail::client_zero>(mock_reg_iface);

    BOOST_CHECK_EQUAL(mock_client0->get_proto_ver(), mock_reg_iface->GLOBAL_PROTOVER);
    BOOST_CHECK_EQUAL(mock_client0->get_device_type(), mock_reg_iface->DEVICE_TYPE);
    BOOST_CHECK_EQUAL(mock_client0->get_num_blocks(), 6);
    BOOST_CHECK_EQUAL(
        mock_client0->get_num_stream_endpoints(), mock_reg_iface->NUM_STREAM_ENDPOINTS);
    BOOST_CHECK_EQUAL(mock_client0->get_num_transports(), mock_reg_iface->NUM_XPORTS);
    BOOST_CHECK_EQUAL(
        mock_client0->has_chdr_crossbar(), mock_reg_iface->CHDR_XBAR_PRESENT);
    BOOST_CHECK_EQUAL(mock_client0->get_num_edges(), num_edges);

    auto adj_list = mock_client0->get_adjacency_list();
    BOOST_CHECK_EQUAL(adj_list.size(), num_edges);
    auto edge0 = adj_list.at(0);
    BOOST_CHECK_EQUAL(edge0.src_blk_index, sep0_id);
    BOOST_CHECK_EQUAL(edge0.src_blk_port, 0);
    BOOST_CHECK_EQUAL(edge0.dst_blk_index, duc0_id);
    BOOST_CHECK_EQUAL(edge0.dst_blk_port, 0);

    // get_noc_id()
    BOOST_CHECK_THROW(mock_client0->get_noc_id(0), uhd::index_error); // Client0
    BOOST_CHECK_THROW(mock_client0->get_noc_id(1), uhd::index_error); // sep0
    BOOST_CHECK_THROW(mock_client0->get_noc_id(2), uhd::index_error); // sep1
    // Check NOC IDs of all of our blocks
    BOOST_CHECK_EQUAL(mock_client0->get_noc_id(3), 0x12AD1000);
    BOOST_CHECK_EQUAL(mock_client0->get_noc_id(4), 0xDDC00000);
    BOOST_CHECK_EQUAL(mock_client0->get_noc_id(5), 0xD11C0000);
    BOOST_CHECK_EQUAL(mock_client0->get_noc_id(6), 0x12AD1000);
    BOOST_CHECK_EQUAL(mock_client0->get_noc_id(7), 0xDDC00000);
    BOOST_CHECK_EQUAL(mock_client0->get_noc_id(8), 0xD11C0000);
    // Check an out-of-bounds query
    BOOST_CHECK_THROW(mock_client0->get_noc_id(9), uhd::index_error);

    // Flush flags: by default, we set active is low, done is high
    BOOST_CHECK_EQUAL(mock_client0->get_flush_active(3), false);
    BOOST_CHECK_EQUAL(mock_client0->get_flush_done(3), true);
    // Flushing and Reset
    UHD_LOG_INFO("TEST", "Setting and resetting flush flags...");
    BOOST_CHECK_THROW(mock_client0->set_flush(0), uhd::index_error);
    // First block is on port 3
    BOOST_CHECK_NO_THROW(mock_client0->set_flush(3));
    BOOST_CHECK_THROW(mock_client0->set_flush(9), uhd::index_error);
    BOOST_CHECK_THROW(mock_client0->reset_ctrl(0), uhd::index_error);
    BOOST_CHECK_NO_THROW(mock_client0->reset_ctrl(3));
    BOOST_CHECK_THROW(mock_client0->reset_ctrl(9), uhd::index_error);
    BOOST_CHECK_THROW(mock_client0->reset_chdr(0), uhd::index_error);
    BOOST_CHECK_NO_THROW(mock_client0->reset_chdr(3));
    BOOST_CHECK_THROW(mock_client0->reset_chdr(9), uhd::index_error);
    UHD_LOG_INFO("TEST", "Done Setting and resetting flush flags.");

    // Block Config
    auto mock_config = mock_client0->get_block_info(3);
    BOOST_CHECK_EQUAL(mock_config.protover, mock_reg_iface->BLOCK_PROTOVER);
    BOOST_CHECK_EQUAL(mock_config.num_inputs, 1);
    BOOST_CHECK_EQUAL(mock_config.num_outputs, 1);
    BOOST_CHECK_EQUAL(mock_config.ctrl_fifo_size, CTRL_FIFO_SIZE);
    BOOST_CHECK_EQUAL(mock_config.ctrl_max_async_msgs, CTRL_MAX_ASYNC_MSGS);
    BOOST_CHECK_EQUAL(mock_config.data_mtu, MTU);
}
