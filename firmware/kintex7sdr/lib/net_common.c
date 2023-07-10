/*
 * Copyright 2009-2012 Ettus Research LLC
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "net_common.h"
#include "banal.h"
#include <hal_io.h>
#include <memory_map.h>
#include <memcpy_wa.h>
#include <ethernet.h>
#include <net/padded_eth_hdr.h>
#include <lwip/ip.h>
#include <lwip/udp.h>
#include <lwip/icmp.h>
#include <stdlib.h>
#include <nonstdio.h>
#include "arp_cache.h"
#include "if_arp.h"
#include <ethertype.h>
#include <string.h>
#include "pkt_ctrl.h"

/***********************************************************************
 * Constants + Globals
 **********************************************************************/
static const bool debug = false;
static const size_t out_buff_size = 2048;
static const eth_mac_addr_t BCAST_MAC_ADDR = {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff}};
#define MAX_UDP_LISTENERS 10

/***********************************************************************
 * 16-bit one's complement sum
 **********************************************************************/
static uint32_t chksum_buffer(
    uint16_t *buf, size_t nshorts,
    uint32_t initial_chksum
){
    uint32_t chksum = initial_chksum;
    for (size_t i = 0; i < nshorts; i++) chksum += buf[i];

    while (chksum >> 16) chksum = (chksum & 0xffff) + (chksum >> 16);

    return chksum;
}

/***********************************************************************
 * Listener registry
 **********************************************************************/
static eth_mac_addr_t _local_mac_addr;
static struct ip_addr _local_ip_addr;
void register_addrs(const eth_mac_addr_t *mac_addr, const struct ip_addr *ip_addr){
    _local_mac_addr = *mac_addr;
    _local_ip_addr = *ip_addr;
}

struct listener_entry {
  unsigned short	port;
  udp_receiver_t	rcvr;
};

static struct listener_entry listeners[MAX_UDP_LISTENERS];

void init_udp_listeners(void){
    for (int i = 0; i < MAX_UDP_LISTENERS; i++){
        listeners[i].rcvr = NULL;
    }
}

static struct listener_entry *
find_listener_by_port(unsigned short port)
{
  for (int i = 0; i < MAX_UDP_LISTENERS; i++){
    if (port == listeners[i].port)
      return &listeners[i];
  }
  return 0;
}

static struct listener_entry *
find_free_listener(void)
{
  for (int i = 0; i < MAX_UDP_LISTENERS; i++){
    if (listeners[i].rcvr == NULL)
      return &listeners[i];
  }
  abort();
}

void
register_udp_listener(int port, udp_receiver_t rcvr)
{
  struct listener_entry *lx = find_listener_by_port(port);
  if (lx)
    lx->rcvr = rcvr;
  else {
    lx = find_free_listener();
    lx->port = port;
    lx->rcvr = rcvr;
  }
}

/***********************************************************************
 * Protocol framer
 **********************************************************************/
void setup_framer(
    eth_mac_addr_t eth_dst,
    eth_mac_addr_t eth_src,
    struct socket_address sock_dst,
    struct socket_address sock_src,
    size_t which
){
    struct {
        padded_eth_hdr_t eth;
        struct ip_hdr ip;
        struct udp_hdr udp;
    } frame;

    //-- load Ethernet header --//
    frame.eth.dst = eth_dst;
    frame.eth.src = eth_src;
    frame.eth.ethertype = ETHERTYPE_IPV4;

    //-- load IPv4 header --//
    IPH_VHLTOS_SET(&frame.ip, 4, 5, 0);
    IPH_LEN_SET(&frame.ip, 0);
    IPH_ID_SET(&frame.ip, 0);
    IPH_OFFSET_SET(&frame.ip, IP_DF); // don't fragment
    IPH_TTL_SET(&frame.ip, 32);
    IPH_PROTO_SET(&frame.ip, IP_PROTO_UDP);
    IPH_CHKSUM_SET(&frame.ip, 0);
    frame.ip.src = sock_src.addr;
    frame.ip.dest = sock_dst.addr;
    IPH_CHKSUM_SET(&frame.ip, chksum_buffer(
        (unsigned short *) &frame.ip,
        sizeof(frame.ip)/sizeof(short), 0
    ));

    //-- load UDP header --//
    frame.udp.src = sock_src.port;
    frame.udp.dest = sock_dst.port;
    frame.udp.len = 0;
    frame.udp.chksum = 0;

    //copy into the framer table registers
    memcpy_wa((void *)(sr_proto_framer_regs->table[which].entry + 1), &frame, sizeof(frame));
}

/***********************************************************************
 * Slow-path packet framing and transmission
 **********************************************************************/
