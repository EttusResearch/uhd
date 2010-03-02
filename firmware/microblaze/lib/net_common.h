/* -*- c++ -*- */
/*
 * Copyright 2009,2010 Ettus Research LLC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef INCLUDED_NET_COMMON_H
#define INCLUDED_NET_COMMON_H

#include <stdint.h>
#include <stddef.h>
#include <dbsm.h>
#include <net/socket_address.h>
#include <net/eth_mac_addr.h>

#define CPU_TX_BUF 	7	// cpu -> eth

extern int cpu_tx_buf_dest_port;

// If this is non-zero, this dbsm could be writing to the ethernet
extern dbsm_t *ac_could_be_sending_to_eth;

void stop_streaming(void);

/*!
 * Helpful typedefs for callback
 */
typedef void (*udp_receiver_t)(struct socket_address src, struct socket_address dst,
			       unsigned char *payload, int payload_len);

typedef eth_mac_addr_t (*get_eth_mac_addr_t)(void);
typedef struct ip_addr (*get_ip_addr_t)(void);

/*!
 * Functions to register callbacks
 */
void register_get_eth_mac_addr(get_eth_mac_addr_t get_eth_mac_addr);

void register_get_ip_addr(get_ip_addr_t get_ip_addr);

void register_udp_listener(int port, udp_receiver_t rcvr);

void send_udp_pkt(int src_port, struct socket_address dst,
		  const void *buf, size_t len);

void handle_eth_packet(uint32_t *p, size_t nlines);

bool is_udp_packet_with_vrt(uint32_t *p, size_t nlines, int port);

#endif /* INCLUDED_NET_COMMON_H */
