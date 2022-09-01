//
// Copyright 2017 Ettus Research, National Instruments Company
// Copyright 2019 Ettus Research, National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "mpmd_link_if_ctrl_udp.hpp"
#include "mpmd_impl.hpp"
#include "mpmd_link_if_mgr.hpp"
#include <uhd/rfnoc/constants.hpp>
#include <uhd/transport/udp_constants.hpp>
#include <uhd/transport/udp_simple.hpp>
#include <uhd/utils/cast.hpp>
#include <uhdlib/rfnoc/rfnoc_common.hpp>
#include <uhdlib/transport/udp_boost_asio_link.hpp>
#include <uhdlib/transport/udp_common.hpp>
#include <uhdlib/utils/narrow.hpp>
#include <string>
#ifdef HAVE_DPDK
#    include <uhdlib/transport/dpdk_simple.hpp>
#    include <uhdlib/transport/udp_dpdk_link.hpp>
#endif

using namespace uhd;
using namespace uhd::transport;
using namespace uhd::mpmd::xport;

namespace {

//! Maximum CHDR packet size in bytes.
// Our 10GbE connections use custom FPGA code which caps frames at 8192 bytes.
// However, we artificially limit this to a smaller frame size, which gives us
// a safety margin.
const size_t MPMD_10GE_DATA_FRAME_MAX_SIZE = 8016;
// For 1 GbE, we either go through the the SFP+ port, which supports up to 8192
// bytes, or the RJ45 port, which uses DMA to the FPGA fabric and supports even
// larger packets.  However, there is a known issue where the MTU discovery can
// incorrectly detect a size that is larger than the true MTU size.  The default
// MTU size for 1GbE is 1500 and that is sufficient for the highest sample rates
// supported over 1GbE, so it is capped at 1500 here.
const size_t MPMD_1GE_DATA_FRAME_MAX_SIZE = 1500;

//! Number of send/recv frames
const size_t MPMD_ETH_NUM_FRAMES = 32;

//! Buffer depth in seconds. We use the link rate to determine how large buffers
// must be to store this many seconds worth of data.
const double MPMD_BUFFER_DEPTH = 20.0e-3; // s
//! For MTU discovery, the time we wait for a packet before calling it
// oversized (seconds).
const double MPMD_MTU_DISCOVERY_TIMEOUT = 0.02;

// TODO: move these to appropriate header file for all other devices
const size_t MAX_RATE_1GIGE  = 1e9 / 8; // byte/s
const size_t MAX_RATE_10GIGE = 10e9 / 8; // byte/s


mpmd_link_if_ctrl_udp::udp_link_info_map get_udp_info_from_xport_info(
    const mpmd_link_if_mgr::xport_info_list_t& link_info_list)
{
    mpmd_link_if_ctrl_udp::udp_link_info_map result;
    for (const auto& link_info : link_info_list) {
        if (!link_info.count("ipv4")) {
            UHD_LOG_ERROR("MPMD::XPORT::UDP",
                "Invalid response from get_chdr_link_options()! No `ipv4' key!");
            throw uhd::runtime_error(
                "Invalid response from get_chdr_link_options()! No `ipv4' key!");
        }
        if (!link_info.count("port")) {
            UHD_LOG_ERROR("MPMD::XPORT::UDP",
                "Invalid response from get_chdr_link_options()! No `port' key!");
            throw uhd::runtime_error(
                "Invalid response from get_chdr_link_options()! No `port' key!");
        }
        const std::string udp_port = link_info.at("port");
        const size_t link_rate     = link_info.count("link_rate")
                                     ? std::stoul(link_info.at("link_rate"))
                                     : MAX_RATE_1GIGE;
        const std::string link_type = link_info.at("type");
        const size_t if_mtu         = std::stoul(link_info.at("mtu"));
        result.emplace(link_info.at("ipv4"),
            mpmd_link_if_ctrl_udp::udp_link_info_t{
                udp_port, link_rate, link_type, if_mtu});
    }

    return result;
}

std::vector<std::string> get_addrs_from_mb_args(const uhd::device_addr_t& mb_args,
    const mpmd_link_if_ctrl_udp::udp_link_info_map& link_info_list)
{
    std::vector<std::string> addrs;
    if (!link_info_list.empty()
        && link_info_list.begin()->second.link_type == "internal") {
        // If link_type is "internal" we are local. In this case
        // use this address always. MPM knows better than us.
        addrs.push_back(link_info_list.begin()->first);
    } else {
        if (mb_args.has_key(FIRST_ADDR_KEY)) {
            addrs.push_back(mb_args[FIRST_ADDR_KEY]);
        }
        if (mb_args.has_key(SECOND_ADDR_KEY)) {
            addrs.push_back(mb_args[SECOND_ADDR_KEY]);
        }
        if (mb_args.has_key(THIRD_ADDR_KEY)) {
            addrs.push_back(mb_args[THIRD_ADDR_KEY]);
        }
        if (mb_args.has_key(FOURTH_ADDR_KEY)) {
            addrs.push_back(mb_args[FOURTH_ADDR_KEY]);
        }
    }
    if(addrs.empty()) {
        if (!link_info_list.empty()) {
            addrs.push_back(link_info_list.begin()->first);
        } else {
            UHD_LOG_WARNING("MPMD::XPORT::UDP",
                "The `" << FIRST_ADDR_KEY
                        << "' key must be specified in "
                        "device args to create an Ethernet transport to an RFNoC block");
            return {};
        }
    }

    // This is where in UHD we encode the knowledge about what
    // get_chdr_link_options() returns to us.
    for (const auto& ip_addr : addrs) {
        if (link_info_list.count(ip_addr)) {
            continue;
        }
        UHD_LOG_WARNING("MPMD::XPORT::UDP",
            "Cannot create UDP link to device: The IP address `"
                << ip_addr << "' is requested, but not reachable.");
        return {};
    }

    return addrs;
}

/*! Run a plausibility check on a detected MTU, and return a value that passes
 * custom constraints.
 *
 * This function forcibly overrides the detected MTU value using hardcoded
 * heuristics/rules, even if the detected MTU is actually correct!
 * These rules should thus be chosen very carefully, and should only coerce down
 * (i.e., the return value should be smaller than argument).
 */
size_t run_mtu_plausibility_check(const size_t detected_mtu)
{
    // 1 GbE MTU check: We have observed that the detected path MTU for 1 GbE
    // devices can come out a few bytes too high over 1 GbE. This is most likely
    // due to some drivers being a little more tolerant with larger-than-MTU
    // packets, which is not helpful for us. When the MTU detection errs on the
    // large side, it can happen that either packets going from UHD to the
    // device get fragmented (this is bad, the USRP can't defragment) or that
    // packets coming from the device won't get accepted by our NIC/driver,
    // causing drops (this is the rarer case). We avoid this by detecting typical
    // 1 GbE MTU sizes and coercing them to 1472 bytes. When using a NIC MTU of
    // 1500, we have observed detected MTUs of 1476 up to 1488 bytes, when they
    // should be 1472 bytes instead.
    {
        constexpr size_t DEFAULT_1GBE_MTU          = 1472; // bytes
        constexpr size_t MIN_1GBE_MTU_COERCE_VALUE = 1472; // bytes
        constexpr size_t MAX_1GBE_MTU_COERCE_VALUE = 1500; // bytes
        if (detected_mtu > MIN_1GBE_MTU_COERCE_VALUE
            && detected_mtu < MAX_1GBE_MTU_COERCE_VALUE) {
            UHD_LOG_DEBUG("MPMD",
                "MTU discovery detected "
                    << detected_mtu
                    << " bytes. This may be due to a faulty MTU discovery. Coercing to "
                    << DEFAULT_1GBE_MTU << " bytes.");
            return DEFAULT_1GBE_MTU;
        }
    } // End 1 GbE MTU check.

    // If no one raises any red flags, we let the detected MTU slide.
    return detected_mtu;
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
size_t discover_mtu(const std::string& address,
    const std::string& port,
    size_t min_frame_size,
    size_t max_frame_size,
    const double echo_timeout,
    const bool use_dpdk)
{
    //! Function to create a udp_simple::sptr (kernel-based or DPDK-based)
    using udp_simple_factory_t = std::function<uhd::transport::udp_simple::sptr(
        const std::string&, const std::string&)>;

    udp_simple_factory_t udp_make_broadcast = udp_simple::make_broadcast;
    if (use_dpdk) {
#ifdef HAVE_DPDK
        udp_make_broadcast = [](const std::string& addr, const std::string& port) {
            return dpdk_simple::make_broadcast(addr, port);
        };
#else
        UHD_LOG_WARNING("MPMD",
            "DPDK was requested but is not available, falling back to regular UDP");
#endif
    }
    const size_t echo_prefix_offset = uhd::mpmd::mpmd_impl::MPM_ECHO_CMD.size();
    const size_t mtu_hdr_len        = echo_prefix_offset + 10;
    UHD_ASSERT_THROW(min_frame_size < max_frame_size);
    UHD_ASSERT_THROW(min_frame_size % 4 == 0);
    UHD_ASSERT_THROW(max_frame_size % 4 == 0);
    UHD_ASSERT_THROW(min_frame_size >= echo_prefix_offset + mtu_hdr_len);
    using namespace uhd::transport;
    // The return port will probably differ from the discovery port, so we
    // need a "broadcast" UDP connection; using make_connected() would
    // drop packets
    udp_simple::sptr udp = udp_make_broadcast(address, port);
    std::string send_buf(uhd::mpmd::mpmd_impl::MPM_ECHO_CMD);
    send_buf.resize(max_frame_size, '#');
    UHD_ASSERT_THROW(send_buf.size() == max_frame_size);
    std::vector<uint8_t> recv_buf;
    recv_buf.resize(max_frame_size, ' ');

    // Little helper to check returned packets match the sent ones
    auto require_bufs_match = [&recv_buf, &send_buf, mtu_hdr_len](const size_t len) {
        if (len < mtu_hdr_len
            or std::memcmp((void*)&recv_buf[0], (void*)&send_buf[0], mtu_hdr_len) != 0) {
            throw uhd::runtime_error("Unexpected content of MTU "
                                     "discovery return packet!");
        }
    };
    UHD_LOG_TRACE("MPMD", "Determining UDP MTU... ");
    size_t seq_no = 0;
    while (min_frame_size < max_frame_size) {
        // Only test multiples of 4 bytes!
        const size_t test_frame_size = (max_frame_size / 2 + min_frame_size / 2 + 3)
                                       & ~size_t(3);
        // Encode sequence number and current size in the string, makes it
        // easy to debug in code or Wireshark. Is also used for identifying
        // response packets.
        std::sprintf(
            &send_buf[echo_prefix_offset], ";%04lu,%04lu", seq_no++, test_frame_size);
        UHD_LOG_TRACE("MPMD", "Testing frame size " << test_frame_size);
        udp->send(boost::asio::buffer(&send_buf[0], test_frame_size));

        const size_t len = udp->recv(boost::asio::buffer(recv_buf), echo_timeout);
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
            UHD_LOG_DEBUG("MPMD", "Unexpected packet truncation during MTU discovery.");
            require_bufs_match(len);
            max_frame_size = len;
        }
    }

    min_frame_size = run_mtu_plausibility_check(min_frame_size);
    UHD_LOG_DEBUG("MPMD", "Path MTU for address " << address << ": " << min_frame_size);
    return min_frame_size;
}

} // namespace


