
// Copyright 2012-2013 Ettus Research LLC

#include <u3_net_stack.h>
#include <string.h> //memcmp
#include <trace.h>

#define MAX_NETHS 4

typedef struct
{
    padded_eth_hdr_t eth;
    struct arp_eth_ipv4 arp;
} padded_arp_t;

typedef struct
{
    padded_eth_hdr_t eth;
    struct ip_hdr ip;
    struct icmp_echo_hdr icmp;
} padded_icmp_t;

typedef struct
{
    padded_eth_hdr_t eth;
    struct ip_hdr ip;
    struct udp_hdr udp;
} padded_udp_t;

/***********************************************************************
 * declares for internal handlers
 **********************************************************************/
static void handle_icmp_echo_packet(
    const uint8_t ethno,
    const struct ip_addr *src, const struct ip_addr *dst,
    const uint16_t id, const uint16_t seq,
    const void *buff, const size_t num_bytes
){
    u3_net_stack_send_icmp_pkt(ethno, ICMP_ER, 0, id, seq, src, buff, num_bytes);
}

static void handle_icmp_dur_packet(
    const uint8_t ethno,
    const struct ip_addr *src, const struct ip_addr *dst,
    const uint16_t id, const uint16_t seq,
    const void *buff, const size_t num_bytes
);

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
 * ARP Cache implementation
 **********************************************************************/
#define ARP_CACHE_NENTRIES 32

static size_t arp_cache_wr_index;

static struct ip_addr arp_cache_ips[ARP_CACHE_NENTRIES];
static eth_mac_addr_t arp_cache_macs[ARP_CACHE_NENTRIES];
static uint8_t arp_cache_eths[ARP_CACHE_NENTRIES];

void u3_net_stack_arp_cache_update(const struct ip_addr *ip_addr, const eth_mac_addr_t *mac_addr, const uint8_t ethno)
{
    for (size_t i = 0; i < ARP_CACHE_NENTRIES; i++)
    {
        if (memcmp(ip_addr, arp_cache_ips+i, sizeof(struct ip_addr)) == 0)
        {
            memcpy(arp_cache_macs+i, mac_addr, sizeof(eth_mac_addr_t));
            arp_cache_eths[i] = ethno;
            return;
        }
    }
    if (arp_cache_wr_index >= ARP_CACHE_NENTRIES) arp_cache_wr_index = 0;
    memcpy(arp_cache_ips+arp_cache_wr_index, ip_addr, sizeof(struct ip_addr));
    memcpy(arp_cache_macs+arp_cache_wr_index, mac_addr, sizeof(eth_mac_addr_t));
    arp_cache_eths[arp_cache_wr_index] = ethno;
    arp_cache_wr_index++;
}

const eth_mac_addr_t *u3_net_stack_arp_cache_lookup(const struct ip_addr *ip_addr)
{
    //do a local look up on our own ports
    for (size_t e = 0; e < MAX_NETHS; e++)
    {
        if (memcmp(ip_addr, u3_net_stack_get_ip_addr(e), sizeof(struct ip_addr)) == 0)
        {
            return u3_net_stack_get_mac_addr(e);
        }
    }
    //now check the arp cache
    for (size_t i = 0; i < ARP_CACHE_NENTRIES; i++)
    {
        if (memcmp(ip_addr, arp_cache_ips+i, sizeof(struct ip_addr)) == 0)
        {
            return &arp_cache_macs[i];
        }
    }
    return NULL;
}

bool resolve_ip(const struct ip_addr *ip_addr, eth_mac_addr_t *mac_addr)
{
    for (size_t e = 0; e < MAX_NETHS; e++)
    {
        if (memcmp(u3_net_stack_get_bcast(e), ip_addr, sizeof(struct ip_addr)) == 0)
        {
            memset(mac_addr, 0xff, sizeof(eth_mac_addr_t));
            return true;
        }
    }
    const eth_mac_addr_t *r = u3_net_stack_arp_cache_lookup(ip_addr);
    if (r != NULL)
    {
        memcpy(mac_addr, r, sizeof(eth_mac_addr_t));
        return true;
    }
    return false;
}