/*!
 * low level routine to assembly an ethernet frame and send it.
 *
 * \param dst destination mac address
 * \param ethertype ethertype field
 * \param buf0 first part of data
 * \param len0 length of first part of data
 * \param buf1 second part of data
 * \param len1 length of second part of data
 * \param buf2 third part of data
 * \param len2 length of third part of data
 */
static void
send_pkt(
    eth_mac_addr_t dst, int ethertype,
    const void *buf0, size_t len0,
    const void *buf1, size_t len1,
    const void *buf2, size_t len2
){

    //control word for framed data
    uint32_t ctrl_word = 0x0;

    //assemble the ethernet header
    padded_eth_hdr_t ehdr;
    ehdr.pad = 0;
    ehdr.dst = dst;
    ehdr.src = _local_mac_addr;
    ehdr.ethertype = ethertype;

    //grab an out buffer and pointer
    uint8_t *buff = (uint8_t *)pkt_ctrl_claim_outgoing_buffer();
    uint8_t *p = buff;
    size_t total_len = 0;

    //create a list of all buffers to copy
    const void *buffs[] = {&ctrl_word, &ehdr, buf0, buf1, buf2};
    size_t lens[] = {sizeof(ctrl_word), sizeof(ehdr), len0, len1, (len2 + 3) & ~3};

    //copy each buffer into the out buffer
    for (size_t i = 0; i < sizeof(buffs)/sizeof(buffs[0]); i++){
        total_len += lens[i]; //use full length (not clipped)
        size_t bytes_remaining = out_buff_size - (size_t)(p - buff);
        if (lens[i] > bytes_remaining) lens[i] = bytes_remaining;
        if (lens[i] && ((lens[i] & 0x3) || (intptr_t) buffs[i] & 0x3))
            printf("send_pkt: bad alignment of len and/or buf\n");
        memcpy_wa(p, buffs[i], lens[i]);
        p += lens[i];
    }

    //ensure that minimum length requirements are met
    if (total_len < 64) total_len = 64; //60 + ctrl word

    pkt_ctrl_commit_outgoing_buffer(total_len/sizeof(uint32_t));
    if (debug) printf("sent %d bytes\n", (int)total_len);
}

void
send_ip_pkt(struct ip_addr dst, int protocol,
	    const void *buf0, size_t len0,
	    const void *buf1, size_t len1)
{
  struct ip_hdr ip;
  IPH_VHLTOS_SET(&ip, 4, 5, 0);
  IPH_LEN_SET(&ip, IP_HLEN + len0 + len1);
  IPH_ID_SET(&ip, 0);
  IPH_OFFSET_SET(&ip, IP_DF);	/* don't fragment */
  IPH_TTL_SET(&ip, 32);
  IPH_PROTO_SET(&ip, protocol);
  IPH_CHKSUM_SET(&ip, 0);
  ip.src = _local_ip_addr;
  ip.dest = dst;

  IPH_CHKSUM_SET(&ip, ~chksum_buffer(
    (unsigned short *) &ip, sizeof(ip)/sizeof(short), 0
  ));

  eth_mac_addr_t dst_mac;
  bool found = arp_cache_lookup_mac(&ip.dest, &dst_mac);
  if (!found){
    printf("net_common: failed to hit cache looking for ");
    print_ip_addr(&ip.dest);
    newline();
    return;
  }

  send_pkt(dst_mac, ETHERTYPE_IPV4,
	   &ip, sizeof(ip), buf0, len0, buf1, len1);
}

void 
send_udp_pkt(int src_port, struct socket_address dst,
	     const void *buf, size_t len)
{
  struct udp_hdr udp _AL4;
  udp.src = src_port;
  udp.dest = dst.port;
  udp.len = UDP_HLEN + len;
  udp.chksum = 0;

  send_ip_pkt(dst.addr, IP_PROTO_UDP,
	      &udp, sizeof(udp), buf, len);
}

static void
handle_udp_packet(struct ip_addr src_ip, struct ip_addr dst_ip,
		  struct udp_hdr *udp, size_t len)
{
  if (len != udp->len){
    printf("UDP inconsistent lengths: %d %d\n", (int)len, udp->len);
    return;
  }

  unsigned char *payload = ((unsigned char *) udp) + UDP_HLEN;
  int payload_len = len - UDP_HLEN;

  if (0){
    printf("\nUDP: src = %d  dst = %d  len = %d\n",
	   udp->src, udp->dest, udp->len);

    //print_bytes(0, payload, payload_len);
  }

