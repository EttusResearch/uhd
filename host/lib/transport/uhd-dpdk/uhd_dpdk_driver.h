//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#ifndef _UHD_DPDK_DRIVER_H_
#define _UHD_DPDK_DRIVER_H_

#include "uhd_dpdk_ctx.h"
#include <rte_mbuf.h>
#include <rte_arp.h>
#include <rte_udp.h>
#include <rte_ip.h>

static inline bool is_broadcast(struct uhd_dpdk_port *port, uint32_t dst_ipv4_addr)
{
    uint32_t network = port->netmask | ((~port->netmask) & dst_ipv4_addr);
    return (network == 0xffffffff);
}


int _uhd_dpdk_process_arp(struct uhd_dpdk_port *port, struct arp_hdr *arp_frame);
int _uhd_dpdk_process_udp(struct uhd_dpdk_port *port, struct rte_mbuf *mbuf,
                          struct udp_hdr *pkt, bool bcast);
int _uhd_dpdk_process_ipv4(struct uhd_dpdk_port *port, struct rte_mbuf *mbuf, struct ipv4_hdr *pkt);
int _uhd_dpdk_send_udp(struct uhd_dpdk_port *port,
                       struct uhd_dpdk_socket *sock,
                       struct rte_mbuf *mbuf);
int _uhd_dpdk_arp_request(struct uhd_dpdk_port *port,
                          uint32_t ip);

int _uhd_dpdk_driver_main(void *arg);
#endif /* _UHD_DPDK_DRIVER_H_ */