/***********************************************************************
 * Net stack config
 **********************************************************************/
static wb_pkt_iface64_config_t *pkt_iface_config = NULL;

void u3_net_stack_init(wb_pkt_iface64_config_t *config)
{
    pkt_iface_config = config;
    u3_net_stack_register_icmp_handler(ICMP_ECHO, 0, &handle_icmp_echo_packet);
    u3_net_stack_register_icmp_handler(ICMP_DUR, ICMP_DUR_PORT, &handle_icmp_dur_packet);
}

static struct ip_addr net_conf_ips[MAX_NETHS];
static eth_mac_addr_t net_conf_macs[MAX_NETHS];
static struct ip_addr net_conf_subnets[MAX_NETHS];
static struct ip_addr net_conf_bcasts[MAX_NETHS];
static uint32_t net_stat_counts[MAX_NETHS];

void u3_net_stack_init_eth(
    const uint8_t ethno,
    const eth_mac_addr_t *mac,
    const struct ip_addr *ip,
    const struct ip_addr *subnet
)
{
    memcpy(&net_conf_macs[ethno], mac, sizeof(eth_mac_addr_t));
    memcpy(&net_conf_ips[ethno], ip, sizeof(struct ip_addr));
    memcpy(&net_conf_subnets[ethno], subnet, sizeof(struct ip_addr));
    net_stat_counts[ethno] = 0;
}

const struct ip_addr *u3_net_stack_get_ip_addr(const uint8_t ethno)
{
    return &net_conf_ips[ethno];
}

const struct ip_addr *u3_net_stack_get_subnet(const uint8_t ethno)
{
    return &net_conf_subnets[ethno];
}

const struct ip_addr *u3_net_stack_get_bcast(const uint8_t ethno)
{
    const uint32_t ip = u3_net_stack_get_ip_addr(ethno)->addr;
    const uint32_t subnet = u3_net_stack_get_subnet(ethno)->addr;
    net_conf_bcasts[ethno].addr = ip | (~subnet);
    return &net_conf_bcasts[ethno];
}

const eth_mac_addr_t *u3_net_stack_get_mac_addr(const uint8_t ethno)
{
    return &net_conf_macs[ethno];
}

/***********************************************************************
 * Ethernet activity stats
 **********************************************************************/
uint32_t u3_net_stack_get_stat_counts(const uint8_t ethno)
{
    return net_stat_counts[ethno];
}

static void incr_stat_counts(const void *p)
{
    const padded_eth_hdr_t *eth = (const padded_eth_hdr_t *)p;
    if (eth->ethno < MAX_NETHS) net_stat_counts[eth->ethno]++;
}

/***********************************************************************
 * Ethernet handlers - send packet w/ payload
 **********************************************************************/
static void send_eth_pkt(
    const void *p0, const size_t l0,
    const void *p1, const size_t l1,
    const void *p2, const size_t l2
)
{
    incr_stat_counts(p0);
    void *ptr = wb_pkt_iface64_tx_claim(pkt_iface_config);
    size_t buff_i = 0;

    uint32_t *buff32 = (uint32_t *)ptr;
    for (size_t i = 0; i < (l0+3)/4; i++)
    {
        buff32[buff_i++] = ((const uint32_t *)p0)[i];
    }
    for (size_t i = 0; i < (l1+3)/4; i++)
    {
        buff32[buff_i++] = ((const uint32_t *)p1)[i];
    }
    for (size_t i = 0; i < (l2+3)/4; i++)
    {
        buff32[buff_i++] = ((const uint32_t *)p2)[i];
    }

    // Fixes issue where we don't write an even number of 32bit words leaving last data stranded in H/W.
    if ((buff_i%2) == 1) buff32[buff_i++] = 0;

    wb_pkt_iface64_tx_submit(pkt_iface_config, l0 + l1 + l2);
}

