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

#include <stdint.h>
#include <string.h>

#include <avr/eeprom.h>
#include <avr/interrupt.h>

#include <lwip/ip.h>
#include <lwip/udp.h>
#include <lwip/icmp.h>

#include <debug.h>
#include <octoclock.h>
#include <state.h>
#include <network.h>

#include <net/enc28j60.h>
#include <net/eth_hdr.h>
#include <net/if_arp.h>
#include <net/ethertype.h>

#include <util/delay.h>

#include "arp_cache.h"

/***********************************************************************
 * Constants + Globals
 **********************************************************************/
static const size_t out_buff_size = ETH_BUF_SIZE;
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

struct listener_entry {
    unsigned short	port;
    udp_receiver_t	rcvr;
};

static struct listener_entry listeners[MAX_UDP_LISTENERS];

void init_udp_listeners(void){
    for (int i = 0; i < MAX_UDP_LISTENERS; i++)
        listeners[i].rcvr = NULL;
}

static struct listener_entry *
find_listener_by_port(unsigned short port)
{
    port = ntohs(port);

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
    //assemble the ethernet header
    eth_hdr_t ehdr;
    ehdr.dst = dst;
    ehdr.src = _local_mac_addr;
    ehdr.ethertype = ethertype;

    //grab an out buffer and pointer
    //select the output buffer based on type of packet
    uint8_t *p;
    p = eth_buf;
    size_t total_len = 0;

    //create a list of all buffers to copy
    const void *buffs[] = {&ehdr, buf0, buf1, buf2};
    size_t lens[] = {sizeof(ehdr), len0, len1, len2};

    //copy each buffer into the out buffer
    for (size_t i = 0; i < sizeof(buffs)/sizeof(buffs[0]); i++){
        total_len += lens[i]; //use full length (not clipped)
        size_t bytes_remaining = out_buff_size - (size_t)(p - (uint8_t*)eth_buf);
        if (lens[i] > bytes_remaining) lens[i] = bytes_remaining;
        memcpy(p, buffs[i], lens[i]);
        p += lens[i];
    }

    //ensure that minimum length requirements are met
    if (total_len < 64) total_len = 64; //60 + ctrl word

    //For some reason, the ENC28J60 won't send the CRC
    //if you don't tell it to send another byte after
    //the given packet
    enc28j60_send(eth_buf, total_len);
}

static void
send_ip_pkt(struct ip_addr dst, int protocol,
    const void *buf0, uint16_t len0,
    const void *buf1, uint16_t len1)
{
    struct ip_hdr ip;
    _IPH_VHLTOS_SET(&ip, 4, 5, 0);
    _IPH_LEN_SET(&ip, (IP_HLEN + len0 + len1));
    _IPH_ID_SET(&ip, 0);
    _IPH_OFFSET_SET(&ip, htons(IP_DF));	/* don't fragment */
    _IPH_TTL_SET(&ip, 64);
    _IPH_PROTO_SET(&ip, protocol);
    _IPH_CHKSUM_SET(&ip, 0);
    ip.src.addr = htonl(_local_ip_addr.addr);
    ip.dest = dst;

    _IPH_CHKSUM_SET(&ip, ~chksum_buffer(
        (uint16_t *) &ip, sizeof(ip)/sizeof(int16_t), 0
    ));

    eth_mac_addr_t dst_mac;
    bool found = arp_cache_lookup_mac(&ip.dest, &dst_mac);
    if (!found) return;

    send_pkt(dst_mac, htons(ETHERTYPE_IPV4),
       &ip, sizeof(ip), buf0, len0, buf1, len1);
}

void 
send_udp_pkt(int src_port, struct socket_address dst,
    const void *buf, size_t len)
{
    struct udp_hdr udp _AL2;
    udp.src = htons(src_port);
    udp.dest = htons(dst.port);
    udp.len = htons(UDP_HLEN + len);
    udp.chksum = 0;

    send_ip_pkt(dst.addr, IP_PROTO_UDP,
        &udp, sizeof(udp), buf, len);
}

static void
handle_udp_packet(struct ip_addr src_ip, struct ip_addr dst_ip,
    struct udp_hdr *udp, size_t len)
{
    unsigned char *payload = ((unsigned char *) udp) + UDP_HLEN;
    int payload_len = len - UDP_HLEN;