/******************************************************************************
 * Structors
 *****************************************************************************/
mpmd_link_if_ctrl_udp::mpmd_link_if_ctrl_udp(const uhd::device_addr_t& mb_args,
    const mpmd_link_if_mgr::xport_info_list_t& xport_info,
    const uhd::rfnoc::chdr_w_t chdr_w)
    : _mb_args(mb_args)
    , _udp_info(get_udp_info_from_xport_info(xport_info))
    , _mtu(MPMD_10GE_DATA_FRAME_MAX_SIZE)
    , _pkt_factory(chdr_w, ENDIANNESS_LITTLE)
{
    const bool use_dpdk =
        mb_args.has_key("use_dpdk"); // FIXME use constrained_device_args
    const std::string mpm_discovery_port = _mb_args.get(
        mpmd_impl::MPM_DISCOVERY_PORT_KEY, std::to_string(mpmd_impl::MPM_DISCOVERY_PORT));
    auto discover_mtu_for_ip = [mpm_discovery_port, use_dpdk](
                                   const std::string& ip_addr, size_t max_frame_size) {
        return discover_mtu(ip_addr,
            mpm_discovery_port,
            IP_PROTOCOL_MIN_MTU_SIZE - IP_PROTOCOL_UDP_PLUS_IP_HEADER,
            max_frame_size,
            MPMD_MTU_DISCOVERY_TIMEOUT,
            use_dpdk);
    };

    const std::vector<std::string> requested_addrs(
        get_addrs_from_mb_args(mb_args, _udp_info));
    for (const auto& ip_addr : requested_addrs) {
        try {
            // If MTU discovery fails, we gracefully recover, but declare that
            // link invalid.
            auto& info = _udp_info.at(ip_addr);
            if (info.link_type == "internal") {
                UHD_LOG_TRACE("MPMD::XPORT::UDP",
                    "MTU for internal interface " << ip_addr << " is "
                                                  << std::to_string(info.if_mtu));
                _mtu = std::min(_mtu, info.if_mtu);
            } else {
                _mtu = std::min(_mtu, discover_mtu_for_ip(ip_addr,
                                    info.link_rate == MAX_RATE_1GIGE ?
                                    MPMD_1GE_DATA_FRAME_MAX_SIZE :
                                    MPMD_10GE_DATA_FRAME_MAX_SIZE));
            }
            _available_addrs.push_back(ip_addr);
        } catch (const uhd::exception& ex) {
            UHD_LOG_WARNING("MPMD::XPORT::UDP",
                "Error during MTU discovery on address " << ip_addr << ": " << ex.what());
        }
    }
}