  struct listener_entry *lx = find_listener_by_port(udp->dest);
  if (lx){
    struct socket_address src = make_socket_address(src_ip, udp->src);
    struct socket_address dst = make_socket_address(dst_ip, udp->dest);
    lx->rcvr(src, dst, payload, payload_len);
  }
}

static void
handle_icmp_packet(struct ip_addr src, struct ip_addr dst,
		   struct icmp_echo_hdr *icmp, size_t len)
{
  switch (icmp->type){
  case ICMP_DUR:	// Destinatino Unreachable
    if (icmp->code == ICMP_DUR_PORT){	// port unreachable
      //handle destination port unreachable (the host ctrl+c'd the app):

      //filter out non udp data response
      struct ip_hdr *ip = (struct ip_hdr *)(((uint8_t*)icmp) + sizeof(struct icmp_echo_hdr));
      struct udp_hdr *udp = (struct udp_hdr *)(((char *)ip) + IP_HLEN);
      if (IPH_PROTO(ip) != IP_PROTO_UDP) break;

      struct listener_entry *lx = find_listener_by_port(udp->src);
      if (lx){
        struct socket_address src = make_socket_address(ip->src, udp->src);
        struct socket_address dst = make_socket_address(ip->dest, udp->dest);
        lx->rcvr(src, dst, NULL, 0);
      }

      putchar('i');
    }
    else {
      //printf("icmp dst unr (code: %d)", icmp->code);
      putchar('i');
    }
    break;

  case ICMP_ECHO:{
    const void *icmp_data_buff = ((uint8_t*)icmp) + sizeof(struct icmp_echo_hdr);
    size_t icmp_data_len = len - sizeof(struct icmp_echo_hdr);

    struct icmp_echo_hdr echo_reply;
    echo_reply.type = 0;
    echo_reply.code = 0;
    echo_reply.chksum = 0;
    echo_reply.id = icmp->id;
    echo_reply.seqno = icmp->seqno;
    echo_reply.chksum = ~chksum_buffer( //data checksum
        (unsigned short *)icmp_data_buff,
        icmp_data_len/sizeof(short),
        chksum_buffer(                  //header checksum
            (unsigned short *)&echo_reply,
            sizeof(echo_reply)/sizeof(short),
        0)
    );

    send_ip_pkt(
        src, IP_PROTO_ICMP,
        &echo_reply, sizeof(echo_reply),
        icmp_data_buff, icmp_data_len
    );
    break;
  }
  default:
    break;
  }
}

static void
send_arp_reply(struct arp_eth_ipv4 *req, eth_mac_addr_t our_mac)
{
  struct arp_eth_ipv4 reply _AL4;
  reply.ar_hrd = req->ar_hrd;
  reply.ar_pro = req->ar_pro;
  reply.ar_hln = req->ar_hln;
  reply.ar_pln = req->ar_pln;
  reply.ar_op = ARPOP_REPLY;
  memcpy(reply.ar_sha, &our_mac, 6);
  memcpy(reply.ar_sip, req->ar_tip, 4);
  memcpy(reply.ar_tha, req->ar_sha, 6);
  memcpy(reply.ar_tip, req->ar_sip, 4);

  eth_mac_addr_t t;
  memcpy(t.addr, reply.ar_tha, 6);
  send_pkt(t, ETHERTYPE_ARP, &reply, sizeof(reply), 0, 0, 0, 0);
}

void net_common_send_arp_request(const struct ip_addr *addr){
    struct arp_eth_ipv4 req _AL4;
    req.ar_hrd = ARPHRD_ETHER;
    req.ar_pro = ETHERTYPE_IPV4;
    req.ar_hln = sizeof(eth_mac_addr_t);
    req.ar_pln = sizeof(struct ip_addr);
    req.ar_op = ARPOP_REQUEST;
    memcpy(req.ar_sha, ethernet_mac_addr(), sizeof(eth_mac_addr_t));
    memcpy(req.ar_sip, get_ip_addr(), sizeof(struct ip_addr));
    memset(req.ar_tha, 0x00, sizeof(eth_mac_addr_t));
    memcpy(req.ar_tip, addr, sizeof(struct ip_addr));

    //send the request with a broadcast ethernet mac address
    send_pkt(BCAST_MAC_ADDR, ETHERTYPE_ARP, &req, sizeof(req), 0, 0, 0, 0);
}