    struct listener_entry *lx = find_listener_by_port(udp->dest);
    if (lx){
        struct socket_address src = make_socket_address(src_ip, ntohs(udp->src));
        struct socket_address dst = make_socket_address(dst_ip, ntohs(udp->dest));
        lx->rcvr(src, dst, payload, payload_len);
    }
}

static void
handle_icmp_packet(struct ip_addr src, struct ip_addr dst,
    struct icmp_echo_hdr *icmp, size_t len)
{
    switch (icmp->type){
        case ICMP_DUR:	// Destination Unreachable
            if (icmp->code == ICMP_DUR_PORT){	// port unreachable
                //filter out non udp data response
                struct ip_hdr *ip = (struct ip_hdr *)(((uint8_t*)icmp) + sizeof(struct icmp_echo_hdr));
                struct udp_hdr *udp = (struct udp_hdr *)(((char *)ip) + IP_HLEN);
                uint8_t protocol = ntohs(ip->_ttl_proto) & 0xff;
                if (protocol != IP_PROTO_UDP) break;

                struct listener_entry *lx = find_listener_by_port(udp->src);
                if (lx){
                    struct socket_address src = make_socket_address(ip->src, udp->src);
                    struct socket_address dst = make_socket_address(ip->dest, udp->dest);
                    lx->rcvr(src, dst, NULL, 0);
                }
            }
            break;

        case ICMP_ECHO:{
            const void *icmp_data_buff = ((uint8_t*)icmp) + sizeof(struct icmp_echo_hdr);
            uint16_t icmp_data_len = len - sizeof(struct icmp_echo_hdr);

            struct icmp_echo_hdr echo_reply;
            echo_reply.type = 0;
            echo_reply.code = 0;
            echo_reply.chksum = 0;
            echo_reply.id = icmp->id;
            echo_reply.seqno = icmp->seqno;
            echo_reply.chksum = ~chksum_buffer( //data checksum
                (uint16_t *)icmp_data_buff,
                icmp_data_len/sizeof(int16_t),
                chksum_buffer(                  //header checksum
                    (uint16_t *)&echo_reply,
                    sizeof(echo_reply)/sizeof(int16_t),
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
    reply.ar_op = htons(ARPOP_REPLY);
    memcpy(reply.ar_sha, &our_mac, 6);
    memcpy(reply.ar_sip, req->ar_tip, 4);
    memcpy(reply.ar_tha, req->ar_sha, 6);
    memcpy(reply.ar_tip, req->ar_sip, 4);

    eth_mac_addr_t t;
    memcpy(t.addr, reply.ar_tha, 6);
    send_pkt(t, htons(ETHERTYPE_ARP), &reply, sizeof(reply), 0, 0, 0, 0);
}

static void
handle_arp_packet(struct arp_eth_ipv4 *p, size_t size)
{
    if (size < sizeof(struct arp_eth_ipv4))
        return;

    if (ntohs(p->ar_hrd) != ARPHRD_ETHER
        || ntohs(p->ar_pro) != ETHERTYPE_IPV4
        || p->ar_hln != 6
        || p->ar_pln != 4)
        return;

    if(ntohs(p->ar_op) == ARPOP_REPLY){
        struct ip_addr ip_addr;
        memcpy(&ip_addr, p->ar_sip, sizeof(ip_addr));
        eth_mac_addr_t mac_addr;
        memcpy(&mac_addr, p->ar_sha, sizeof(mac_addr));
        arp_cache_update(&ip_addr, &mac_addr);
    }

    if (ntohs(p->ar_op) != ARPOP_REQUEST)
        return;

    struct ip_addr sip;
    struct ip_addr tip;

    memcpy(&(sip.addr), &(p->ar_sip), 4);
    memcpy(&(tip.addr), &(p->ar_tip), 4);
    sip.addr = ntohl(sip.addr);
    tip.addr = ntohl(tip.addr);

    if(memcmp(&tip, &_local_ip_addr, sizeof(_local_ip_addr)) == 0){	//They're looking for us
        send_arp_reply(p, _local_mac_addr);
    }
}

void
handle_eth_packet(size_t recv_len)
{
    eth_hdr_t *eth_hdr = (eth_hdr_t *)eth_buf;
    uint16_t ethertype = htons(eth_hdr->ethertype);

    if (ethertype == ETHERTYPE_ARP){
        struct arp_eth_ipv4 *arp = (struct arp_eth_ipv4 *)(eth_buf + sizeof(eth_hdr_t));
        handle_arp_packet(arp, recv_len-ETH_HLEN);
    }
    else if (ethertype == ETHERTYPE_IPV4){
        struct ip_hdr *ip = (struct ip_hdr *)(eth_buf + sizeof(eth_hdr_t));

        if (_IPH_V(ip) != 4 || _IPH_HL(ip) != 5)	// ignore pkts w/ bad version or options
            return;

        if (_IPH_OFFSET(ip) & (IP_MF | IP_OFFMASK))	// ignore fragmented packets
            return;

        // filter on dest ip addr (should be broadcast or for us)
        bool is_bcast = memcmp(&eth_hdr->dst, &BCAST_MAC_ADDR, sizeof(BCAST_MAC_ADDR)) == 0;
        struct ip_addr htonl_local_ip_addr;
        htonl_local_ip_addr.addr = htonl(_local_ip_addr.addr);

        bool is_my_ip = memcmp(&ip->dest, &htonl_local_ip_addr, sizeof(_local_ip_addr)) == 0;
        if (!is_bcast && !is_my_ip) return;

        arp_cache_update(&ip->src, (eth_mac_addr_t *)(((char *)eth_buf)+6));

        switch (_IPH_PROTO(ip)){
            case IP_PROTO_UDP:
                handle_udp_packet(ip->src, ip->dest, (struct udp_hdr *)(((char *)ip) + IP_HLEN), (recv_len-ETH_HLEN-IP_HLEN));
                break;

            case IP_PROTO_ICMP:
                handle_icmp_packet(ip->src, ip->dest, (struct icmp_echo_hdr *)(((char *)ip) + IP_HLEN), (recv_len-ETH_HLEN-IP_HLEN));
                break;

            default:	// ignore
                break;
        }
    }
    else
        return;	// Not ARP or IPV4, ignore
}

/***********************************************************************
 * Timer+GARP stuff
 **********************************************************************/

static bool send_garp = false;
static uint32_t num_overflows = 0;

// Six overflows is the closest overflow count to one minute.
ISR(TIMER1_OVF_vect){
    num_overflows++;
    if(!(num_overflows % 6)) send_garp = true;
}

// Send a GARP packet once per minute.
static void
send_gratuitous_arp(){
    send_garp = false;

    //Need to htonl IP address
    struct ip_addr htonl_ip_addr;
    htonl_ip_addr.addr = htonl(_local_ip_addr.addr);

    struct arp_eth_ipv4 req _AL4;
    req.ar_hrd = htons(ARPHRD_ETHER);
    req.ar_pro = htons(ETHERTYPE_IPV4);
    req.ar_hln = sizeof(eth_mac_addr_t);
    req.ar_pln = sizeof(struct ip_addr);
    req.ar_op = htons(ARPOP_REQUEST);
    memcpy(req.ar_sha, &_local_mac_addr, sizeof(eth_mac_addr_t));
    memcpy(req.ar_sip, &htonl_ip_addr,   sizeof(struct ip_addr));
    memset(req.ar_tha, 0x00,             sizeof(eth_mac_addr_t));
    memcpy(req.ar_tip, &htonl_ip_addr,   sizeof(struct ip_addr));

    //Send the request with the broadcast MAC address
    send_pkt(BCAST_MAC_ADDR, htons(ETHERTYPE_ARP), &req, sizeof(req), 0, 0, 0, 0);
}

// Executed every loop
void network_check(void){
    size_t recv_len = enc28j60_recv(eth_buf, ETH_BUF_SIZE);
    if(recv_len > 0) handle_eth_packet(recv_len);

    if(send_garp) send_gratuitous_arp();
}

void network_init(void){
    /*
     * Read MAC address from EEPROM and initialize Ethernet driver. If EEPROM is blank,
     * use default MAC address instead.
     */
    eeprom_busy_wait();
    if(eeprom_read_byte(0) == 0xFF){
        _MAC_ADDR(_local_mac_addr.addr, 0x00,0x80,0x2F,0x11,0x22,0x33);
        _local_ip_addr.addr = _IP(192,168,10,3);
        using_network_defaults = true;
    }
    else{
        eeprom_read_block((void*)&_local_mac_addr, (void*)OCTOCLOCK_EEPROM_MAC_ADDR, 6);
        eeprom_read_block((void*)&_local_ip_addr,  (void*)OCTOCLOCK_EEPROM_IP_ADDR, 4);
        using_network_defaults = false;
    }

    enc28j60_init((uint8_t*)&_local_mac_addr);
    init_udp_listeners();

    send_garp = true;
}
