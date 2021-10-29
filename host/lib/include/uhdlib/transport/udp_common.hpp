//
// Copyright 2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/exception.hpp>
#include <uhd/rfnoc/constants.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/transport/links.hpp>
#include <uhdlib/utils/narrow.hpp>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <thread>

namespace uhd { namespace transport {

// Jumbo frames can be up to 9600 bytes;
constexpr size_t MAX_ETHERNET_MTU = 9600;

constexpr size_t UDP_DEFAULT_NUM_FRAMES = 1;

// Based on common 1500 byte MTU for 1GbE
constexpr size_t UDP_DEFAULT_FRAME_SIZE = 1472;

// 20ms of data for 1GbE link (in bytes)
constexpr size_t UDP_DEFAULT_BUFF_SIZE = 2500000;


#if defined(UHD_PLATFORM_MACOS) || defined(UHD_PLATFORM_BSD)
// MacOS limits socket buffer size to 1 Mib
static const size_t MAX_BUFF_SIZE_ETH_MACOS = 0x100000; // 1Mib
#endif

typedef std::shared_ptr<boost::asio::ip::udp::socket> socket_sptr;

/*!
 * Wait for the socket to become ready for a receive operation.
 * \param sock_fd the open socket file descriptor
 * \param timeout_ms the timeout duration in milliseconds
 * \return true when the socket is ready for receive
 */
UHD_INLINE bool wait_for_recv_ready(int sock_fd, int32_t timeout_ms)
{
#ifdef UHD_PLATFORM_WIN32 // select is more portable than poll unfortunately
    // setup timeval for timeout
    timeval tv;
    // If the tv_usec > 1 second on some platforms, select will
    // error EINVAL: An invalid timeout interval was specified.
    tv.tv_sec  = int(timeout_ms / 1000);
    tv.tv_usec = int(timeout_ms * 1000) % 1000000;

    // setup rset for timeout
    fd_set rset;
    FD_ZERO(&rset);
    FD_SET(sock_fd, &rset);

// http://www.gnu.org/s/hello/manual/libc/Interrupted-Primitives.html
// This macro is provided with gcc to properly deal with EINTR.
// If not provided, define an empty macro, assume that is OK
#    ifndef TEMP_FAILURE_RETRY
#        define TEMP_FAILURE_RETRY(x) (x)
#    endif

    // call select with timeout on receive socket
    return TEMP_FAILURE_RETRY(::select(sock_fd + 1, &rset, NULL, NULL, &tv)) > 0;
#else
    pollfd pfd_read;
    pfd_read.fd     = sock_fd;
    pfd_read.events = POLLIN;

    // call poll with timeout on receive socket
    return ::poll(&pfd_read, 1, (int)timeout_ms) > 0;
#endif
}

UHD_INLINE socket_sptr open_udp_socket(
    const std::string& addr, const std::string& port, boost::asio::io_service& io_service)
{
    using udp = boost::asio::ip::udp;

    // resolve the address
    udp::resolver resolver(io_service);
    udp::resolver::query query(udp::v4(), addr, port);
    udp::endpoint receiver_endpoint = *resolver.resolve(query);

    // create, open, and connect the socket
    socket_sptr socket = socket_sptr(new udp::socket(io_service));
    socket->open(udp::v4());
    socket->connect(receiver_endpoint);

    return socket;
}

UHD_INLINE size_t recv_udp_packet(
    int sock_fd, void* mem, size_t frame_size, int32_t timeout_ms)
{
    ssize_t len;

#ifdef MSG_DONTWAIT // try a non-blocking recv() if supported
    len = ::recv(sock_fd, (char*)mem, frame_size, MSG_DONTWAIT);
    if (len > 0) {
        return len;
    }
#endif

    if (wait_for_recv_ready(sock_fd, timeout_ms)) {
        len = uhd::narrow_cast<ssize_t>(::recv(sock_fd, (char*)mem, frame_size, 0));
        if (len == 0) {
            throw uhd::io_error("socket closed");
        }
        if (len < 0) {
            throw uhd::io_error(
                str(boost::format("recv error on socket: %s") % strerror(errno)));
        }
        return len;
    }

    return 0; // timeout
}

UHD_INLINE void send_udp_packet(int sock_fd, void* mem, size_t len)
{
    // Retry logic because send may fail with ENOBUFS.
    // This is known to occur at least on some OSX systems.
    // But it should be safe to always check for the error.
    while (true) {
        const ssize_t ret =
            uhd::narrow_cast<ssize_t>(::send(sock_fd, (const char*)mem, len, 0));
        if (ret == ssize_t(len))
            break;
        if (ret == -1 and errno == ENOBUFS) {
            std::this_thread::sleep_for(std::chrono::microseconds(1));
            continue; // try to send again
        }
        if (ret == -1) {
            throw uhd::io_error(
                str(boost::format("send error on socket: %s") % strerror(errno)));
        }
        UHD_ASSERT_THROW(ret == ssize_t(len));
    }
}

template <typename Opt>
size_t get_udp_socket_buffer_size(socket_sptr socket)
{
    Opt option;
    socket->get_option(option);
    return option.value();
}

template <typename Opt>
size_t resize_udp_socket_buffer(socket_sptr socket, size_t num_bytes)
{
#if defined(UHD_PLATFORM_MACOS) || defined(UHD_PLATFORM_BSD)
    // limit buffer resize on macos or it will error
    num_bytes = std::min(num_bytes, MAX_BUFF_SIZE_ETH_MACOS);
#endif
    Opt option(num_bytes);
    socket->set_option(option);
    return get_udp_socket_buffer_size<Opt>(socket);
}

UHD_INLINE size_t resize_udp_socket_buffer_with_warning(
    std::function<size_t(size_t)> resize_fn,
    const size_t target_size,
    const std::string& name)
{
    size_t actual_size = 0;
    std::string help_message;
#if defined(UHD_PLATFORM_LINUX)
    help_message = str(boost::format("Please run: sudo sysctl -w net.core.%smem_max=%d")
                       % ((name == "recv") ? "r" : "w") % target_size);
#endif /*defined(UHD_PLATFORM_LINUX)*/

    // resize the buffer if size was provided
    if (target_size > 0) {
        actual_size = resize_fn(target_size);

        UHD_LOGGER_TRACE("UDP")
            << boost::format("Target/actual %s sock buff size: %d/%d bytes") % name
                   % target_size % actual_size;
        if (actual_size < target_size)
            UHD_LOGGER_WARNING("UDP")
                << boost::format(
                       "The %s buffer could not be resized sufficiently.\n"
                       "Target sock buff size: %d bytes.\n"
                       "Actual sock buff size: %d bytes.\n"
                       "See the transport application notes on buffer resizing.\n%s")
                       % name % target_size % actual_size % help_message;
    }

    return actual_size;
}

/*!
 * Determines a set of values to use for a UDP CHDR link based on defaults and
 * any overrides that the user may have provided. In cases where both device
 * and stream arguments can be used to override a value, note that the stream
 * argument will always take precedence.
 *
 * \param link_type the link type (CTRL, RX, TX) to calculate parameters for
 * \param send_mtu the MTU of link for Tx cases
 * \param recv_mtu the MTU of link for Rx cases
 * \param default_link_params default values to use for the link parameters
 * \param device_args device-level argument dictionary for overrides
 * \param link_args argument dictionary with stream-level overrides (come from
 *        stream params)
 * \return Parameters to apply
 */
inline link_params_t calculate_udp_link_params(
    const uhd::transport::link_type_t link_type,
    const size_t send_mtu,
    const size_t recv_mtu,
    const link_params_t& default_link_params,
    const uhd::device_addr_t& device_args,
    const uhd::device_addr_t& link_args)
{
    // Apply any device-level overrides to the default values first.
    // If the MTU is overridden, it will be capped to the value provided by
    // the caller.
    const size_t constrained_send_mtu =
        std::min(send_mtu, device_args.cast<size_t>("mtu", send_mtu));
    const size_t constrained_recv_mtu =
        std::min(recv_mtu, device_args.cast<size_t>("mtu", recv_mtu));

    link_params_t link_params;
    link_params.num_send_frames =
        device_args.cast<size_t>("num_send_frames", default_link_params.num_send_frames);
    link_params.num_recv_frames =
        device_args.cast<size_t>("num_recv_frames", default_link_params.num_recv_frames);
    link_params.send_frame_size =
        device_args.cast<size_t>("send_frame_size", default_link_params.send_frame_size);
    link_params.recv_frame_size =
        device_args.cast<size_t>("recv_frame_size", default_link_params.recv_frame_size);
    link_params.send_buff_size =
        device_args.cast<size_t>("send_buff_size", default_link_params.send_buff_size);
    link_params.recv_buff_size =
        device_args.cast<size_t>("recv_buff_size", default_link_params.recv_buff_size);

    // Now apply stream-level overrides based on the link type.
    if (link_type == link_type_t::CTRL) {
        // Control links typically do not allow the number of frames to be
        // configured.
        // Keep the default_link_params value if it is higher to handle
        // high speed DPDK links
        link_params.num_recv_frames =
            std::max(uhd::rfnoc::CMD_FIFO_SIZE / uhd::rfnoc::MAX_CMD_PKT_SIZE,
                link_params.num_recv_frames);
    } else if (link_type == link_type_t::TX_DATA) {
        // Note that the send frame size will be capped to the Tx MTU.
        link_params.send_frame_size = link_args.cast<size_t>("send_frame_size",
            std::min(link_params.send_frame_size, constrained_send_mtu));
        link_params.num_send_frames =
            link_args.cast<size_t>("num_send_frames", link_params.num_send_frames);
        link_params.send_buff_size =
            link_args.cast<size_t>("send_buff_size", link_params.send_buff_size);
    } else if (link_type == link_type_t::RX_DATA) {
        // Note that the receive frame size will be capped to the Rx MTU.
        link_params.recv_frame_size = link_args.cast<size_t>("recv_frame_size",
            std::min(link_params.recv_frame_size, constrained_recv_mtu));
        link_params.num_recv_frames =
            link_args.cast<size_t>("num_recv_frames", link_params.num_recv_frames);
        link_params.recv_buff_size =
            link_args.cast<size_t>("recv_buff_size", link_params.recv_buff_size);
    }

#if defined(UHD_PLATFORM_MACOS) || defined(UHD_PLATFORM_BSD)
    // limit buffer size on OSX to avoid the warning issued by
    // resize_buff_helper
    if (link_params.recv_buff_size > MAX_BUFF_SIZE_ETH_MACOS) {
        link_params.recv_buff_size = MAX_BUFF_SIZE_ETH_MACOS;
    }
    if (link_params.send_buff_size > MAX_BUFF_SIZE_ETH_MACOS) {
        link_params.send_buff_size = MAX_BUFF_SIZE_ETH_MACOS;
    }
#endif

    return link_params;
}


}} // namespace uhd::transport
