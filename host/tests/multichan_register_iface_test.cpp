//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/rfnoc/mock_block.hpp>
#include <uhd/rfnoc/multichan_register_iface.hpp>
#include <boost/test/unit_test.hpp>
#include <algorithm>
#include <iostream>
#include <vector>

using namespace uhd::rfnoc;

namespace {

constexpr uint32_t BASE_ADDR   = 0x8000;
constexpr size_t INSTANCE_SIZE = 0x1000;

inline uint32_t get_addr_translation(uint32_t offset, size_t instance)
{
    return offset + BASE_ADDR + INSTANCE_SIZE * instance;
}

} // namespace

BOOST_AUTO_TEST_CASE(test_poke32)
{
    auto mock_reg_iface = std::make_shared<mock_reg_iface_t>();
    register_iface_holder mock_holder{mock_reg_iface};
    multichan_register_iface block_reg_iface{mock_holder, BASE_ADDR, INSTANCE_SIZE};
    uint32_t addr = 0x100;
    uint32_t data = 0x1230;
    block_reg_iface.poke32(addr, data);
    uint32_t abs_addr = get_addr_translation(addr, 0);
    BOOST_CHECK_EQUAL(mock_reg_iface->write_memory[abs_addr], data);
    for (size_t instance = 0; instance < 4; instance++) {
        data = 0xabc0 | instance;
        block_reg_iface.poke32(addr, data, instance);
        abs_addr = get_addr_translation(addr, instance);
        BOOST_CHECK_EQUAL(mock_reg_iface->write_memory[abs_addr], data);
    }
}

BOOST_AUTO_TEST_CASE(test_peek32)
{
    auto mock_reg_iface = std::make_shared<mock_reg_iface_t>();
    register_iface_holder mock_holder{mock_reg_iface};
    multichan_register_iface block_reg_iface{mock_holder, BASE_ADDR, INSTANCE_SIZE};
    uint32_t addr = 0x200;
    for (size_t instance = 0; instance < 4; instance++) {
        uint32_t data                         = 0xdef0 | instance;
        uint32_t abs_addr                     = get_addr_translation(addr, instance);
        mock_reg_iface->read_memory[abs_addr] = data;
        if (instance == 0) {
            BOOST_CHECK_EQUAL(block_reg_iface.peek32(addr), data);
        }
        BOOST_CHECK_EQUAL(block_reg_iface.peek32(addr, instance), data);
    }
}

BOOST_AUTO_TEST_CASE(test_multi_poke32)
{
    auto mock_reg_iface = std::make_shared<mock_reg_iface_t>();
    register_iface_holder mock_holder{mock_reg_iface};
    multichan_register_iface block_reg_iface{mock_holder, BASE_ADDR, INSTANCE_SIZE};
    std::vector<uint32_t> addrs = {0, 4, 8, 12, 16, 20, 24, 28};
    std::vector<uint32_t> data  = {
         0x0000, 0x0010, 0x0200, 0x3000, 0x0004, 0x0050, 0x0600, 0x7000};
    block_reg_iface.multi_poke32(addrs, data);
    for (size_t i = 0; i < addrs.size(); i++) {
        uint32_t abs_addr = get_addr_translation(addrs[i], 0);
        BOOST_CHECK_EQUAL(mock_reg_iface->write_memory[abs_addr], data[i]);
    }
    std::reverse(data.begin(), data.end());
    for (size_t instance = 0; instance < 4; instance++) {
        block_reg_iface.multi_poke32(addrs, data, instance);
        for (size_t i = 0; i < addrs.size(); i++) {
            uint32_t abs_addr = get_addr_translation(addrs[i], instance);
            BOOST_CHECK_EQUAL(mock_reg_iface->write_memory[abs_addr], data[i]);
        }
    }
}

BOOST_AUTO_TEST_CASE(test_block_poke32)
{
    auto mock_reg_iface = std::make_shared<mock_reg_iface_t>();
    register_iface_holder mock_holder{mock_reg_iface};
    multichan_register_iface block_reg_iface{mock_holder, BASE_ADDR, INSTANCE_SIZE};
    uint32_t addr              = 0x100;
    std::vector<uint32_t> data = {
        0x0000, 0x0010, 0x0200, 0x3000, 0x0004, 0x0050, 0x0600, 0x7000};
    block_reg_iface.block_poke32(addr, data);
    for (size_t i = 0; i < data.size(); i++) {
        uint32_t abs_addr = get_addr_translation(addr + i * sizeof(uint32_t), 0);
        BOOST_CHECK_EQUAL(mock_reg_iface->write_memory[abs_addr], data[i]);
    }
    std::reverse(data.begin(), data.end());
    for (size_t instance = 0; instance < 4; instance++) {
        block_reg_iface.block_poke32(addr, data, instance);
        for (size_t i = 0; i < data.size(); i++) {
            uint32_t abs_addr =
                get_addr_translation(addr + i * sizeof(uint32_t), instance);
            BOOST_CHECK_EQUAL(mock_reg_iface->write_memory[abs_addr], data[i]);
        }
    }
}