void send_gratuitous_arp(void){
  struct arp_eth_ipv4 req _AL4;
  req.ar_hrd = ARPHRD_ETHER;
  req.ar_pro = ETHERTYPE_IPV4;
  req.ar_hln = sizeof(eth_mac_addr_t);
  req.ar_pln = sizeof(struct ip_addr);
  req.ar_op = ARPOP_REQUEST;
  memcpy(req.ar_sha, ethernet_mac_addr(), sizeof(eth_mac_addr_t));
  memcpy(req.ar_sip, get_ip_addr(),       sizeof(struct ip_addr));
  memset(req.ar_tha, 0x00,                sizeof(eth_mac_addr_t));
  memcpy(req.ar_tip, get_ip_addr(),       sizeof(struct ip_addr));

  //send the request with a broadcast ethernet mac address
  send_pkt(BCAST_MAC_ADDR, ETHERTYPE_ARP, &req, sizeof(req), 0, 0, 0, 0);
}

static void
handle_arp_packet(struct arp_eth_ipv4 *p, size_t size)
{
  if (size < sizeof(struct arp_eth_ipv4)){
    printf("\nhandle_arp: weird size = %d\n", (int)size);
    return;
  }

  if (0){
    printf("ar_hrd = %d\n", p->ar_hrd);
    printf("ar_pro = %d\n", p->ar_pro);
    printf("ar_hln = %d\n", p->ar_hln);
    printf("ar_pln = %d\n", p->ar_pln);
    printf("ar_op  = %d\n", p->ar_op);
    printf("ar_sha = "); print_mac_addr(p->ar_sha); newline();
    printf("ar_sip = "); print_ip_addr (p->ar_sip); newline();
    printf("ar_tha = "); print_mac_addr(p->ar_tha); newline();
    printf("ar_tip = "); print_ip_addr (p->ar_tip); newline();
  }

  if (p->ar_hrd != ARPHRD_ETHER
      || p->ar_pro != ETHERTYPE_IPV4
      || p->ar_hln != 6
      || p->ar_pln != 4)
    return;

  if (p->ar_op == ARPOP_REPLY){
    struct ip_addr ip_addr;
    memcpy(&ip_addr, p->ar_sip, sizeof(ip_addr));
    eth_mac_addr_t mac_addr;
    memcpy(&mac_addr, p->ar_sha, sizeof(mac_addr));
    arp_cache_update(&ip_addr, &mac_addr);
  }

  if (p->ar_op != ARPOP_REQUEST)
    return;

  struct ip_addr sip;
  struct ip_addr tip;

  sip.addr = get_int32(p->ar_sip);
  tip.addr = get_int32(p->ar_tip);

  if (memcmp(&tip, &_local_ip_addr, sizeof(_local_ip_addr)) == 0){	// They're looking for us...
    send_arp_reply(p, _local_mac_addr);
  }
}

void
handle_eth_packet(uint32_t *p, size_t nlines)
{
  static size_t bcount = 0;
  if (debug) printf("===> %d\n", (int)bcount++);
  if (debug) print_buffer(p, nlines);

  padded_eth_hdr_t *eth_hdr = (padded_eth_hdr_t *)p;

  if (eth_hdr->ethertype == ETHERTYPE_ARP){
    struct arp_eth_ipv4 *arp = (struct arp_eth_ipv4 *)(p + 4);
    handle_arp_packet(arp, nlines*sizeof(uint32_t) - 14);
  }
  else if (eth_hdr->ethertype == ETHERTYPE_IPV4){
    struct ip_hdr *ip = (struct ip_hdr *)(p + 4);
    if (IPH_V(ip) != 4 || IPH_HL(ip) != 5)	// ignore pkts w/ bad version or options
      return;

    if (IPH_OFFSET(ip) & (IP_MF | IP_OFFMASK))	// ignore fragmented packets
      return;

    // filter on dest ip addr (should be broadcast or for us)
    bool is_bcast = memcmp(&eth_hdr->dst, &BCAST_MAC_ADDR, sizeof(BCAST_MAC_ADDR)) == 0;
    bool is_my_ip = memcmp(&ip->dest, &_local_ip_addr, sizeof(_local_ip_addr)) == 0;
    if (!is_bcast && !is_my_ip) return;

    arp_cache_update(&ip->src, (eth_mac_addr_t *)(((char *)p)+8));

    int protocol = IPH_PROTO(ip);
    int len = IPH_LEN(ip) - IP_HLEN;

    switch (protocol){
    case IP_PROTO_UDP:
      handle_udp_packet(ip->src, ip->dest, (struct udp_hdr *)(((char *)ip) + IP_HLEN), len);
      break;

    case IP_PROTO_ICMP:
      handle_icmp_packet(ip->src, ip->dest, (struct icmp_echo_hdr *)(((char *)ip) + IP_HLEN), len);
      break;

    default:	// ignore
      break;
    }
  }
  else
    return;	// Not ARP or IPV4, ignore
}
