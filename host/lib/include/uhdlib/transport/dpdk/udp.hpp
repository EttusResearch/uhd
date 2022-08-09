//
// Copyright 2019 Ettus Research, a National Instruments brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#pragma once

#include <uhdlib/transport/dpdk/common.hpp>
#include <arpa/inet.h>
#include <netinet/udp.h>
#include <rte_ip.h>
#include <rte_udp.h>
#include <boost/format.hpp>

namespace uhd { namespace transport { namespace dpdk {

constexpr size_t HDR_SIZE_UDP_IPV4 =
    (sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_udp_hdr));

/*!
 * An enumerated type representing the type of flow for an IPv4 client
 * Currently, only UDP is supported (FLOW_TYPE_UDP)
 */
enum flow_type {
    FLOW_TYPE_UDP,
    FLOW_TYPE_COUNT,
};

/*!
 * A tuple for IPv4 flows that can be used for hashing
 */
struct ipv4_5tuple
{
    enum flow_type flow_type;
    rte_ipv4_addr src_ip;
    rte_ipv4_addr dst_ip;
    uint16_t src_port;
    uint16_t dst_port;
};

inline void fill_rte_ipv4_hdr(struct rte_mbuf* mbuf,
    const dpdk_port* port,
    uint32_t dst_rte_ipv4_addr,
    uint8_t proto_id,
    uint32_t payload_len)
{
    struct rte_ether_hdr* eth_hdr = rte_pktmbuf_mtod(mbuf, struct rte_ether_hdr*);
    struct rte_ipv4_hdr* ip_hdr   = (struct rte_ipv4_hdr*)&eth_hdr[1];

    ip_hdr->version_ihl     = 0x40 | 5;
    ip_hdr->type_of_service = 0;
    ip_hdr->total_length    = rte_cpu_to_be_16(20 + payload_len);
    ip_hdr->packet_id       = 0;
    ip_hdr->fragment_offset = rte_cpu_to_be_16(RTE_IPV4_HDR_DF_FLAG);
    ip_hdr->time_to_live    = 64;
    ip_hdr->next_proto_id   = proto_id;
    ip_hdr->hdr_checksum    = 0; // Require HW offload
    ip_hdr->src_addr        = port->get_ipv4();
    ip_hdr->dst_addr        = dst_rte_ipv4_addr;

    mbuf->ol_flags = PKT_TX_IP_CKSUM | PKT_TX_IPV4;
    mbuf->l2_len   = sizeof(struct rte_ether_hdr);
    mbuf->l3_len   = sizeof(struct rte_ipv4_hdr);
    mbuf->pkt_len  = sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr) + payload_len;
    mbuf->data_len = sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr) + payload_len;
}

/* All values except payload length must be in network order */
inline void fill_rte_udp_hdr(struct rte_mbuf* mbuf,
    const dpdk_port* port,
    uint32_t dst_rte_ipv4_addr,
    uint16_t src_port,
    uint16_t dst_port,
    uint32_t payload_len)
{
    struct rte_ether_hdr* eth_hdr;
    struct rte_ipv4_hdr* ip_hdr;
    struct rte_udp_hdr* tx_hdr;

    fill_rte_ipv4_hdr(
        mbuf, port, dst_rte_ipv4_addr, IPPROTO_UDP, sizeof(struct rte_udp_hdr) + payload_len);

    eth_hdr = rte_pktmbuf_mtod(mbuf, struct rte_ether_hdr*);
    ip_hdr  = (struct rte_ipv4_hdr*)&eth_hdr[1];
    tx_hdr  = (struct rte_udp_hdr*)&ip_hdr[1];

    tx_hdr->src_port    = src_port;
    tx_hdr->dst_port    = dst_port;
    tx_hdr->dgram_len   = rte_cpu_to_be_16(8 + payload_len);
    tx_hdr->dgram_cksum = 0;
    mbuf->l4_len        = sizeof(struct rte_udp_hdr);
}

//! Return an IPv4 address (numeric, in network order) into a string
inline std::string ipv4_num_to_str(const uint32_t ip_addr)
{
    char addr_str[INET_ADDRSTRLEN];
    struct in_addr rte_ipv4_addr;
    rte_ipv4_addr.s_addr = ip_addr;
    inet_ntop(AF_INET, &rte_ipv4_addr, addr_str, sizeof(addr_str));
    return std::string(addr_str);
}

inline std::string eth_addr_to_string(const struct rte_ether_addr mac_addr)
{
    auto mac_stream = boost::format("%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx");
    mac_stream % (uint32_t)mac_addr.addr_bytes[0] % (uint32_t)mac_addr.addr_bytes[1]
        % (uint32_t)mac_addr.addr_bytes[2] % (uint32_t)mac_addr.addr_bytes[3]
        % (uint32_t)mac_addr.addr_bytes[4] % (uint32_t)mac_addr.addr_bytes[5];
    return mac_stream.str();
}

}}} /* namespace uhd::transport::dpdk */