BOOST_AUTO_TEST_CASE(test_block_peek32)
{
    auto mock_reg_iface = std::make_shared<mock_reg_iface_t>();
    register_iface_holder mock_holder{mock_reg_iface};
    multichan_register_iface block_reg_iface{mock_holder, BASE_ADDR, INSTANCE_SIZE};
    uint32_t addr              = 0x200;
    std::vector<uint32_t> data = {
        0x0008, 0x0090, 0x0a00, 0xb000, 0x000c, 0x00d0, 0x0e00, 0xf000};
    for (size_t instance = 0; instance < 4; instance++) {
        for (size_t i = 0; i < data.size(); i++) {
            uint32_t abs_addr =
                get_addr_translation(addr + i * sizeof(uint32_t), instance);
            mock_reg_iface->read_memory[abs_addr] = data[i];
        }
        std::vector<uint32_t> peek_data =
            block_reg_iface.block_peek32(addr, data.size(), instance);
        BOOST_CHECK_EQUAL(peek_data.size(), data.size());
        for (size_t i = 0; i < data.size(); i++) {
            BOOST_CHECK_EQUAL(peek_data[i], data[i]);
        }
        if (instance == 0) {
            peek_data = block_reg_iface.block_peek32(addr, data.size());
            BOOST_CHECK_EQUAL(peek_data.size(), data.size());
            for (size_t i = 0; i < data.size(); i++) {
                BOOST_CHECK_EQUAL(peek_data[i], data[i]);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(test_poke64)
{
    auto mock_reg_iface = std::make_shared<mock_reg_iface_t>();
    register_iface_holder mock_holder{mock_reg_iface};
    multichan_register_iface block_reg_iface{mock_holder, BASE_ADDR, INSTANCE_SIZE};
    uint32_t addr = 0x100;
    uint64_t data = 0xabcdef12;
    block_reg_iface.poke64(addr, data);
    uint32_t abs_addr = get_addr_translation(addr, 0);
    BOOST_CHECK_EQUAL(
        mock_reg_iface->write_memory[abs_addr], uint32_t(data & 0xFFFFFFFF));
    BOOST_CHECK_EQUAL(
        mock_reg_iface->write_memory[abs_addr + 4], uint32_t((data >> 32) & 0xFFFFFFFF));
    for (size_t instance = 0; instance < 4; instance++) {
        data = 0x12345670 | instance;
        block_reg_iface.poke64(addr, data, instance);
        abs_addr = get_addr_translation(addr, instance);
        BOOST_CHECK_EQUAL(
            mock_reg_iface->write_memory[abs_addr], uint32_t(data & 0xFFFFFFFF));
        BOOST_CHECK_EQUAL(mock_reg_iface->write_memory[abs_addr + 4],
            uint32_t((data >> 32) & 0xFFFFFFFF));
    }
}

BOOST_AUTO_TEST_CASE(test_peek64)
{
    auto mock_reg_iface = std::make_shared<mock_reg_iface_t>();
    register_iface_holder mock_holder{mock_reg_iface};
    multichan_register_iface block_reg_iface{mock_holder, BASE_ADDR, INSTANCE_SIZE};
    uint32_t addr = 0x200;
    for (size_t instance = 0; instance < 4; instance++) {
        uint64_t data                             = 0x9abcdef0 | instance;
        uint32_t abs_addr                         = get_addr_translation(addr, instance);
        mock_reg_iface->read_memory[abs_addr]     = uint32_t(data & 0xFFFFFFFF);
        mock_reg_iface->read_memory[abs_addr + 4] = uint32_t((data >> 32) & 0xFFFFFFFF);
        if (instance == 0) {
            BOOST_CHECK_EQUAL(block_reg_iface.peek64(addr), data);
        }
        BOOST_CHECK_EQUAL(block_reg_iface.peek64(addr, instance), data);
    }
}