/***********************************************************************
 * ARP handlers
 **********************************************************************/
static void send_arp_reply(
    const uint8_t ethno,
    const struct arp_eth_ipv4 *req,
    const eth_mac_addr_t *our_mac
){
    padded_arp_t reply;
    reply.eth.ethno = ethno;
    memcpy(&reply.eth.dst, (eth_mac_addr_t *)req->ar_sha, sizeof(eth_mac_addr_t));
    memcpy(&reply.eth.src, u3_net_stack_get_mac_addr(ethno), sizeof(eth_mac_addr_t));
    reply.eth.ethertype = ETHERTYPE_ARP;

    reply.arp.ar_hrd = req->ar_hrd;
    reply.arp.ar_pro = req->ar_pro;
    reply.arp.ar_hln = req->ar_hln;
    reply.arp.ar_pln = req->ar_pln;
    reply.arp.ar_op = ARPOP_REPLY;
    memcpy(reply.arp.ar_sha, our_mac,     sizeof(eth_mac_addr_t));
    memcpy(reply.arp.ar_sip, req->ar_tip, sizeof(struct ip_addr));
    memcpy(reply.arp.ar_tha, req->ar_sha, sizeof(eth_mac_addr_t));
    memcpy(reply.arp.ar_tip, req->ar_sip, sizeof(struct ip_addr));

    send_eth_pkt(&reply, sizeof(reply), NULL, 0, NULL, 0);
}

void u3_net_stack_send_arp_request(const uint8_t ethno, const struct ip_addr *addr)
{
    padded_arp_t req;
    req.eth.ethno = ethno;
    memset(&req.eth.dst, 0xff, sizeof(eth_mac_addr_t)); //bcast
    memcpy(&req.eth.src, u3_net_stack_get_mac_addr(ethno), sizeof(eth_mac_addr_t));
    req.eth.ethertype = ETHERTYPE_ARP;

    req.arp.ar_hrd = ARPHRD_ETHER;
    req.arp.ar_pro = ETHERTYPE_IPV4;
    req.arp.ar_hln = sizeof(eth_mac_addr_t);
    req.arp.ar_pln = sizeof(struct ip_addr);
    req.arp.ar_op = ARPOP_REQUEST;
    memcpy(req.arp.ar_sha, u3_net_stack_get_mac_addr(ethno), sizeof(eth_mac_addr_t));
    memcpy(req.arp.ar_sip, u3_net_stack_get_ip_addr(ethno), sizeof(struct ip_addr));
    memset(req.arp.ar_tha, 0x00, sizeof(eth_mac_addr_t));
    memcpy(req.arp.ar_tip, addr, sizeof(struct ip_addr));

    send_eth_pkt(&req, sizeof(req), NULL, 0, NULL, 0);
}

static void handle_arp_packet(const uint8_t ethno, const struct arp_eth_ipv4 *p)
{
    UHD_FW_TRACE(DEBUG, "handle_arp_packet");
    if (p->ar_hrd != ARPHRD_ETHER
      || p->ar_pro != ETHERTYPE_IPV4
      || p->ar_hln != sizeof(eth_mac_addr_t)
      || p->ar_pln != sizeof(struct ip_addr))
    return;

    //got an arp reply -- injest it into the arp cache
    if (p->ar_op == ARPOP_REPLY)
    {
        UHD_FW_TRACE(DEBUG, "ARPOP_REPLY");
        struct ip_addr ip_addr;
        memcpy(&ip_addr, p->ar_sip, sizeof(ip_addr));
        eth_mac_addr_t mac_addr;
        memcpy(&mac_addr, p->ar_sha, sizeof(mac_addr));
        u3_net_stack_arp_cache_update(&ip_addr, &mac_addr, ethno);
    }

    //got an arp request -- reply if its for our address
    if (p->ar_op == ARPOP_REQUEST)
    {
        UHD_FW_TRACE(DEBUG, "ARPOP_REQUEST");
        if (memcmp(p->ar_tip, u3_net_stack_get_ip_addr(ethno), sizeof(struct ip_addr)) == 0)
        {
            send_arp_reply(ethno, p, u3_net_stack_get_mac_addr(ethno));
        }
    }
}

