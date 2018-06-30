//
// Copyright 2017 Ettus Research, National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "mpmd_impl.hpp"
#include "mpmd_xport_mgr.hpp"
#include "mpmd_xport_ctrl_udp.hpp"
#include <uhd/transport/udp_zero_copy.hpp>
#include <uhd/transport/udp_simple.hpp>
#include <uhd/transport/udp_constants.hpp>


using namespace uhd;
using namespace uhd::mpmd::xport;

namespace {

    #if defined(UHD_PLATFORM_MACOS) || defined(UHD_PLATFORM_BSD)
    //! Size of the host-side socket buffer for RX
    const size_t MPMD_RX_SW_BUFF_SIZE_ETH        = 0x100000; // 1Mib
    #elif defined(UHD_PLATFORM_LINUX) || defined(UHD_PLATFORM_WIN32)
    //! Size of the host-side socket buffer for RX
    // For an ~8k frame size any size >32MiB is just wasted buffer space
    const size_t MPMD_RX_SW_BUFF_SIZE_ETH        = 0x2000000; // 32 MiB
    #endif

    //! Maximum CHDR packet size in bytes
    const size_t MPMD_10GE_DATA_FRAME_MAX_SIZE = 8000;

    //! Number of send/recv frames
    const size_t MPMD_ETH_NUM_FRAMES = 32;

    //! For MTU discovery, the time we wait for a packet before calling it
    // oversized (seconds).
    const double MPMD_MTU_DISCOVERY_TIMEOUT = 0.02;

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

    /*! Do a binary search to discover MTU
     *
     * Uses the MPM echo service to figure out MTU. We simply send a bunch of
     * packets and see if they come back until we converged on the path MTU.
     * The end result must lie between \p min_frame_size and \p max_frame_size.
     *
     * \param address IP address
     * \param port UDP port (yeah it's a string!)
     * \param min_frame_size Minimum frame size, initialize algorithm to start
     *                       with this value
     * \param max_frame_size Maximum frame size, initialize algorithm to start
     *                       with this value
     * \param echo_timeout Timeout value in seconds. For frame sizes that
     *                     exceed the MTU, we don't expect a response, and this
     *                     is the amount of time we'll wait before we assume
     *                     the frame size exceeds the MTU.
     */
    size_t discover_mtu(
            const std::string &address,
            const std::string &port,
            size_t min_frame_size,
            size_t max_frame_size,
            const double echo_timeout = 0.020
    ) {
        const size_t echo_prefix_offset =
            uhd::mpmd::mpmd_impl::MPM_ECHO_CMD.size();
        const size_t mtu_hdr_len = echo_prefix_offset + 10;
        UHD_ASSERT_THROW(min_frame_size < max_frame_size);
        UHD_ASSERT_THROW(min_frame_size % 4 == 0);
        UHD_ASSERT_THROW(max_frame_size % 4 == 0);
        UHD_ASSERT_THROW(min_frame_size >= echo_prefix_offset + mtu_hdr_len);
        using namespace uhd::transport;
        // The return port will probably differ from the discovery port, so we
        // need a "broadcast" UDP connection; using make_connected() would
        // drop packets
        udp_simple::sptr udp = udp_simple::make_broadcast(address, port);
        std::string send_buf(uhd::mpmd::mpmd_impl::MPM_ECHO_CMD);
        send_buf.resize(max_frame_size, '#');
        UHD_ASSERT_THROW(send_buf.size() == max_frame_size);
        std::vector<uint8_t> recv_buf;
        recv_buf.resize(max_frame_size, ' ');

        // Little helper to check returned packets match the sent ones
        auto require_bufs_match = [&recv_buf, &send_buf, mtu_hdr_len](
                const size_t len
            ){
                if (len < mtu_hdr_len or std::memcmp(
                        (void *) &recv_buf[0],
                        (void *) &send_buf[0],
                        mtu_hdr_len
                ) != 0) {
                    throw uhd::runtime_error("Unexpected content of MTU "
                                             "discovery return packet!");
                }
            };
        UHD_LOG_TRACE("MPMD", "Determining UDP MTU... ");
        size_t seq_no = 0;
        while (min_frame_size < max_frame_size) {
            // Only test multiples of 4 bytes!
            const size_t test_frame_size =
                (max_frame_size/2 + min_frame_size/2 + 3) & ~size_t(3);
            // Encode sequence number and current size in the string, makes it
            // easy to debug in code or Wireshark. Is also used for identifying
            // response packets.
            std::sprintf(
                &send_buf[echo_prefix_offset],
                ";%04lu,%04lu",
                seq_no++,
                test_frame_size
            );
            UHD_LOG_TRACE("MPMD", "Testing frame size " << test_frame_size);
            udp->send(boost::asio::buffer(&send_buf[0], test_frame_size));

            const size_t len =
                udp->recv(boost::asio::buffer(recv_buf), echo_timeout);
            if (len == 0) {
                // Nothing received, so this is probably too big
                max_frame_size = test_frame_size - 4;
            } else if (len >= test_frame_size) {
                // Size went through, so bump the minimum
                require_bufs_match(len);
                min_frame_size = test_frame_size;
            } else if (len < test_frame_size) {
                // This is an odd case. Something must have snipped the packet
                // on the way back. Still, we'll just back off and try
                // something smaller.
                UHD_LOG_DEBUG("MPMD",
                    "Unexpected packet truncation during MTU discovery.");
                require_bufs_match(len);
                max_frame_size = len;
            }
        }
        UHD_LOG_DEBUG("MPMD",
            "Path MTU for address " << address << ": " << min_frame_size);
        return min_frame_size;
    }

}


mpmd_xport_ctrl_udp::mpmd_xport_ctrl_udp(
        const uhd::device_addr_t& mb_args
) : _mb_args(mb_args)
  , _recv_args(filter_args(mb_args, "recv"))
  , _send_args(filter_args(mb_args, "send"))
  , _available_addrs(get_addrs_from_mb_args(mb_args))
  , _mtu(MPMD_10GE_DATA_FRAME_MAX_SIZE)
{
    const std::string mpm_discovery_port = _mb_args.get(
        mpmd_impl::MPM_DISCOVERY_PORT_KEY,
        std::to_string(mpmd_impl::MPM_DISCOVERY_PORT)
    );
    auto discover_mtu_for_ip = [mpm_discovery_port](const std::string &ip_addr){
        return discover_mtu(
            ip_addr,
            mpm_discovery_port,
            IP_PROTOCOL_MIN_MTU_SIZE-IP_PROTOCOL_UDP_PLUS_IP_HEADER,
            MPMD_10GE_DATA_FRAME_MAX_SIZE,
            MPMD_MTU_DISCOVERY_TIMEOUT
        );
    };

    for (const auto &ip_addr : _available_addrs) {
        _mtu = std::min(_mtu, discover_mtu_for_ip(ip_addr));
    }
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
    default_buff_args.send_frame_size = get_mtu(uhd::TX_DIRECTION);
    default_buff_args.recv_frame_size = get_mtu(uhd::RX_DIRECTION);
    default_buff_args.num_recv_frames = MPMD_ETH_NUM_FRAMES;
    default_buff_args.num_send_frames = MPMD_ETH_NUM_FRAMES;

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

size_t mpmd_xport_ctrl_udp::get_mtu(const uhd::direction_t /*dir*/) const
{
    return _mtu;
}
