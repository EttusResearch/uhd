//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#ifndef _UHD_DPDK_UDP_H_
#define _UHD_DPDK_UDP_H_

#include "uhd_dpdk_ctx.h"
#include <rte_udp.h>

struct uhd_dpdk_udp_priv {
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t dst_ipv4_addr;
    size_t dropped_pkts;
    size_t xferd_pkts;
    bool filter_bcast;
    /* TODO: Cache destination address ptr to avoid ARP table lookup cost? */
    //struct uhd_dpdk_arp_entry *arp_entry;
};

int _uhd_dpdk_udp_setup(struct uhd_dpdk_config_req *req);
int _uhd_dpdk_udp_release(struct uhd_dpdk_config_req *req);

void uhd_dpdk_udp_open(struct uhd_dpdk_config_req *req,
                       struct uhd_dpdk_sockarg_udp *arg);
void uhd_dpdk_udp_close(struct uhd_dpdk_config_req *req);

int uhd_dpdk_udp_prep(struct uhd_dpdk_socket *sock,
                      struct rte_mbuf *mbuf);

/*
 * Get key for RX table corresponding to this socket
 *
 * This is primarily used to get access to the waiter entry
 */
int _uhd_dpdk_udp_rx_key(struct uhd_dpdk_socket *sock,
                         struct uhd_dpdk_ipv4_5tuple *key);
#endif /* _UHD_DPDK_UDP_H_ */