/***********************************************************************
 * UDP handlers
 **********************************************************************/
#define UDP_NHANDLERS 16

static uint16_t udp_handler_ports[UDP_NHANDLERS];
static u3_net_stack_udp_handler_t udp_handlers[UDP_NHANDLERS];
static size_t udp_handlers_index = 0;

void u3_net_stack_register_udp_handler(
    const uint16_t port,
    const u3_net_stack_udp_handler_t handler
)
{
    if (udp_handlers_index < UDP_NHANDLERS)
    {
        udp_handler_ports[udp_handlers_index] = port;
        udp_handlers[udp_handlers_index] = handler;
        udp_handlers_index++;
    }
}

void u3_net_stack_send_udp_pkt(
    const uint8_t ethno,
    const struct ip_addr *dst,
    const uint16_t src_port,
    const uint16_t dst_port,
    const void *buff,
    const size_t num_bytes
)
{
    eth_mac_addr_t dst_mac_addr;
    if (!resolve_ip(dst, &dst_mac_addr))
    {
        UHD_FW_TRACE(WARN, "u3_net_stack_send_udp_pkt arp_cache_lookup fail");
        return;
    }

    padded_udp_t pkt;

    pkt.eth.ethno = ethno;
    memcpy(&pkt.eth.dst, &dst_mac_addr,                    sizeof(eth_mac_addr_t));
    memcpy(&pkt.eth.src, u3_net_stack_get_mac_addr(ethno), sizeof(eth_mac_addr_t));
    pkt.eth.ethertype = ETHERTYPE_IPV4;

    IPH_VHLTOS_SET(&pkt.ip, 4, 5, 0);
    IPH_LEN_SET(&pkt.ip, IP_HLEN + UDP_HLEN + num_bytes);
    IPH_ID_SET(&pkt.ip, 0);
    IPH_OFFSET_SET(&pkt.ip, IP_DF);	/* don't fragment */
    IPH_TTL_SET(&pkt.ip, 32);
    IPH_PROTO_SET(&pkt.ip, IP_PROTO_UDP);
    IPH_CHKSUM_SET(&pkt.ip, 0);
    memcpy(&pkt.ip.src, u3_net_stack_get_ip_addr(ethno), sizeof(struct ip_addr));
    memcpy(&pkt.ip.dest, dst, sizeof(struct ip_addr));

    IPH_CHKSUM_SET(&pkt.ip, ~chksum_buffer(
        (unsigned short *) &pkt.ip, sizeof(pkt.ip)/sizeof(short), 0
    ));

    pkt.udp.src = src_port;
    pkt.udp.dest = dst_port;
    pkt.udp.len = UDP_HLEN + num_bytes;
    pkt.udp.chksum = 0;

    send_eth_pkt(&pkt, sizeof(pkt), buff, num_bytes, NULL, 0);
}

static void handle_udp_packet(
    const uint8_t ethno,
    const struct ip_addr *src,
    const struct ip_addr *dst,
    const struct udp_hdr *udp,
    const size_t num_bytes
){
    for (size_t i = 0; i < udp_handlers_index; i++)
    {
        if (udp_handler_ports[i] == udp->dest)
        {
            udp_handlers[i](
                ethno, src, u3_net_stack_get_ip_addr(ethno), udp->src, udp->dest,
                ((const uint8_t *)udp) + sizeof(struct udp_hdr),
                num_bytes - UDP_HLEN
            );
            return;
        }
    }
    UHD_FW_TRACE_FSTR(ERROR, "Unhandled UDP packet src=%u, dest=%u", udp->src, udp->dest);
    //TODO send destination unreachable
}

