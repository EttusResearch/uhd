//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/utils/log.hpp>
#include <uhdlib/transport/adapter.hpp>
#include <uhdlib/transport/udp_boost_asio_link.hpp>
#include <boost/format.hpp>

using namespace uhd::transport;

namespace asio = boost::asio;

udp_boost_asio_link::udp_boost_asio_link(
    const std::string& addr, const std::string& port, const link_params_t& params)
    : recv_link_base_t(params.num_recv_frames, params.recv_frame_size)
    , send_link_base_t(params.num_send_frames, params.send_frame_size)
    , _recv_memory_pool(buffer_pool::make(params.num_recv_frames, params.recv_frame_size))
    , _send_memory_pool(buffer_pool::make(params.num_send_frames, params.send_frame_size))
{
    for (size_t i = 0; i < params.num_recv_frames; i++) {
        _recv_buffs.push_back(udp_boost_asio_frame_buff(_recv_memory_pool->at(i)));
    }

    for (size_t i = 0; i < params.num_send_frames; i++) {
        _send_buffs.push_back(udp_boost_asio_frame_buff(_send_memory_pool->at(i)));
    }

    for (auto& buff : _recv_buffs) {
        recv_link_base_t::preload_free_buff(&buff);
    }

    for (auto& buff : _send_buffs) {
        send_link_base_t::preload_free_buff(&buff);
    }

    // create, open, and connect the socket
    _socket  = open_udp_socket(addr, port, _io_service);
    _sock_fd = _socket->native_handle();

    auto info   = udp_boost_asio_adapter_info(*_socket);
    auto& ctx   = adapter_ctx::get();
    _adapter_id = ctx.register_adapter(info);

    UHD_LOGGER_TRACE("UDP") << boost::format("Created UDP link to %s:%s") % addr % port;
    UHD_LOGGER_TRACE("UDP") << boost::format("Local UDP socket endpoint: %s:%s")
                                   % get_local_addr() % get_local_port();
}

uint16_t udp_boost_asio_link::get_local_port() const
{
    return _socket->local_endpoint().port();
}

std::string udp_boost_asio_link::get_local_addr() const
{
    return _socket->local_endpoint().address().to_string();
}

size_t udp_boost_asio_link::resize_recv_socket_buffer(size_t num_bytes)
{
    return resize_udp_socket_buffer<asio::socket_base::receive_buffer_size>(
        _socket, num_bytes);
}

size_t udp_boost_asio_link::resize_send_socket_buffer(size_t num_bytes)
{
    return resize_udp_socket_buffer<asio::socket_base::send_buffer_size>(
        _socket, num_bytes);
}

udp_boost_asio_link::sptr udp_boost_asio_link::make(const std::string& addr,
    const std::string& port,
    const link_params_t& params,
    size_t& recv_socket_buff_size,
    size_t& send_socket_buff_size)
{
    UHD_ASSERT_THROW(params.num_recv_frames != 0);
    UHD_ASSERT_THROW(params.num_send_frames != 0);
    UHD_ASSERT_THROW(params.recv_frame_size != 0);
    UHD_ASSERT_THROW(params.send_frame_size != 0);
    UHD_ASSERT_THROW(params.recv_buff_size != 0);
    UHD_ASSERT_THROW(params.send_buff_size != 0);

    udp_boost_asio_link::sptr link(new udp_boost_asio_link(addr, port, params));

    // call the helper to resize send and recv buffers

    recv_socket_buff_size = resize_udp_socket_buffer_with_warning(
        [link](size_t size) { return link->resize_recv_socket_buffer(size); },
        params.recv_buff_size,
        "recv");
    send_socket_buff_size = resize_udp_socket_buffer_with_warning(
        [link](size_t size) { return link->resize_send_socket_buffer(size); },
        params.send_buff_size,
        "send");

    if (recv_socket_buff_size < params.num_recv_frames * MAX_ETHERNET_MTU) {
        UHD_LOG_WARNING("UDP",
            "The current recv_buff_size of "
                << params.recv_buff_size
                << " is less than the minimum recommended size of "
                << params.num_recv_frames * MAX_ETHERNET_MTU
                << " and may result in dropped packets on some NICs");
    }
    if (send_socket_buff_size < params.num_send_frames * MAX_ETHERNET_MTU) {
        UHD_LOG_WARNING("UDP",
            "The current send_buff_size of "
                << params.send_buff_size
                << " is less than the minimum recommended size of "
                << params.num_send_frames * MAX_ETHERNET_MTU
                << " and may result in dropped packets on some NICs");
    }

    return link;
}
