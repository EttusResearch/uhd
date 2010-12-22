/* -*- c -*- */
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

static const bool debug = false;

static const eth_mac_addr_t BCAST_MAC_ADDR = {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff}};

//used in the top level application...
struct socket_address fp_socket_src, fp_socket_dst;

// ------------------------------------------------------------------------

static eth_mac_addr_t _local_mac_addr;
static struct ip_addr _local_ip_addr;
void register_addrs(const eth_mac_addr_t *mac_addr, const struct ip_addr *ip_addr){
    _local_mac_addr = *mac_addr;
    _local_ip_addr = *ip_addr;
}

//-------------------------------------------------------------------------

#define	MAX_UDP_LISTENERS	6

struct listener_entry {
  unsigned short	port;
  udp_receiver_t	rcvr;
};

static struct listener_entry listeners[MAX_UDP_LISTENERS];

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
    if (listeners[i].rcvr == 0)
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

// ------------------------------------------------------------------------


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
send_pkt(eth_mac_addr_t dst, int ethertype,
	 const void *buf0, size_t len0,
	 const void *buf1, size_t len1,
	 const void *buf2, size_t len2)
{

  // Assemble the header
  padded_eth_hdr_t	ehdr;
  ehdr.pad = 0;
  ehdr.dst = dst;
  ehdr.src = _local_mac_addr;
  ehdr.ethertype = ethertype;

  uint32_t *buff = (uint32_t *)pkt_ctrl_claim_outgoing_buffer();

  // Copy the pieces into the buffer
  uint32_t *p = buff;
  *p++ = 0x0;  				  // slow path
  memcpy_wa(p, &ehdr, sizeof(ehdr));      // 4 lines
  p += sizeof(ehdr)/sizeof(uint32_t);


  // FIXME modify memcpy_wa to do read/modify/write if reqd

  if (len0 && ((len0 & 0x3) || (intptr_t) buf0 & 0x3))
    printf("send_pkt: bad alignment of len0 and/or buf0\n");

  if (len1 && ((len1 & 0x3) || (intptr_t) buf1 & 0x3))
    printf("send_pkt: bad alignment of len1 and/or buf1\n");

  if (len2 && ((len2 & 0x3) || (intptr_t) buf2 & 0x3))
    printf("send_pkt: bad alignment of len2 and/or buf2\n");

  if (len0){
    memcpy_wa(p, buf0, len0);
    p += len0/sizeof(uint32_t);
  }
  if (len1){
    memcpy_wa(p, buf1, len1);
    p += len1/sizeof(uint32_t);
  }
  if (len2){
    memcpy_wa(p, buf2, len2);
    p += len2/sizeof(uint32_t);
  }

  size_t total_len = (p - buff) * sizeof(uint32_t);
  if (total_len < 60)		// ensure that we don't try to send a short packet
    total_len = 60;

  pkt_ctrl_commit_outgoing_buffer(total_len/4);
  if (debug) printf("sent %d bytes\n", (int)total_len);
}

unsigned int CHKSUM(unsigned int x, unsigned int *chksum)
{
  *chksum += x;
  *chksum = (*chksum & 0xffff) + (*chksum>>16);
  *chksum = (*chksum & 0xffff) + (*chksum>>16);
  return x;
}

static unsigned int
chksum_buffer(unsigned short *buf, int nshorts, unsigned int initial_chksum)
{
  unsigned int chksum = initial_chksum;
  for (int i = 0; i < nshorts; i++)
    CHKSUM(buf[i], &chksum);

  return chksum;
}

void
send_ip_pkt(struct ip_addr dst, int protocol,
	    const void *buf0, size_t len0,
	    const void *buf1, size_t len1)
{
  int ttl = 32;

  struct ip_hdr ip;
  IPH_VHLTOS_SET(&ip, 4, 5, 0);
  IPH_LEN_SET(&ip, IP_HLEN + len0 + len1);
  IPH_ID_SET(&ip, 0);
  IPH_OFFSET_SET(&ip, IP_DF);	/* don't fragment */
  ip._ttl_proto = (ttl << 8) | (protocol & 0xff);
  ip._chksum = 0;
  ip.src = _local_ip_addr;
  ip.dest = dst;

  ip._chksum = ~chksum_buffer((unsigned short *) &ip,
			      sizeof(ip)/sizeof(short), 0);

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
      if (IPH_PROTO(ip) != IP_PROTO_UDP || udp->dest != fp_socket_dst.port) return;

      //end async update packets per second
      sr_tx_ctrl->cyc_per_up = 0;

      //the end continuous streaming command
      sr_rx_ctrl->cmd = 1 << 31; //no samples now
      sr_rx_ctrl->time_secs = 0;
      sr_rx_ctrl->time_ticks = 0; //latch the command

      //struct udp_hdr *udp = (struct udp_hdr *)((char *)icmp + 28);
      //printf("icmp port unr %d\n", udp->dest);
      putchar('i');
    }
    else {
      //printf("icmp dst unr (code: %d)", icmp->code);
      putchar('i');
    }
    break;

  case ICMP_ECHO:{
    struct icmp_echo_hdr echo_reply;
    echo_reply.type = 0;
    echo_reply.code = 0;
    echo_reply.chksum = 0;
    echo_reply.id = icmp->id;
    echo_reply.seqno = icmp->seqno;
    echo_reply.chksum = ~chksum_buffer(
        (unsigned short *)&echo_reply,
        sizeof(echo_reply)/sizeof(short),
    0);
    send_ip_pkt(
        src, IP_PROTO_ICMP, &echo_reply, sizeof(echo_reply),
        ((uint8_t*)icmp) + sizeof(struct icmp_echo_hdr),
        len - sizeof(struct icmp_echo_hdr)
    );
    break;
  }
  default:
    break;
  }
}

static void __attribute__((unused))
print_arp_ip(const unsigned char ip[4])
{
  printf("%d.%d.%d.%d", ip[0], ip[1], ip[2],ip[3]);
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
    printf("ar_sip = "); print_arp_ip(p->ar_sip);    newline();
    printf("ar_tha = "); print_mac_addr(p->ar_tha); newline();
    printf("ar_tip = "); print_arp_ip(p->ar_tip);    newline();
  }

  if (p->ar_hrd != ARPHRD_ETHER
      || p->ar_pro != ETHERTYPE_IPV4
      || p->ar_hln != 6
      || p->ar_pln != 4)
    return;
  
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

// ------------------------------------------------------------------------

void
print_ip(struct ip_addr ip)
{
  unsigned int t = ntohl(ip.addr);
  printf("%d.%d.%d.%d",
	 (t >> 24) & 0xff,
	 (t >> 16) & 0xff,
	 (t >>  8) & 0xff,
	 t & 0xff);
}
