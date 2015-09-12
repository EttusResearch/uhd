/*
 * Copyright 2009-2012,2014 Ettus Research LLC
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
#ifndef INCLUDED_NETWORK_H
#define INCLUDED_NETWORK_H

#include <stdint.h>
#include <stddef.h>

#include <octoclock.h>
#include <net/socket_address.h>
#include <net/eth_mac_addr.h>

#include "octoclock/common.h"

/*
 * Without its own HTON[LS] included in this build,
 * some of lwIP's #defines do nothing.
 * These are substitutions with our own HTON[LS].
 */
#define htons(n) (((((uint16_t)(n) & 0xFF)) << 8) | (((uint16_t)(n) & 0xFF00) >> 8))
#define ntohs(n) htons(n)

#define htonl(n) (((((uint32_t)(n) & 0xFF)) << 24) | \
                  ((((uint32_t)(n) & 0xFF00)) << 8) | \
                  ((((uint32_t)(n) & 0xFF0000)) >> 8) | \
                  ((((uint32_t)(n) & 0xFF000000)) >> 24))

#define ntohl(n) htonl(n)

#define _MAC_ADDR(mac_addr,a,b,c,d,e,f) mac_addr[0] = a; \
                                        mac_addr[1] = b; \
                                        mac_addr[2] = c; \
                                        mac_addr[3] = d; \
                                        mac_addr[4] = e; \
                                        mac_addr[5] = f;

#define _MAC_SET_EQUAL(mac_addr1,mac_addr2) for(uint8_t i = 0; i < 6; i++) mac_addr1[i] = mac_addr2[i];

#define _IP(a,b,c,d) (((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)c << 8) | ((uint32_t)d << 0))
#define _IPH_V(hdr) (ntohs((hdr)->_v_hl_tos) >> 12)
#define _IPH_HL(hdr) ((ntohs((hdr)->_v_hl_tos) >> 8) & 0x0f)
#define _IPH_TOS(hdr) (ntohs((hdr)->_v_hl_tos) & 0xff)
#define _IPH_LEN(hdr) (ntohs((hdr)->_len))
#define _IPH_ID(hdr) (ntohs((hdr)->_id))
#define _IPH_OFFSET(hdr) (ntohs((hdr)->_offset))
#define _IPH_TTL(hdr) (ntohs((hdr)->_ttl_proto) >> 8)
#define _IPH_PROTO(hdr) (ntohs((hdr)->_ttl_proto) & 0xff)
#define _IPH_CHKSUM(hdr) (ntohs((hdr)->_chksum))

#define _IPH_VHLTOS_SET(hdr, v, hl, tos) (hdr)->_v_hl_tos = (htons(((v) << 12) | ((hl) << 8) | (tos)))
#define _IPH_LEN_SET(hdr, len) (hdr)->_len = (htons(len))
#define _IPH_ID_SET(hdr, id) (hdr)->_id = (id)
#define _IPH_OFFSET_SET(hdr, off) (hdr)->_offset = (off)
#define _IPH_TTL_SET(hdr, ttl) (hdr)->_ttl_proto = (htons(_IPH_PROTO(hdr) | ((u16_t)(ttl) << 8)))
#define _IPH_PROTO_SET(hdr, proto) (hdr)->_ttl_proto = (htons((proto) | (_IPH_TTL(hdr) << 8)))
#define _IPH_CHKSUM_SET(hdr, chksum) (hdr)->_chksum = (chksum)

volatile bool using_network_defaults;

// Ethernet I/O buffers
#define ETH_BUF_SIZE 512
uint8_t eth_buf[ETH_BUF_SIZE];

typedef void (*udp_receiver_t)(struct socket_address src, struct socket_address dst,
			       unsigned char *payload, int payload_len);

void init_udp_listeners(void);

void register_addrs(const eth_mac_addr_t *mac_addr, const struct ip_addr *ip_addr);

void register_udp_listener(int port, udp_receiver_t rcvr);

void send_udp_pkt(int src_port, struct socket_address dst,
		  const void *buf, size_t len);

void handle_eth_packet(size_t recv_len);

void network_check(void);

void network_init(void);

#endif /* INCLUDED_NETWORK_H */
