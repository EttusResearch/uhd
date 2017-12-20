//
// Copyright 2017 Ettus Research, National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0
//

#include "mpmd_xport_mgr.hpp"
#include "mpmd_xport_ctrl_udp.hpp"
#include <uhd/transport/udp_zero_copy.hpp>

using namespace uhd;
using namespace uhd::mpmd::xport;

namespace {

    #if defined(UHD_PLATFORM_MACOS) || defined(UHD_PLATFORM_BSD)
    const size_t MPMD_RX_SW_BUFF_SIZE_ETH        = 0x100000; // 1Mib
    #elif defined(UHD_PLATFORM_LINUX) || defined(UHD_PLATFORM_WIN32)
    //For an ~8k frame size any size >32MiB is just wasted buffer space:
    const size_t MPMD_RX_SW_BUFF_SIZE_ETH        = 0x2000000; // 32 MiB
    #endif

    const size_t MPMD_10GE_DATA_FRAME_MAX_SIZE = 8000; // CHDR packet size in bytes

    std::vector<std::string> get_addrs_from_mb_args(
        const uhd::device_addr_t& mb_args
    ) {
        // mb_args must always include addr
        if (not mb_args.has_key(FIRST_ADDR_KEY)) {
            throw uhd::runtime_error("The " + FIRST_ADDR_KEY + " key must be specified in "
                "device args to create an Ethernet transport to an RFNoC block");
        }
        std::vector<std::string> addrs{mb_args[FIRST_ADDR_KEY]};
        if (mb_args.has_key(SECOND_ADDR_KEY)){
            addrs.push_back(mb_args[SECOND_ADDR_KEY]);
        }
        return addrs;
    }
}


mpmd_xport_ctrl_udp::mpmd_xport_ctrl_udp(
        const uhd::device_addr_t& mb_args
) : _mb_args(mb_args)
  , _recv_args(filter_args(mb_args, "recv"))
  , _send_args(filter_args(mb_args, "send"))
  , _available_addrs(get_addrs_from_mb_args(mb_args))
{
}

uhd::both_xports_t
mpmd_xport_ctrl_udp::make_transport(
        mpmd_xport_mgr::xport_info_t &xport_info,
        const usrp::device3_impl::xport_type_t xport_type,
        const uhd::device_addr_t& xport_args_
) {
    auto xport_args = xport_args_;

    if (xport_type == usrp::device3_impl::RX_DATA
            and not xport_args.has_key("recv_buff_size")) {
        xport_args["recv_buff_size"] =
            std::to_string(MPMD_RX_SW_BUFF_SIZE_ETH);
    }

    transport::zero_copy_xport_params default_buff_args;
    // Create actual UDP transport
    // TODO don't hardcode these
    default_buff_args.send_frame_size = 8000;
    default_buff_args.recv_frame_size = 8000;
    default_buff_args.num_recv_frames = 32;
    default_buff_args.num_send_frames = 32;

    transport::udp_zero_copy::buff_params buff_params;
    auto recv = transport::udp_zero_copy::make(
        xport_info["ipv4"],
        xport_info["port"],
        default_buff_args,
        buff_params,
        xport_args
    );
    const uint16_t port = recv->get_local_port();
    const std::string src_ip_addr = recv->get_local_addr();
    xport_info["src_port"] = std::to_string(port);
    xport_info["src_ipv4"] = src_ip_addr;

    // Create both_xports_t object and finish:
    both_xports_t xports;
    xports.endianness = uhd::ENDIANNESS_BIG;
    xports.send_sid = sid_t(xport_info["send_sid"]);
    xports.recv_sid = xports.send_sid.reversed();
    xports.recv_buff_size = buff_params.recv_buff_size;
    xports.send_buff_size = buff_params.send_buff_size;
    xports.recv = recv; // Note: This is a type cast!
    xports.send = recv; // This too
    return xports;
}

bool mpmd_xport_ctrl_udp::is_valid(
    const mpmd_xport_mgr::xport_info_t& xport_info
) const {
    return std::find(
        _available_addrs.cbegin(),
        _available_addrs.cend(),
        xport_info.at("ipv4")
    ) != _available_addrs.cend();
}