/***********************************************************************
 * ICMP handlers
 **********************************************************************/
#define ICMP_NHANDLERS 8

static uint8_t icmp_handler_types[ICMP_NHANDLERS];
static uint8_t icmp_handler_codes[ICMP_NHANDLERS];
static u3_net_stack_icmp_handler_t icmp_handlers[ICMP_NHANDLERS];
static size_t icmp_handlers_index = 0;

void u3_net_stack_register_icmp_handler(
    const uint8_t type,
    const uint8_t code,
    const u3_net_stack_icmp_handler_t handler
)
{
    if (icmp_handlers_index < ICMP_NHANDLERS)
    {
        icmp_handler_types[icmp_handlers_index] = type;
        icmp_handler_codes[icmp_handlers_index] = code;
        icmp_handlers[icmp_handlers_index] = handler;
        icmp_handlers_index++;
    }
}

static void handle_icmp_packet(
    const uint8_t ethno,
    const struct ip_addr *src,
    const struct ip_addr *dst,
    const struct icmp_echo_hdr *icmp,
    const size_t num_bytes
){

    for (size_t i = 0; i < icmp_handlers_index; i++)
    {
        if (icmp_handler_types[i] == icmp->type && icmp_handler_codes[i] == icmp->code)
        {
            icmp_handlers[i](
                ethno, src, u3_net_stack_get_ip_addr(ethno), icmp->id, icmp->seqno,
                ((const uint8_t *)icmp) + sizeof(struct icmp_echo_hdr),
                num_bytes - sizeof(struct icmp_echo_hdr)
            );
            return;
        }
    }
    UHD_FW_TRACE_FSTR(ERROR, "Unhandled ICMP packet type=%u", icmp->type);
}

static void handle_icmp_dur_packet(
    const uint8_t ethno,
    const struct ip_addr *src, const struct ip_addr *dst,
    const uint16_t id, const uint16_t seq,
    const void *buff, const size_t num_bytes
){
    struct ip_hdr *ip = (struct ip_hdr *)buff;
    struct udp_hdr *udp = (struct udp_hdr *)(((char *)ip) + IP_HLEN);
    if (IPH_PROTO(ip) != IP_PROTO_UDP) return;
    for (size_t i = 0; i < udp_handlers_index; i++)
    {
        if (udp_handler_ports[i] == udp->src)
        {
            udp_handlers[i](ethno,
                src, u3_net_stack_get_ip_addr(ethno),
                udp->src, udp->dest, NULL, 0
            );
            return;
        }
    }
}

void u3_net_stack_send_icmp_pkt(
    const uint8_t ethno,
    const uint8_t type,
    const uint8_t code,
    const uint16_t id,
    const uint16_t seq,
    const struct ip_addr *dst,
    const void *buff,
    const size_t num_bytes
)
{
    eth_mac_addr_t dst_mac_addr;
    if (!resolve_ip(dst, &dst_mac_addr))
    {
        UHD_FW_TRACE(WARN, "u3_net_stack_send_echo_request arp_cache_lookup fail");
        return;
    }

    padded_icmp_t pkt;

    pkt.eth.ethno = ethno;
    memcpy(&pkt.eth.dst, &dst_mac_addr,                    sizeof(eth_mac_addr_t));
    memcpy(&pkt.eth.src, u3_net_stack_get_mac_addr(ethno), sizeof(eth_mac_addr_t));
    pkt.eth.ethertype = ETHERTYPE_IPV4;

    IPH_VHLTOS_SET(&pkt.ip, 4, 5, 0);
    IPH_LEN_SET(&pkt.ip, IP_HLEN + sizeof(pkt.icmp) + num_bytes);
    IPH_ID_SET(&pkt.ip, 0);
    IPH_OFFSET_SET(&pkt.ip, IP_DF);	/* don't fragment */
    IPH_TTL_SET(&pkt.ip, 32);
    IPH_PROTO_SET(&pkt.ip, IP_PROTO_ICMP);
    IPH_CHKSUM_SET(&pkt.ip, 0);
    memcpy(&pkt.ip.src, u3_net_stack_get_ip_addr(ethno), sizeof(struct ip_addr));
    memcpy(&pkt.ip.dest, dst, sizeof(struct ip_addr));

    IPH_CHKSUM_SET(&pkt.ip, ~chksum_buffer(
        (unsigned short *) &pkt.ip, sizeof(pkt.ip)/sizeof(short), 0
    ));

    pkt.icmp.type = type;
    pkt.icmp.code = code;
    pkt.icmp.chksum = 0;
    pkt.icmp.id = id;
    pkt.icmp.seqno = seq;
    pkt.icmp.chksum = ~chksum_buffer( //data checksum
        (unsigned short *)buff,
        num_bytes/sizeof(short),
        chksum_buffer(                  //header checksum
            (unsigned short *)&pkt.icmp,
            sizeof(pkt.icmp)/sizeof(short),
        0)
    );

    send_eth_pkt(&pkt, sizeof(pkt), buff, num_bytes, NULL, 0);
}

