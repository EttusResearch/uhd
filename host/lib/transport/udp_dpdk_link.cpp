//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/config.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/static.hpp>
#include <uhdlib/transport/adapter.hpp>
#include <uhdlib/transport/dpdk/udp.hpp>
#include <uhdlib/transport/udp_dpdk_link.hpp>
#include <arpa/inet.h>
#include <memory>

// The DPDK function rte_mbuf_to_priv() is experimental and generates a warning.
// This pragma disables that warning since it is expected
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

using namespace uhd::transport;
using namespace uhd::transport::dpdk;

udp_dpdk_link::udp_dpdk_link(dpdk::port_id_t port_id,
    const std::string& remote_addr,
    const std::string& remote_port,
    const std::string& local_port,
    const link_params_t& params)
    : _num_recv_frames(params.num_recv_frames)
    , _recv_frame_size(params.recv_frame_size)
    , _num_send_frames(params.num_send_frames)
    , _send_frame_size(params.send_frame_size)
{
    // Get a reference to the context, since this class manages DPDK memory
    _ctx = dpdk_ctx::get();
    UHD_ASSERT_THROW(_ctx);

    // Fill in remote IPv4 address and UDP port
    // NOTE: Remote MAC address is filled in later by I/O service
    int status = inet_pton(AF_INET, remote_addr.c_str(), &_remote_ipv4);
    if (status != 1) {
        UHD_LOG_ERROR("DPDK", std::string("Invalid destination address ") + remote_addr);
        throw uhd::runtime_error(
            std::string("DPDK: Invalid destination address ") + remote_addr);
    }
    _remote_port = rte_cpu_to_be_16(std::stoul(remote_port));

    // Grab the port with a route to the remote host
    _port = _ctx->get_port(port_id);

    uint16_t local_port_num = rte_cpu_to_be_16(std::stoul(local_port));
    // Get an unused UDP port for listening
    _local_port = _port->alloc_udp_port(local_port_num);

    // Validate params
    const size_t max_frame_size = _port->get_mtu() - dpdk::HDR_SIZE_UDP_IPV4;
    if (params.send_frame_size > max_frame_size ||
        params.recv_frame_size > max_frame_size) {
        UHD_LOGGER_ERROR("DPDK")
            << boost::format("recv_frame_size=%d, send_frame_size=%d, max_frame_size=%d, "
                             "HDR_SIZE_UDP_IPV4=%d")
                   % params.recv_frame_size % params.send_frame_size % max_frame_size
                   % dpdk::HDR_SIZE_UDP_IPV4;
        throw uhd::assertion_error(
            "{ send_frame_size, recv_frame_size } > max_frame_size");
    }

    // Register the adapter
    auto info      = _port->get_adapter_info();
    auto& adap_ctx = adapter_ctx::get();
    _adapter_id    = adap_ctx.register_adapter(info);
    UHD_LOGGER_TRACE("DPDK") << boost::format("Created udp_dpdk_link to (%s:%s)")
                                    % remote_addr % remote_port;
    UHD_LOGGER_TRACE("DPDK")
        << boost::format("num_recv_frames=%d, recv_frame_size=%d, num_send_frames=%d, "
                         "send_frame_size=%d")
               % params.num_recv_frames % params.recv_frame_size % params.num_send_frames
               % params.send_frame_size;
}

udp_dpdk_link::~udp_dpdk_link() {}

udp_dpdk_link::sptr udp_dpdk_link::make(const std::string& remote_addr,
    const std::string& remote_port,
    const link_params_t& params)
{
    auto ctx  = dpdk::dpdk_ctx::get();
    auto port = ctx->get_route(remote_addr);
    if (!port) {
        UHD_LOG_ERROR("DPDK",
            std::string("Could not find route to destination address ") + remote_addr);
        throw uhd::runtime_error(
            std::string("DPDK: Could not find route to destination address ")
            + remote_addr);
    }
    return make(port->get_port_id(), remote_addr, remote_port, "0", params);
}

udp_dpdk_link::sptr udp_dpdk_link::make(const dpdk::port_id_t port_id,
    const std::string& remote_addr,
    const std::string& remote_port,
    const std::string& local_port,
    const link_params_t& params)
{
    UHD_ASSERT_THROW(params.recv_frame_size > 0);
    UHD_ASSERT_THROW(params.send_frame_size > 0);
    UHD_ASSERT_THROW(params.num_send_frames > 0);
    UHD_ASSERT_THROW(params.num_recv_frames > 0);

    return std::make_shared<udp_dpdk_link>(
        port_id, remote_addr, remote_port, local_port, params);
}

