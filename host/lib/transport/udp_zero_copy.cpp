//
// Copyright 2010-2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/transport/buffer_pool.hpp>
#include <uhd/transport/udp_zero_copy.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/transport/udp_common.hpp>
#include <uhdlib/utils/atomic.hpp>
#include <boost/format.hpp>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>

using namespace uhd;
using namespace uhd::transport;
namespace asio = boost::asio;

/***********************************************************************
 * Check registry for correct fast-path setting (windows only)
 **********************************************************************/
#ifdef HAVE_ATLBASE_H
#    define CHECK_REG_SEND_THRESH
#    include <atlbase.h> //CRegKey
static void check_registry_for_fast_send_threshold(const size_t mtu)
{
    static bool warned = false;
    if (warned)
        return; // only allow one printed warning per process

    CRegKey reg_key;
    DWORD threshold = 1024; // system default when threshold is not specified
    if (reg_key.Open(HKEY_LOCAL_MACHINE,
            "System\\CurrentControlSet\\Services\\AFD\\Parameters",
            KEY_READ)
            != ERROR_SUCCESS
        or reg_key.QueryDWORDValue("FastSendDatagramThreshold", threshold)
               != ERROR_SUCCESS
        or threshold < mtu) {
        UHD_LOGGER_WARNING("UDP")
            << boost::format(
                   "The MTU (%d) is larger than the FastSendDatagramThreshold (%d)!\n"
                   "This will negatively affect the transmit performance.\n"
                   "See the transport application notes for more detail.\n")
                   % mtu % threshold;
        warned = true;
    }
    reg_key.Close();
}
#endif /*HAVE_ATLBASE_H*/

/***********************************************************************
 * Reusable managed receiver buffer:
 *  - get_new performs the recv operation
 **********************************************************************/
class udp_zero_copy_asio_mrb : public managed_recv_buffer
{
public:
    udp_zero_copy_asio_mrb(void* mem, int sock_fd, const size_t frame_size)
        : _mem(mem), _sock_fd(sock_fd), _frame_size(frame_size), _len(0)
    { /*NOP*/
    }

    void release(void) override
    {
        _claimer.release();
    }

    UHD_INLINE sptr get_new(const double timeout, size_t& index)
    {
        if (not _claimer.claim_with_wait(timeout))
            return sptr();

        const int32_t timeout_ms = static_cast<int32_t>(timeout * 1000);
        _len = recv_udp_packet(_sock_fd, _mem, _frame_size, timeout_ms);

        if (_len > 0) {
            index++;
            return make(this, _mem, size_t(_len));
        }

        _claimer.release(); // undo claim
        return sptr(); // null for timeout
    }

private:
    void* _mem;
    int _sock_fd;
    size_t _frame_size;
    ssize_t _len;
    simple_claimer _claimer;
};

/***********************************************************************
 * Reusable managed send buffer:
 *  - commit performs the send operation
 **********************************************************************/
class udp_zero_copy_asio_msb : public managed_send_buffer
{
public:
    udp_zero_copy_asio_msb(void* mem, int sock_fd, const size_t frame_size)
        : _mem(mem), _sock_fd(sock_fd), _frame_size(frame_size)
    { /*NOP*/
    }

    void release(void) override
    {
        send_udp_packet(_sock_fd, _mem, size());
        _claimer.release();
    }

    UHD_INLINE sptr get_new(const double timeout, size_t& index)
    {
        if (not _claimer.claim_with_wait(timeout))
            return sptr();
        index++; // advances the caller's buffer
        return make(this, _mem, _frame_size);
    }

private:
    void* _mem;
    int _sock_fd;
    size_t _frame_size;
    simple_claimer _claimer;
};

/***********************************************************************
 * Zero Copy UDP implementation with ASIO:
 *   This is the portable zero copy implementation for systems
 *   where a faster, platform specific solution is not available.
 *   However, it is not a true zero copy implementation as each
 *   send and recv requires a copy operation to/from userspace.
 **********************************************************************/
class udp_zero_copy_asio_impl : public udp_zero_copy
{
public:
    typedef std::shared_ptr<udp_zero_copy_asio_impl> sptr;

