//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/usrp/cores/xport_adapter_ctrl.hpp>
#include <unordered_map>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/asio.hpp>
#include <chrono>
#include <cstdio>
#include <thread>

using namespace uhd::usrp;

namespace {

const uhd::compat_num16 MIN_COMPAT_REMOTE_STRM{1, 0};

const std::unordered_map<std::string, uint32_t> STREAM_MODES{
    {xport_adapter_ctrl::STREAM_MODE_FULL_PACKET, 0},
    {xport_adapter_ctrl::STREAM_MODE_RAW_PAYLOAD, 1}};

std::pair<uint32_t, uint32_t> cast_ipv4_and_port(
    const std::string& ipv4, const std::string& port)
{
    using namespace boost::asio;
    io_service io_service;
    ip::udp::resolver resolver(io_service);
    try {
        ip::udp::resolver::query query(ip::udp::v4(), ipv4, port);
        ip::udp::endpoint endpoint = *resolver.resolve(query);
        return {
            uint32_t(endpoint.address().to_v4().to_ulong()), uint32_t(endpoint.port())};
    } catch (const std::exception&) {
        throw uhd::value_error("Invalid UDP address: " + ipv4 + ":" + port);
    }
}

std::pair<uint32_t, uint32_t> cast_mac(const std::string& mac_addr)
{
    unsigned char mac[8] = {0}; // 8 bytes for conversion to uint64_t
    int ret              = std::sscanf(mac_addr.c_str(),
        "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
        &mac[5],
        &mac[4],
        &mac[3],
        &mac[2],
        &mac[1],
        &mac[0]);
    if (ret != 6) {
        throw uhd::value_error("Invalid MAC address: " + mac_addr);
    }
    const uint32_t mac_msb = *(reinterpret_cast<uint32_t*>(mac + 0));
    const uint32_t mac_lsb = *(reinterpret_cast<uint32_t*>(mac + 4));
    return {mac_msb, mac_lsb};
}


} // namespace

constexpr const char xport_adapter_ctrl::STREAM_MODE_RAW_PAYLOAD[] = "RAW_PAYLOAD";
constexpr const char xport_adapter_ctrl::STREAM_MODE_FULL_PACKET[] = "FULL_PACKET";

xport_adapter_ctrl::xport_adapter_ctrl(poke_fn_type&& poke_fn,
    peek_fn_type&& peek_fn,
    const bool has_arp,
    const std::string& log_id)
    : _poke32(std::move(poke_fn))
    , _peek32(std::move(peek_fn))
    , _log_id(log_id)
    , _compat_num(_peek32(XPORT_ADAPTER_COMPAT_NUM))
    , _ta_inst(
          _compat_num >= MIN_COMPAT_REMOTE_STRM ? _peek32(XPORT_ADAPTER_NODE_INST) : 0)
{
    if (_compat_num >= MIN_COMPAT_REMOTE_STRM) {
        const uint32_t capabilities = _peek32(XPORT_ADAPTER_INFO);
        if (capabilities & (1 << 0)) {
            _capabilities["rx_routing"] = "1";
        }
        if (capabilities & (1 << 1)) {
            _capabilities["rx_hdr_removal"] = "1";
        }
        if (has_arp) {
            _capabilities["arp"] = "1";
        }
        _capabilities["ta_inst"] = std::to_string(_ta_inst);
    }
    UHD_LOG_TRACE(
        _log_id, "Remote streaming capabilities: " << _capabilities.to_string());
}


void xport_adapter_ctrl::add_remote_ep_route(const uhd::rfnoc::sep_inst_t epid,
    const std::string ipv4,
    const std::string port,
    const std::string mac_addr,
    const std::string stream_mode)
{
    const std::string stream_mode_upper = boost::to_upper_copy(stream_mode);
    UHD_ASSERT_THROW(STREAM_MODES.count(stream_mode_upper));
    if (!_capabilities.has_key("rx_routing")) {
        throw uhd::runtime_error(
            "This transport adapter does not support routing to remote "
            "destinations!");
    }
    if (stream_mode_upper == STREAM_MODE_RAW_PAYLOAD
        && !_capabilities.has_key("rx_hdr_removal")) {
        throw uhd::runtime_error(
            "Requesting to remove CHDR headers, but feature not enabled!");
    }

    if (ipv4.empty()) {
        throw uhd::value_error("Must provide valid IPv4 address!");
    }
    if (port.empty()) {
        throw uhd::value_error("Must provide valid port value!");
    }
    if (mac_addr.empty() && !_capabilities.has_key("arp")) {
        throw uhd::value_error(
            "Device has no ARP capabilities -- must provide MAC address!");
    }
    const bool request_arp         = mac_addr.empty();
    const auto ipv4_and_port       = cast_ipv4_and_port(ipv4, port);
    const uint32_t stream_mode_int = STREAM_MODES.at(stream_mode_upper);
    const uint32_t cfg_word        = epid | (stream_mode_int << 16);

    // Check TA is ready by polling BUSY flag
    using namespace std::chrono_literals;
    const auto timeout = std::chrono::steady_clock::now() + 500ms;
    while (bool(_peek32(KV_CFG) & (1 << 31))) {
        if (std::chrono::steady_clock::now() > timeout) {
            UHD_LOG_THROW(uhd::runtime_error,
                _log_id,
                "Timeout while polling BUSY flag on transport adapter!");
        }
        std::this_thread::sleep_for(100ms);
    }

    // Now write settings to TA
    UHD_LOG_DEBUG(_log_id,
        "On transport adapter " << _ta_inst << ": Adding route from EPID " << epid
                                << " to destination " << ipv4 << ":" << port
                                << " (MAC Address: " << (request_arp ? "AUTO" : mac_addr)
                                << "), stream mode " << stream_mode_upper << " ("
                                << stream_mode_int << ")");
    if (!request_arp) {
        const auto mac_int = cast_mac(mac_addr);
        _poke32(KV_MAC_LO, mac_int.first);
        _poke32(KV_MAC_HI, mac_int.second);
        _poke32(KV_IPV4, ipv4_and_port.first);
    } else {
        // If the user didn't specify MAC, then the device firmware can try and
        // look it up.
        constexpr int num_arp_tries   = 3;
        constexpr auto retry_interval = 300ms;
        bool arp_successful           = false;
        for (int i = 0; i < num_arp_tries; i++) {
            try {
                _poke32(KV_IPV4_W_ARP, ipv4_and_port.first);
                arp_successful = true;
                break;
            } catch (const uhd::lookup_error&) {
                UHD_LOG_TRACE(_log_id, "ARP lookup failed for IP address " << ipv4);
                std::this_thread::sleep_for(retry_interval);
            }
        }
        if (!arp_successful) {
            UHD_LOG_THROW(uhd::lookup_error,
                _log_id,
                "Device was unable to look up Ethernet (MAC) address for IP "
                "address "
                    << ipv4
                    << ". Make sure device is correctly connected, "
                       "or provide MAC address manually.");
        }
    }
    _poke32(KV_UDP_PORT, ipv4_and_port.second);
    _poke32(KV_CFG, cfg_word);
}
