//
// Copyright 2019 Ettus Research, a National Instruments brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhdlib/transport/dpdk/common.hpp>
#include <uhdlib/transport/dpdk_io_service.hpp>
#include <uhdlib/transport/udp_dpdk_link.hpp>
#include <uhdlib/usrp/common/io_service_mgr.hpp>

namespace uhd { namespace usrp {

class dpdk_io_service_mgr_impl : public io_service_mgr
{
public:
    dpdk_io_service_mgr_impl()
    {
        _dpdk_ctx = transport::dpdk::dpdk_ctx::get();
    }

    transport::io_service::sptr connect_links(transport::recv_link_if::sptr recv_link,
        transport::send_link_if::sptr send_link,
        const transport::link_type_t /*link_type*/,
        const io_service_args_t& /*default_args*/,
        const uhd::device_addr_t& /*stream_args*/,
        const std::string& /*streamer_id*/)
    {
        auto io_srv = _get_io_service(recv_link, send_link);
        io_srv->attach_recv_link(recv_link);
        io_srv->attach_send_link(send_link);
        return io_srv;
    }

    void disconnect_links(
        transport::recv_link_if::sptr recv_link, transport::send_link_if::sptr send_link)
    {
        auto io_srv = _get_io_service(recv_link, send_link);
        io_srv->detach_recv_link(recv_link);
        io_srv->detach_send_link(send_link);
    }

private:
    transport::io_service::sptr _get_io_service(
        transport::recv_link_if::sptr recv_link, transport::send_link_if::sptr send_link)
    {
        using namespace transport::dpdk;

        // For DPDK, the same object implements the send and recv link
        // interfaces so we should always have both parameters.
        UHD_ASSERT_THROW(recv_link && send_link);

        auto link = std::dynamic_pointer_cast<transport::udp_dpdk_link>(recv_link);
        port_id_t port_id = link->get_port()->get_port_id();

        auto io_srv = _dpdk_ctx->get_io_service(port_id);
        UHD_ASSERT_THROW(io_srv);
        return io_srv;
    }

    transport::dpdk::dpdk_ctx::sptr _dpdk_ctx;
};

}} // namespace uhd::usrp