    udp_zero_copy_asio_impl(const std::string& addr,
        const std::string& port,
        const zero_copy_xport_params& xport_params)
        : _recv_frame_size(xport_params.recv_frame_size)
        , _num_recv_frames(xport_params.num_recv_frames)
        , _send_frame_size(xport_params.send_frame_size)
        , _num_send_frames(xport_params.num_send_frames)
        , _recv_buffer_pool(buffer_pool::make(
              xport_params.num_recv_frames, xport_params.recv_frame_size))
        , _send_buffer_pool(buffer_pool::make(
              xport_params.num_send_frames, xport_params.send_frame_size))
        , _next_recv_buff_index(0)
        , _next_send_buff_index(0)
    {
        UHD_LOGGER_TRACE("UDP")
            << boost::format("Creating UDP transport to %s:%s") % addr % port;

#ifdef CHECK_REG_SEND_THRESH
        check_registry_for_fast_send_threshold(this->get_send_frame_size());
#endif /*CHECK_REG_SEND_THRESH*/

        _socket  = open_udp_socket(addr, port, _io_service);
        _sock_fd = _socket->native_handle();

        UHD_LOGGER_TRACE("UDP") << boost::format("Local UDP socket endpoint: %s:%s")
                                       % get_local_addr() % get_local_port();

        // allocate re-usable managed receive buffers
        for (size_t i = 0; i < get_num_recv_frames(); i++) {
            _mrb_pool.push_back(std::make_shared<udp_zero_copy_asio_mrb>(
                _recv_buffer_pool->at(i), _sock_fd, get_recv_frame_size()));
        }

        // allocate re-usable managed send buffers
        for (size_t i = 0; i < get_num_send_frames(); i++) {
            _msb_pool.push_back(std::make_shared<udp_zero_copy_asio_msb>(
                _send_buffer_pool->at(i), _sock_fd, get_send_frame_size()));
        }
    }

    size_t resize_send_socket_buffer(size_t num_bytes)
    {
        return resize_udp_socket_buffer<asio::socket_base::send_buffer_size>(
            _socket, num_bytes);
    }

    size_t resize_recv_socket_buffer(size_t num_bytes)
    {
        return resize_udp_socket_buffer<asio::socket_base::receive_buffer_size>(
            _socket, num_bytes);
    }

    /*******************************************************************
     * Receive implementation:
     * Block on the managed buffer's get call and advance the index.
     ******************************************************************/
    managed_recv_buffer::sptr get_recv_buff(double timeout) override
    {
        if (_next_recv_buff_index == _num_recv_frames)
            _next_recv_buff_index = 0;
        return _mrb_pool[_next_recv_buff_index]->get_new(timeout, _next_recv_buff_index);
    }

    size_t get_num_recv_frames(void) const override
    {
        return _num_recv_frames;
    }
    size_t get_recv_frame_size(void) const override
    {
        return _recv_frame_size;
    }

    /*******************************************************************
     * Send implementation:
     * Block on the managed buffer's get call and advance the index.
     ******************************************************************/
    managed_send_buffer::sptr get_send_buff(double timeout) override
    {
        if (_next_send_buff_index == _num_send_frames)
            _next_send_buff_index = 0;
        return _msb_pool[_next_send_buff_index]->get_new(timeout, _next_send_buff_index);
    }

    size_t get_num_send_frames(void) const override
    {
        return _num_send_frames;
    }
    size_t get_send_frame_size(void) const override
    {
        return _send_frame_size;
    }

    uint16_t get_local_port(void) const override
    {
        return _socket->local_endpoint().port();
    }

    std::string get_local_addr(void) const override
    {
        return _socket->local_endpoint().address().to_string();
    }

private:
    // memory management -> buffers and fifos
    const size_t _recv_frame_size, _num_recv_frames;
    const size_t _send_frame_size, _num_send_frames;
    buffer_pool::sptr _recv_buffer_pool, _send_buffer_pool;
    std::vector<std::shared_ptr<udp_zero_copy_asio_msb>> _msb_pool;
    std::vector<std::shared_ptr<udp_zero_copy_asio_mrb>> _mrb_pool;
    size_t _next_recv_buff_index, _next_send_buff_index;

    // asio guts -> socket and service
    asio::io_service _io_service;
    socket_sptr _socket;
    int _sock_fd;
};

/***********************************************************************
 * UDP zero copy make function
 **********************************************************************/
