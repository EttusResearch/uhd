//
// Copyright 2010-2011,2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/transport/udp_simple.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/transport/udp_common.hpp>
#ifdef _WIN32
#    include <mstcpip.h>
#    include <winsock2.h>
#endif
#include <chrono>

using namespace uhd::transport;
namespace asio = boost::asio;

/***********************************************************************
 * UDP simple implementation: connected and broadcast
 **********************************************************************/
class udp_simple_impl : public udp_simple
{
public:
    udp_simple_impl(
        const std::string& addr, const std::string& port, bool bcast, bool connect)
        : _connected(connect)
    {
        UHD_LOG_TRACE("UDP", "Creating udp transport for " << addr << " " << port);

        // resolve the address
        asio::ip::udp::resolver resolver(_io_context);
        _send_endpoint = *resolver
                              .resolve(asio::ip::udp::v4(),
                                  addr,
                                  port,
                                  asio::ip::resolver_query_base::all_matching)
                              .begin();

        // create and open the socket
        _socket = socket_sptr(new asio::ip::udp::socket(_io_context));
        _socket->open(asio::ip::udp::v4());

#ifdef _WIN32
        // On Linux, ICMP "Port Unreachable" errors are silently dropped on unconnected
        // UDP sockets and only reported on connected sockets (as ECONNREFUSED).
        // Windows reports them as WSAECONNRESET (10054) on ALL UDP sockets by default.
        // Replicate Linux behavior: suppress CONNRESET only for unconnected (broadcast)
        // sockets; leave it enabled for connected sockets so errors are still reported.
        if (!connect) {
            DWORD connreset      = FALSE;
            DWORD bytes_returned = 0;
            int result           = WSAIoctl(_socket->native_handle(),
                SIO_UDP_CONNRESET,
                &connreset,
                sizeof(connreset),
                nullptr,
                0,
                &bytes_returned,
                nullptr,
                nullptr);
            if (result != 0) {
                UHD_LOG_ERROR("UDP",
                    "WSAIoctl(SIO_UDP_CONNRESET) failed with return value: "
                        << result << ", error: " << WSAGetLastError());
            }
        }
#endif

        // allow broadcasting
        _socket->set_option(asio::socket_base::broadcast(bcast));

        // connect the socket
        if (connect)
            _socket->connect(_send_endpoint);
    }

    size_t send(const asio::const_buffer& buff) override
    {
        if (_connected)
            return _socket->send(asio::buffer(buff));
        return _socket->send_to(asio::buffer(buff), _send_endpoint);
    }

    size_t recv(const asio::mutable_buffer& buff, double timeout) override
    {
        const int32_t timeout_ms = static_cast<int32_t>(timeout * 1000);

        if (not wait_for_recv_ready(_socket->native_handle(), timeout_ms))
            return 0;
        return _socket->receive_from(asio::buffer(buff), _recv_endpoint);
    }

    std::string get_recv_addr(void) override
    {
        return _recv_endpoint.address().to_string();
    }

    std::string get_send_addr(void) override
    {
        return _send_endpoint.address().to_string();
    }

private:
    bool _connected;
    asio::io_context _io_context;
    socket_sptr _socket;
    asio::ip::udp::endpoint _send_endpoint;
    asio::ip::udp::endpoint _recv_endpoint;
};

udp_simple::~udp_simple(void)
{
    /* NOP */
}

/***********************************************************************
 * UDP public make functions
 **********************************************************************/
udp_simple::sptr udp_simple::make_connected(
    const std::string& addr, const std::string& port)
{
    return sptr(new udp_simple_impl(addr, port, false, true /* no bcast, connect */));
}

udp_simple::sptr udp_simple::make_broadcast(
    const std::string& addr, const std::string& port)
{
    return sptr(new udp_simple_impl(addr, port, true, false /* bcast, no connect */));
}

/***********************************************************************
 * Simple UART over UDP
 **********************************************************************/
class udp_simple_uart_impl : public uhd::uart_iface
{
public:
    udp_simple_uart_impl(udp_simple::sptr udp)
    {
        _udp = udp;
        _len = 0;
        _off = 0;
        this->write_uart(""); // send an empty packet to init
    }

    void write_uart(const std::string& buf) override
    {
        _udp->send(asio::buffer(buf));
    }

    std::string read_uart(double timeout) override
    {
        std::string line;
        const auto exit_time = std::chrono::steady_clock::now()
                               + std::chrono::milliseconds(int64_t(timeout * 1000));
        do {
            // drain anything in current buffer
            while (_off < _len) {
                const char ch = _buf[_off++];
                _line += ch;
                if (ch == '\n') {
                    line.swap(_line);
                    return line;
                }
            }

            // recv a new packet into the buffer
            _len = _udp->recv(asio::buffer(_buf),
                std::max(std::chrono::duration<double>(
                             exit_time - std::chrono::steady_clock::now())
                             .count(),
                    0.0));
            _off = 0;

        } while (_len != 0);
        return line;
    }

private:
    udp_simple::sptr _udp;
    size_t _len, _off;
    uint8_t _buf[udp_simple::mtu];
    std::string _line;
};

uhd::uart_iface::sptr udp_simple::make_uart(sptr udp)
{
    return uart_iface::sptr(new udp_simple_uart_impl(udp));
}