/******************************************************************************
 * API
 *****************************************************************************/
uhd::transport::both_links_t mpmd_link_if_ctrl_udp::get_link(const size_t link_idx,
    const uhd::transport::link_type_t link_type,
    const uhd::device_addr_t& link_args)
{
    UHD_ASSERT_THROW(link_idx < _available_addrs.size());
    const std::string ip_addr  = _available_addrs.at(link_idx);
    const std::string udp_port = _udp_info.at(ip_addr).udp_port;

    const size_t link_rate = get_link_rate(link_idx);
    const bool enable_fc   = not link_args.has_key("enable_fc")
                           || uhd::cast::from_str<bool>(link_args.get("enable_fc"));
    const bool lossy_xport = enable_fc;
    const bool use_dpdk = _mb_args.has_key("use_dpdk");  // FIXME use constrained device args
    link_params_t default_link_params;
    default_link_params.num_send_frames = MPMD_ETH_NUM_FRAMES;
    default_link_params.num_recv_frames = MPMD_ETH_NUM_FRAMES;
    default_link_params.send_frame_size = (link_rate == MAX_RATE_10GIGE)
                                              ? MPMD_10GE_DATA_FRAME_MAX_SIZE
                                              : (link_rate == MAX_RATE_1GIGE)
                                                    ? MPMD_1GE_DATA_FRAME_MAX_SIZE
                                                    : get_mtu(uhd::TX_DIRECTION);
    default_link_params.recv_frame_size = (link_rate == MAX_RATE_10GIGE)
                                              ? MPMD_10GE_DATA_FRAME_MAX_SIZE
                                              : (link_rate == MAX_RATE_1GIGE)
                                                    ? MPMD_1GE_DATA_FRAME_MAX_SIZE
                                                    : get_mtu(uhd::RX_DIRECTION);
    default_link_params.send_buff_size = get_link_rate(link_idx) * MPMD_BUFFER_DEPTH;
    default_link_params.recv_buff_size = get_link_rate(link_idx) * MPMD_BUFFER_DEPTH;

#ifdef HAVE_DPDK
    if(use_dpdk) {
        default_link_params.num_recv_frames = default_link_params.recv_buff_size /
            default_link_params.recv_frame_size;
    }
#endif

    link_params_t link_params = calculate_udp_link_params(link_type,
        get_mtu(uhd::TX_DIRECTION),
        get_mtu(uhd::RX_DIRECTION),
        default_link_params,
        _mb_args,
        link_args);

    // Enforce a minimum bound of the number of receive and send frames.
    link_params.num_send_frames =
        std::max(uhd::rfnoc::MIN_NUM_FRAMES, link_params.num_send_frames);
    link_params.num_recv_frames =
        std::max(uhd::rfnoc::MIN_NUM_FRAMES, link_params.num_recv_frames);

    if (use_dpdk) {
#ifdef HAVE_DPDK
        auto link = uhd::transport::udp_dpdk_link::make(ip_addr, udp_port, link_params);
        return std::make_tuple(link,
            link_params.send_buff_size,
            link,
            link_params.recv_buff_size,
            lossy_xport,
            true,
            enable_fc);
#else
        UHD_LOG_WARNING("MPMD", "Cannot create DPDK transport, falling back to UDP");
#endif
    }
    auto link = uhd::transport::udp_boost_asio_link::make(ip_addr,
        udp_port,
        link_params,
        link_params.recv_buff_size,
        link_params.send_buff_size);
    return std::make_tuple(link,
        link_params.send_buff_size,
        link,
        link_params.recv_buff_size,
        lossy_xport,
        false,
        enable_fc);
}

size_t mpmd_link_if_ctrl_udp::get_num_links() const
{
    return _available_addrs.size();
}

//! Return the rate of the underlying link in bytes/sec
double mpmd_link_if_ctrl_udp::get_link_rate(const size_t link_idx) const
{
    UHD_ASSERT_THROW(link_idx < get_num_links());
    return _udp_info.at(_available_addrs.at(link_idx)).link_rate;
}

const uhd::rfnoc::chdr::chdr_packet_factory&
mpmd_link_if_ctrl_udp::get_packet_factory() const
{
    return _pkt_factory;
}
