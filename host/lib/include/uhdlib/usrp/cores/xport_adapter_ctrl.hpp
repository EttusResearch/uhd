//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/types/device_addr.hpp>
#include <uhdlib/rfnoc/rfnoc_common.hpp>
#include <uhdlib/utils/compat_check.hpp>
#include <cstdlib>
#include <functional>
#include <string>

namespace uhd { namespace usrp {

/*! Transport adapter control
 *
 * This is a C++ version of xport_adapter_ctrl.py and xport_adapter_mgr.py
 * rolled into one class.
 */
class xport_adapter_ctrl
{
public:
    // Register offsets
    static constexpr uint32_t XPORT_ADAPTER_COMPAT_NUM =
        0x0000; // 8 bits major, 8 bits minor
    static constexpr uint32_t XPORT_ADAPTER_INFO      = 0x0004;
    static constexpr uint32_t XPORT_ADAPTER_NODE_INST = 0x0008; // read-only
    static constexpr uint32_t KV_MAC_LO               = 0x000C;
    static constexpr uint32_t KV_MAC_HI               = 0x0010;
    static constexpr uint32_t KV_IPV4                 = 0x0014;
    static constexpr uint32_t KV_UDP_PORT             = 0x0018;
    static constexpr uint32_t KV_CFG                  = 0x001C;
    static constexpr uint32_t KV_IPV4_W_ARP           = 0x0020;
    // The last entry has no equivalent in the FPGA, it will be mapped back to
    // KV_IPV4, but will force a MAC address lookup on the device firmware.

    // Use these as the stream_mode argument in add_remote_ep_route()
    static const char STREAM_MODE_RAW_PAYLOAD[];
    static const char STREAM_MODE_FULL_PACKET[];


    using poke_fn_type = std::function<void(const uint32_t, const uint32_t)>;
    using peek_fn_type = std::function<uint32_t(const uint32_t)>;

    xport_adapter_ctrl(poke_fn_type&& poke_fn,
        peek_fn_type&& peek_fn,
        const bool has_arp,
        const std::string& log_id);

    ~xport_adapter_ctrl(void) = default;

    uhd::rfnoc::sep_inst_t get_xport_adapter_inst() const
    {
        return _ta_inst;
    }

    uhd::compat_num16 get_compat_num() const
    {
        return _compat_num;
    }

    uhd::device_addr_t get_capabilities() const
    {
        return _capabilities;
    }

    void add_remote_ep_route(const rfnoc::sep_inst_t epid,
        const std::string ipv4,
        const std::string port,
        const std::string mac_addr,
        const std::string stream_mode);

private:
    //! Poker object
    poke_fn_type _poke32;

    //! Peeker object
    peek_fn_type _peek32;

    //! Log ID (prefix)
    const std::string _log_id;

    //! Compat numbers
    const uhd::compat_num16 _compat_num;

    //! Transport adapter instance
    const rfnoc::sep_inst_t _ta_inst;

    //! Dictionary of available capabilities (rx_routing, rx_hdr_removal, ...)
    uhd::device_addr_t _capabilities;
};

}} // namespace uhd::usrp