/***********************************************************************
 * Ethernet handlers
 **********************************************************************/
static void handle_eth_packet(const void *buff, const size_t num_bytes)
{
    const padded_eth_hdr_t *eth_hdr = (padded_eth_hdr_t *)buff;
    const uint8_t *eth_body = ((const uint8_t *)buff) + sizeof(padded_eth_hdr_t);
    UHD_FW_TRACE_FSTR(DEBUG, "handle_eth_packet got ethertype 0x%x", (unsigned)eth_hdr->ethertype);

    if (eth_hdr->ethertype == ETHERTYPE_ARP)
    {
        UHD_FW_TRACE(DEBUG, "eth_hdr->ethertype == ETHERTYPE_ARP");
        const struct arp_eth_ipv4 *arp = (const struct arp_eth_ipv4 *)eth_body;
        handle_arp_packet(eth_hdr->ethno, arp);
    }
    else if (eth_hdr->ethertype == ETHERTYPE_IPV4)
    {
        UHD_FW_TRACE(DEBUG, "eth_hdr->ethertype == ETHERTYPE_IPV4");
        const struct ip_hdr *ip = (const struct ip_hdr *)eth_body;
        const uint8_t *ip_body = eth_body + IP_HLEN;

        if (IPH_V(ip) != 4 || IPH_HL(ip) != 5) return;// ignore pkts w/ bad version or options
        if (IPH_OFFSET(ip) & (IP_MF | IP_OFFMASK)) return;// ignore fragmented packets

        //TODO -- only handle when the mac is bcast or IP is for us

        u3_net_stack_arp_cache_update(&ip->src, &eth_hdr->src, eth_hdr->ethno);

        if (IPH_PROTO(ip) == IP_PROTO_UDP)
        {
            handle_udp_packet(
                eth_hdr->ethno, &ip->src, &ip->dest,
                (const struct udp_hdr *)ip_body,
                IPH_LEN(ip) - IP_HLEN
            );
        }

        if (IPH_PROTO(ip) == IP_PROTO_ICMP)
        {
            handle_icmp_packet(
                eth_hdr->ethno, &ip->src, &ip->dest,
                (const struct icmp_echo_hdr *)ip_body,
                IPH_LEN(ip) - IP_HLEN
            );
        }
    }
    else return;    // Not ARP or IPV4, ignore
}

void u3_net_stack_handle_one(void)
{
    size_t num_bytes = 0;
    const void *ptr = wb_pkt_iface64_rx_try_claim(pkt_iface_config, &num_bytes);
    if (ptr != NULL)
    {
        UHD_FW_TRACE_FSTR(DEBUG, "u3_net_stack_handle_one got %u bytes", (unsigned)num_bytes);
        incr_stat_counts(ptr);
        handle_eth_packet(ptr, num_bytes);
        wb_pkt_iface64_rx_release(pkt_iface_config);
    }
}