void udp_dpdk_link::enqueue_recv_mbuf(struct rte_mbuf* mbuf)
{
    // Get packet size
    struct rte_udp_hdr* hdr = rte_pktmbuf_mtod_offset(
        mbuf, struct rte_udp_hdr*, sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr));
    size_t packet_size = rte_be_to_cpu_16(hdr->dgram_len) - sizeof(struct rte_udp_hdr);
    // Prepare the dpdk_frame_buff
    auto buff = new (rte_mbuf_to_priv(mbuf)) dpdk_frame_buff(mbuf);
    buff->header_jump(
        sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_udp_hdr));
    buff->set_packet_size(packet_size);
    // Add the dpdk_frame_buff to the list
    if (_recv_buff_head) {
        buff->prev                  = _recv_buff_head->prev;
        buff->next                  = _recv_buff_head;
        _recv_buff_head->prev->next = buff;
        _recv_buff_head->prev       = buff;
    } else {
        _recv_buff_head = buff;
        buff->next      = buff;
        buff->prev      = buff;
    }
}

frame_buff::uptr udp_dpdk_link::get_recv_buff(int32_t /*timeout_ms*/)
{
    auto buff = _recv_buff_head;
    if (buff) {
        if (_recv_buff_head->next == buff) {
            /* Only had the one buff, so the list is empty */
            _recv_buff_head = nullptr;
        } else {
            /* Make the next buff the new list head */
            _recv_buff_head->next->prev = _recv_buff_head->prev;
            _recv_buff_head->prev->next = _recv_buff_head->next;
            _recv_buff_head             = _recv_buff_head->next;
        }
        buff->next = nullptr;
        buff->prev = nullptr;
        return frame_buff::uptr(buff);
    }
    return frame_buff::uptr();
}

void udp_dpdk_link::release_recv_buff(frame_buff::uptr buff)
{
    dpdk_frame_buff* buff_ptr = (dpdk_frame_buff*)buff.release();
    assert(buff_ptr);
    rte_pktmbuf_free(buff_ptr->get_pktmbuf());
}

frame_buff::uptr udp_dpdk_link::get_send_buff(int32_t /*timeout_ms*/)
{
    auto mbuf = rte_pktmbuf_alloc(_port->get_tx_pktbuf_pool());
    if (mbuf) {
        auto buff = new (rte_mbuf_to_priv(mbuf)) dpdk_frame_buff(mbuf);
        buff->header_jump(
            sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_udp_hdr));
        return frame_buff::uptr(buff);
    }
    return frame_buff::uptr();
}

void udp_dpdk_link::release_send_buff(frame_buff::uptr buff)
{
    dpdk_frame_buff* buff_ptr = (dpdk_frame_buff*)buff.release();
    assert(buff_ptr);
    auto mbuf = buff_ptr->get_pktmbuf();
    if (buff_ptr->packet_size()) {
        // Fill in L2 header
        auto local_mac           = _port->get_mac_addr();
        struct rte_ether_hdr* l2_hdr = rte_pktmbuf_mtod(mbuf, struct rte_ether_hdr*);
#if RTE_VER_YEAR > 21 || (RTE_VER_YEAR == 21 && RTE_VER_MONTH == 11)
        rte_ether_addr_copy(&_remote_mac, &l2_hdr->dst_addr);
        rte_ether_addr_copy(&local_mac, &l2_hdr->src_addr);
#else
        rte_ether_addr_copy(&_remote_mac, &l2_hdr->d_addr);
        rte_ether_addr_copy(&local_mac, &l2_hdr->s_addr);
#endif
        l2_hdr->ether_type = rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4);
        // Fill in L3 and L4 headers
        dpdk::fill_rte_udp_hdr(mbuf,
            _port,
            _remote_ipv4,
            _local_port,
            _remote_port,
            buff_ptr->packet_size());
        // Prepare the packet buffer and send it out
        int status = rte_eth_tx_prepare(_port->get_port_id(), _queue, &mbuf, 1);
        if (status != 1) {
            throw uhd::runtime_error("DPDK: Failed to prepare TX buffer for send");
        }
        status = rte_eth_tx_burst(_port->get_port_id(), _queue, &mbuf, 1);
        while (status != 1) {
            status = rte_eth_tx_burst(_port->get_port_id(), _queue, &mbuf, 1);
            // FIXME: Should we make available retrying?
            // throw uhd::runtime_error("DPDK: Failed to send TX buffer");
        }
    } else {
        // Release the buffer if there is nothing in it
        rte_pktmbuf_free(mbuf);
    }
}
#pragma GCC diagnostic pop
