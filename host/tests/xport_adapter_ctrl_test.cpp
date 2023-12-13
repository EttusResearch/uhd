//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//


#include <uhd/utils/log.hpp>
#include <uhdlib/usrp/cores/xport_adapter_ctrl.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>
#include <map>

using namespace uhd::usrp;


struct mock_xport_adapter_regs
{
    mock_xport_adapter_regs(uhd::rfnoc::sep_inst_t ta_inst,
        const uhd::compat_num16 compat,
        const uint32_t cap_flags)
    {
        mem[uint32_t(xport_adapter_ctrl::XPORT_ADAPTER_COMPAT_NUM)] =
            (static_cast<uint32_t>(compat.get_major()) << 8) | compat.get_minor();
        mem[uint32_t(xport_adapter_ctrl::XPORT_ADAPTER_NODE_INST)] =
            static_cast<uint32_t>(ta_inst);
        mem[uint32_t(xport_adapter_ctrl::XPORT_ADAPTER_INFO)] = cap_flags;
        mem[uint32_t(xport_adapter_ctrl::KV_CFG)]             = 0; // not busy
    }

    void poke32(const uint32_t addr, const uint32_t data)
    {
        mem[addr] = data;
    }

    uint32_t peek32(const uint32_t addr) const
    {
        return mem.count(addr) ? mem.at(addr) : 0xFFFFFFFF;
    }

    std::map<uint32_t, uint32_t> mem;
};

BOOST_AUTO_TEST_CASE(test_xport_adapter_init)
{
    constexpr uhd::rfnoc::sep_inst_t ta_inst = 1;
    uhd::compat_num16 compat(1, 0);
    mock_xport_adapter_regs regs(ta_inst, compat, 0x3);

    xport_adapter_ctrl ta_ctl(
        [&](const uint32_t addr, const uint32_t data) { regs.poke32(addr, data); },
        [&](const uint32_t addr) { return regs.peek32(addr); },
        false,
        "TEST_TA_CTL");

    BOOST_CHECK(ta_ctl.get_compat_num() == compat);
    BOOST_CHECK_EQUAL(ta_ctl.get_xport_adapter_inst(), ta_inst);
    const auto caps = ta_ctl.get_capabilities();
    BOOST_CHECK(caps.has_key("rx_routing"));
    BOOST_CHECK(caps.has_key("rx_hdr_removal"));
    BOOST_CHECK(!caps.has_key("arp"));

    xport_adapter_ctrl ta_ctl_w_arp(
        [&](const uint32_t addr, const uint32_t data) { regs.poke32(addr, data); },
        [&](const uint32_t addr) { return regs.peek32(addr); },
        true,
        "TEST_TA_CTL");
    BOOST_CHECK(ta_ctl_w_arp.get_capabilities().has_key("arp"));
}

BOOST_AUTO_TEST_CASE(test_xport_adapter_sanity_check)
{
    constexpr uhd::rfnoc::sep_inst_t ta_inst = 1;
    uhd::compat_num16 compat(1, 0);
    mock_xport_adapter_regs regs(ta_inst, compat, 0x0);
    xport_adapter_ctrl ta_ctl_nostream(
        [&](const uint32_t addr, const uint32_t data) { regs.poke32(addr, data); },
        [&](const uint32_t addr) { return regs.peek32(addr); },
        false,
        "TEST_TA_CTL");

    BOOST_REQUIRE_THROW(
        ta_ctl_nostream.add_remote_ep_route(0, "1.2.3.4", "5678", "", "INVALID_MODE"),
        uhd::assertion_error);
    BOOST_REQUIRE_THROW(
        ta_ctl_nostream.add_remote_ep_route(
            0, "1.2.3.4", "5678", "", xport_adapter_ctrl::STREAM_MODE_RAW_PAYLOAD),
        uhd::runtime_error);

    regs.mem[uint32_t(xport_adapter_ctrl::XPORT_ADAPTER_INFO)] = 0x1;
    xport_adapter_ctrl ta_ctl_noraw(
        [&](const uint32_t addr, const uint32_t data) { regs.poke32(addr, data); },
        [&](const uint32_t addr) { return regs.peek32(addr); },
        false,
        "TEST_TA_CTL");
    BOOST_REQUIRE_THROW(
        ta_ctl_nostream.add_remote_ep_route(
            0, "1.2.3.4", "5678", "", xport_adapter_ctrl::STREAM_MODE_RAW_PAYLOAD),
        uhd::runtime_error);

    regs.mem[uint32_t(xport_adapter_ctrl::XPORT_ADAPTER_INFO)] = 0x3;
    xport_adapter_ctrl ta_ctl(
        [&](const uint32_t addr, const uint32_t data) { regs.poke32(addr, data); },
        [&](const uint32_t addr) { return regs.peek32(addr); },
        false,
        "TEST_TA_CTL");

    ta_ctl.add_remote_ep_route(23,
        "192.168.40.1",
        "5678",
        "AA:BB:CC:DD:EE:FF",
        xport_adapter_ctrl::STREAM_MODE_RAW_PAYLOAD);
    BOOST_CHECK_EQUAL(regs.mem[uint32_t(xport_adapter_ctrl::KV_MAC_LO)], 0xCCDDEEFF);
    BOOST_CHECK_EQUAL(regs.mem[uint32_t(xport_adapter_ctrl::KV_MAC_HI)], 0x0000AABB);
    BOOST_CHECK_EQUAL(regs.mem[uint32_t(xport_adapter_ctrl::KV_IPV4)], 0xc0a82801);
    BOOST_CHECK_EQUAL(regs.mem[uint32_t(xport_adapter_ctrl::KV_UDP_PORT)], 5678);
    BOOST_CHECK_EQUAL(regs.mem[uint32_t(xport_adapter_ctrl::KV_CFG)] & 0xFFFF, 23);
    BOOST_CHECK_EQUAL((regs.mem[uint32_t(xport_adapter_ctrl::KV_CFG)] >> 16), 1);
}