udp_zero_copy::sptr udp_zero_copy::make(const std::string& addr,
    const std::string& port,
    const zero_copy_xport_params& default_buff_args,
    udp_zero_copy::buff_params& buff_params_out,
    const device_addr_t& hints)
{
    // Initialize xport_params
    zero_copy_xport_params xport_params = default_buff_args;

    xport_params.recv_frame_size =
        size_t(hints.cast<double>("recv_frame_size", default_buff_args.recv_frame_size));
    xport_params.num_recv_frames =
        size_t(hints.cast<double>("num_recv_frames", default_buff_args.num_recv_frames));
    xport_params.send_frame_size =
        size_t(hints.cast<double>("send_frame_size", default_buff_args.send_frame_size));
    xport_params.num_send_frames =
        size_t(hints.cast<double>("num_send_frames", default_buff_args.num_send_frames));
    xport_params.recv_buff_size =
        size_t(hints.cast<double>("recv_buff_size", default_buff_args.recv_buff_size));
    xport_params.send_buff_size =
        size_t(hints.cast<double>("send_buff_size", default_buff_args.send_buff_size));

    if (xport_params.num_recv_frames == 0) {
        UHD_LOG_TRACE(
            "UDP", "Default value for num_recv_frames: " << UDP_DEFAULT_NUM_FRAMES);
        xport_params.num_recv_frames = UDP_DEFAULT_NUM_FRAMES;
    }
    if (xport_params.num_send_frames == 0) {
        UHD_LOG_TRACE(
            "UDP", "Default value for no num_send_frames: " << UDP_DEFAULT_NUM_FRAMES);
        xport_params.num_send_frames = UDP_DEFAULT_NUM_FRAMES;
    }
    if (xport_params.recv_frame_size == 0) {
        UHD_LOG_TRACE("UDP",
            "Using default value for  recv_frame_size: " << UDP_DEFAULT_FRAME_SIZE);
        xport_params.recv_frame_size = UDP_DEFAULT_FRAME_SIZE;
    }
    if (xport_params.send_frame_size == 0) {
        UHD_LOG_TRACE(
            "UDP", "Using default value for send_frame_size, " << UDP_DEFAULT_FRAME_SIZE);
        xport_params.send_frame_size = UDP_DEFAULT_FRAME_SIZE;
    }

    UHD_LOG_TRACE("UDP", "send_frame_size: " << xport_params.send_frame_size);
    UHD_LOG_TRACE("UDP", "recv_frame_size: " << xport_params.recv_frame_size);

    if (xport_params.recv_buff_size == 0) {
        UHD_LOG_TRACE("UDP", "Using default value for recv_buff_size");
        xport_params.recv_buff_size = std::max(
            UDP_DEFAULT_BUFF_SIZE, xport_params.num_recv_frames * MAX_ETHERNET_MTU);
        UHD_LOG_TRACE("UDP",
            "Using default value for recv_buff_size" << xport_params.recv_buff_size);
    }
    if (xport_params.send_buff_size == 0) {
        UHD_LOG_TRACE("UDP", "default_buff_args has no send_buff_size");
        xport_params.send_buff_size = std::max(
            UDP_DEFAULT_BUFF_SIZE, xport_params.num_send_frames * MAX_ETHERNET_MTU);
    }

#if defined(UHD_PLATFORM_MACOS) || defined(UHD_PLATFORM_BSD)
    // limit default buffer size on macos to avoid the warning issued by
    // resize_buff_helper
    if (not hints.has_key("recv_buff_size")
        and xport_params.recv_buff_size > MAX_BUFF_SIZE_ETH_MACOS) {
        xport_params.recv_buff_size = MAX_BUFF_SIZE_ETH_MACOS;
    }
    if (not hints.has_key("send_buff_size")
        and xport_params.send_buff_size > MAX_BUFF_SIZE_ETH_MACOS) {
        xport_params.send_buff_size = MAX_BUFF_SIZE_ETH_MACOS;
    }
#endif

    udp_zero_copy_asio_impl::sptr udp_trans(
        new udp_zero_copy_asio_impl(addr, port, xport_params));

    // call the helper to resize send and recv buffers
    buff_params_out.recv_buff_size = resize_udp_socket_buffer_with_warning(
        [udp_trans](size_t size) { return udp_trans->resize_recv_socket_buffer(size); },
        xport_params.recv_buff_size,
        "recv");
    buff_params_out.send_buff_size = resize_udp_socket_buffer_with_warning(
        [udp_trans](size_t size) { return udp_trans->resize_send_socket_buffer(size); },
        xport_params.send_buff_size,
        "send");

    if (buff_params_out.recv_buff_size
        < xport_params.num_recv_frames * MAX_ETHERNET_MTU) {
        UHD_LOG_WARNING("UDP",
            "The current recv_buff_size of "
                << xport_params.recv_buff_size
                << " is less than the minimum recommended size of "
                << xport_params.num_recv_frames * MAX_ETHERNET_MTU
                << " and may result in dropped packets on some NICs");
    }
    if (buff_params_out.send_buff_size
        < xport_params.num_send_frames * MAX_ETHERNET_MTU) {
        UHD_LOG_WARNING("UDP",
            "The current send_buff_size of "
                << xport_params.send_buff_size
                << " is less than the minimum recommended size of "
                << xport_params.num_send_frames * MAX_ETHERNET_MTU
                << " and may result in dropped packets on some NICs");
    }

    return udp_trans;
}