BOOST_AUTO_TEST_CASE(test_xport_adapter_busy_flag)
{
    constexpr uhd::rfnoc::sep_inst_t ta_inst = 1;
    uhd::compat_num16 compat(1, 0);
    mock_xport_adapter_regs regs(ta_inst, compat, 0x3);
    // We make it permanently busy
    regs.mem[uint32_t(xport_adapter_ctrl::KV_CFG)] = 1 << 31;
    xport_adapter_ctrl ta_ctl(
        [&](const uint32_t addr, const uint32_t data) { regs.poke32(addr, data); },
        [&](const uint32_t addr) { return regs.peek32(addr); },
        false,
        "TEST_TA_CTL");

    UHD_LOG_INFO("TEST", "Expecting error here VVV");
    BOOST_CHECK_THROW(ta_ctl.add_remote_ep_route(23,
                          "192.168.40.1",
                          "5678",
                          "AA:BB:CC:DD:EE:FF",
                          xport_adapter_ctrl::STREAM_MODE_RAW_PAYLOAD),
        uhd::runtime_error);
    UHD_LOG_INFO("TEST", "Expecting error here ^^^");

    // Now we fake out BUSY flag going low after 2 peeks
    int peek_count = 0;
    xport_adapter_ctrl ta_ctl2(
        [&](const uint32_t addr, const uint32_t data) { regs.poke32(addr, data); },
        [&](const uint32_t addr) {
            if (addr == uint32_t(xport_adapter_ctrl::KV_CFG)) {
                UHD_LOG_INFO("TEST", "Detecting peek to KV_CFG...");
                if (++peek_count == 2) {
                    return uint32_t(0);
                }
            }
            return regs.peek32(addr);
        },
        false,
        "TEST_TA_CTL");

    BOOST_CHECK_NO_THROW(ta_ctl2.add_remote_ep_route(23,
        "192.168.40.1",
        "5678",
        "AA:BB:CC:DD:EE:FF",
        xport_adapter_ctrl::STREAM_MODE_RAW_PAYLOAD));
}

BOOST_AUTO_TEST_CASE(test_xport_adapter_arp)
{
    constexpr uhd::rfnoc::sep_inst_t ta_inst = 2;
    uhd::compat_num16 compat(1, 0);
    mock_xport_adapter_regs regs(ta_inst, compat, 0x3);
    xport_adapter_ctrl ta_ctl(
        [&](const uint32_t addr, const uint32_t data) { regs.poke32(addr, data); },
        [&](const uint32_t addr) { return regs.peek32(addr); },
        true,
        "TEST_TA_CTL");

    BOOST_CHECK_NO_THROW(ta_ctl.add_remote_ep_route(
        42, "192.168.30.1", "5678", "", xport_adapter_ctrl::STREAM_MODE_RAW_PAYLOAD));
    BOOST_CHECK(!regs.mem.count(uint32_t(xport_adapter_ctrl::KV_MAC_LO)));
    BOOST_CHECK(!regs.mem.count(uint32_t(xport_adapter_ctrl::KV_MAC_HI)));
    BOOST_CHECK(!regs.mem.count(uint32_t(xport_adapter_ctrl::KV_IPV4)));
    BOOST_CHECK_EQUAL(regs.mem[uint32_t(xport_adapter_ctrl::KV_UDP_PORT)], 5678);
    BOOST_CHECK_EQUAL(regs.mem[uint32_t(xport_adapter_ctrl::KV_CFG)] & 0xFFFF, 42);
    BOOST_CHECK_EQUAL((regs.mem[uint32_t(xport_adapter_ctrl::KV_CFG)] >> 16), 1);
    BOOST_CHECK_EQUAL(regs.mem[uint32_t(xport_adapter_ctrl::KV_IPV4_W_ARP)], 0xc0a81e01);
}
