//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#include "uhd_dpdk_driver.h"
#include "uhd_dpdk_fops.h"
#include "uhd_dpdk_udp.h"
#include <rte_malloc.h>
#include <rte_mempool.h>
#include <arpa/inet.h>
#include <unistd.h>

int _uhd_dpdk_process_arp(struct uhd_dpdk_port *port, struct arp_hdr *arp_frame)
{
    uint32_t dest_ip = arp_frame->arp_data.arp_sip;
    struct ether_addr dest_addr = arp_frame->arp_data.arp_sha;

    struct uhd_dpdk_arp_entry *entry = NULL;
    rte_hash_lookup_data(port->arp_table, &dest_ip, (void **) &entry);
    if (!entry) {
        entry = rte_zmalloc(NULL, sizeof(*entry), 0);
        if (!entry) {
            return -ENOMEM;
        }
        LIST_INIT(&entry->pending_list);
        ether_addr_copy(&dest_addr, &entry->mac_addr);
        if (rte_hash_add_key_data(port->arp_table, &dest_ip, entry) < 0) {
            rte_free(entry);
            return -ENOSPC;
        }
    } else {
        struct uhd_dpdk_config_req *req = NULL;
        ether_addr_copy(&dest_addr, &entry->mac_addr);
        /* Now wake any config reqs waiting for the ARP */
        LIST_FOREACH(req, &entry->pending_list, entry) {
            _uhd_dpdk_config_req_compl(req, 0);
        }
    }

    return 0;
}

int _uhd_dpdk_arp_request(struct uhd_dpdk_port *port, uint32_t ip)
{
    struct rte_mbuf *mbuf;
    struct ether_hdr *hdr;
    struct arp_hdr *arp_frame;

    mbuf = rte_pktmbuf_alloc(port->parent->tx_pktbuf_pool);
    if (unlikely(mbuf == NULL)) {
        RTE_LOG(WARNING, MEMPOOL, "Could not allocate packet buffer for ARP request\n");
        return -ENOMEM;
    }

    hdr = rte_pktmbuf_mtod(mbuf, struct ether_hdr *);
    arp_frame = (struct arp_hdr *) &hdr[1];

    memset(hdr->d_addr.addr_bytes, 0xFF, ETHER_ADDR_LEN);
    ether_addr_copy(&port->mac_addr, &hdr->s_addr);
    hdr->ether_type = rte_cpu_to_be_16(ETHER_TYPE_ARP);

    arp_frame->arp_hrd = rte_cpu_to_be_16(ARP_HRD_ETHER);
    arp_frame->arp_pro = rte_cpu_to_be_16(ETHER_TYPE_IPv4);
    arp_frame->arp_hln = 6;
    arp_frame->arp_pln = 4;
    arp_frame->arp_op  = rte_cpu_to_be_16(ARP_OP_REQUEST);
    ether_addr_copy(&port->mac_addr, &arp_frame->arp_data.arp_sha);
    arp_frame->arp_data.arp_sip = port->ipv4_addr;
    memset(arp_frame->arp_data.arp_tha.addr_bytes, 0x00, ETHER_ADDR_LEN);
    arp_frame->arp_data.arp_tip = ip;

    mbuf->pkt_len = 42;
    mbuf->data_len = 42;
    mbuf->ol_flags = PKT_TX_IP_CKSUM;

    if (rte_eth_tx_burst(port->id, 0, &mbuf, 1) != 1) {
        RTE_LOG(WARNING, RING, "%s: TX descriptor ring is full\n", __func__);
        rte_pktmbuf_free(mbuf);
        return -EAGAIN;
    }
    return 0;
}


int _uhd_dpdk_process_udp(struct uhd_dpdk_port *port, struct rte_mbuf *mbuf, struct udp_hdr *pkt)
{
    int status = 0;
    struct uhd_dpdk_ipv4_5tuple ht_key = {
        .sock_type = UHD_DPDK_SOCK_UDP,
        .src_ip = 0,
        .src_port = 0,
        .dst_ip = 0,
        .dst_port = pkt->dst_port
    };

    struct uhd_dpdk_socket *sock = NULL;
    rte_hash_lookup_data(port->rx_table, &ht_key, (void **) &sock);
    if (!sock) {
        status = -ENODEV;
        goto udp_rx_drop;
    }

    struct uhd_dpdk_udp_priv *pdata = (struct uhd_dpdk_udp_priv *) sock->priv;
    status = rte_ring_enqueue(sock->rx_ring, mbuf);
    if (status) {
        pdata->dropped_pkts++;
        goto udp_rx_drop;
    }
    return 0;

udp_rx_drop:
    rte_pktmbuf_free(mbuf);
    return status;
}

int _uhd_dpdk_process_ipv4(struct uhd_dpdk_port *port, struct rte_mbuf *mbuf, struct ipv4_hdr *pkt)
{
    if (pkt->dst_addr != port->ipv4_addr) {
        rte_pktmbuf_free(mbuf);
        return -ENODEV;
    }
    if (pkt->next_proto_id == 0x11) {
        return _uhd_dpdk_process_udp(port, mbuf, (struct udp_hdr *) &pkt[1]);
    }
    rte_pktmbuf_free(mbuf);
    return -EINVAL;
}

static int _uhd_dpdk_fill_ipv4_addr(struct uhd_dpdk_port *port,
                                    struct rte_mbuf *mbuf)
{
    struct ether_hdr *eth_hdr = rte_pktmbuf_mtod(mbuf, struct ether_hdr *);
    struct ipv4_hdr *ip_hdr = (struct ipv4_hdr *) &eth_hdr[1];
    if (is_broadcast(port, ip_hdr->dst_addr)) {
        memset(eth_hdr->d_addr.addr_bytes, 0xff, ETHER_ADDR_LEN);
    } else {
        /* Lookup dest_addr */
        struct uhd_dpdk_arp_entry *entry = NULL;
        rte_hash_lookup_data(port->arp_table, &ip_hdr->dst_addr, (void **) &entry);
        if (!entry) {
            RTE_LOG(ERR, USER1, "TX packet on port %d to addr 0x%08x has no ARP entry\n", port->id, ip_hdr->dst_addr);
            return -ENODEV;
        }

        ether_addr_copy(&entry->mac_addr, &eth_hdr->d_addr);
    }
    return 0;
}

static int _uhd_dpdk_send(struct uhd_dpdk_port *port,
                          struct uhd_dpdk_tx_queue *txq,
                          struct rte_ring *q)
{
    struct rte_mbuf *buf;

    unsigned int num_tx = rte_ring_count(q);
    num_tx = (num_tx < UHD_DPDK_TX_BURST_SIZE) ? num_tx : UHD_DPDK_TX_BURST_SIZE;
    for (unsigned int i = 0; i < num_tx; i++) {
        int status = rte_ring_dequeue(q, (void **) &buf);
        if (status) {
            RTE_LOG(ERR, USER1, "%s: Q Count doesn't match actual\n", __func__);
            break;
        }
        struct ether_hdr *eth_hdr = rte_pktmbuf_mtod(buf, struct ether_hdr *);
        if (eth_hdr->ether_type == rte_cpu_to_be_16(ETHER_TYPE_IPv4)) {
            status = _uhd_dpdk_fill_ipv4_addr(port, buf);
            if (status) {
                return status;
            }
        }
        status = rte_eth_tx_burst(port->id, 0, &buf, 1); /* Automatically frees mbuf */
        if (status != 1) {
            status = rte_ring_enqueue(txq->retry_queue, buf);
            if (status)
                RTE_LOG(WARNING, USER1, "%s: Could not re-enqueue pkt %d\n", __func__, i);
            num_tx = i;
            rte_pktmbuf_free(buf);
            break;
        }
    }

    return num_tx;
}

static inline int _uhd_dpdk_restore_bufs(struct uhd_dpdk_port *port,
                                         struct uhd_dpdk_tx_queue *q,
                                         unsigned int num_bufs)
{
    /* Allocate more buffers to replace the sent ones */
    struct rte_mbuf *freebufs[UHD_DPDK_TXQ_SIZE];
    int status = rte_pktmbuf_alloc_bulk(port->parent->tx_pktbuf_pool, freebufs, num_bufs);
    if (status) {
        RTE_LOG(ERR, USER1, "%d %s: Could not restore %u pktmbufs in bulk!\n", status, __func__, num_bufs);
    }

    /* Enqueue the buffers for the user thread to retrieve */
    unsigned int enqd = rte_ring_enqueue_bulk(q->freebufs, (void **) freebufs, num_bufs, NULL);
    if (enqd != num_bufs) {
        RTE_LOG(ERR, USER1, "Could not enqueue pktmbufs!\n");
        return status;
    }

    return num_bufs;
}

static inline void _uhd_dpdk_disable_ports(struct uhd_dpdk_thread *t)
{
    struct uhd_dpdk_port *port = NULL;
    LIST_FOREACH(port, &t->port_list, port_entry) {
        rte_eth_dev_stop(port->id);
    }
}

static inline int _uhd_dpdk_driver_cleanup(struct uhd_dpdk_thread *t)
{
    /* Close sockets upon request, but reply to other service requests with
     * errors
     */
    struct uhd_dpdk_config_req *sock_req;
    if (rte_ring_dequeue(t->sock_req_ring, (void **) &sock_req)) {
        switch (sock_req->req_type) {
        case UHD_DPDK_SOCK_CLOSE:
            _uhd_dpdk_sock_release(sock_req);
            break;
        default:
            _uhd_dpdk_config_req_compl(sock_req, -ENODEV);
            break;
        }
    }

    /* Do nothing if there are users remaining */
    struct uhd_dpdk_port *port = NULL;
    LIST_FOREACH(port, &t->port_list, port_entry) {
        /* Check for RX sockets */
        const void *hash_key;
        void *hash_sock;
        uint32_t hash_next = 0;
        if (rte_hash_iterate(port->rx_table, &hash_key,
                             &hash_sock, &hash_next) != -ENOENT)
            return -EAGAIN;

        /* Check for TX sockets */
        struct uhd_dpdk_tx_queue *q = NULL;
        LIST_FOREACH(q, &port->txq_list, entry) {
            if (!LIST_EMPTY(&q->tx_list))
                return -EAGAIN;
        }
    }

    /* Now can free memory */
    LIST_FOREACH(port, &t->port_list, port_entry) {
        rte_hash_free(port->rx_table);

        struct uhd_dpdk_tx_queue *q = LIST_FIRST(&port->txq_list);
        while (!LIST_EMPTY(&port->txq_list)) {
            struct uhd_dpdk_tx_queue *nextq = LIST_NEXT(q, entry);
            while (!rte_ring_empty(q->queue)) {
                struct rte_buf *buf = NULL;
                rte_ring_dequeue(q->queue, (void **) &buf);
                rte_free(buf);
            }
            while (!rte_ring_empty(q->freebufs)) {
                struct rte_buf *buf = NULL;
                rte_ring_dequeue(q->freebufs, (void **) &buf);
                rte_free(buf);
            }
            while (!rte_ring_empty(q->retry_queue)) {
                struct rte_buf *buf = NULL;
                rte_ring_dequeue(q->retry_queue, (void **) &buf);
                rte_free(buf);
            }
            rte_ring_free(q->queue);
            rte_ring_free(q->freebufs);
            rte_ring_free(q->retry_queue);
            rte_free(q);
            q = nextq;
        }

        const void *arp_key;
        uint32_t arp_key_next = 0;
        struct uhd_dpdk_arp_entry *arp_entry = NULL;
        while (rte_hash_iterate(port->arp_table, &arp_key,
                                (void **) &arp_entry, &arp_key_next) >= 0) {
            rte_free(arp_entry);
        }
        rte_hash_free(port->arp_table);
    }

    return 0;
}

int _uhd_dpdk_driver_main(void *arg)
{

    /* Don't currently have use for arguments */
    if (arg)
        return -EINVAL;

    /* Check that this is a valid lcore */
    unsigned int lcore_id = rte_lcore_id();
    if (lcore_id == LCORE_ID_ANY)
        return -ENODEV;

    /* Check that this lcore has ports */
    struct uhd_dpdk_thread *t = &ctx->threads[lcore_id];
    if (t->id != lcore_id)
        return -ENODEV;

    RTE_LOG(INFO, USER2, "Thread %d started\n", lcore_id);
    int status = 0;
    while (!status) {
        /* Check for open()/close() requests and service 1 at a time */
        struct uhd_dpdk_config_req *sock_req;
        if (rte_ring_dequeue(t->sock_req_ring, (void **) &sock_req) == 0) {
            /* FIXME: Not checking return vals */
            switch (sock_req->req_type) {
            case UHD_DPDK_SOCK_OPEN:
                _uhd_dpdk_sock_setup(sock_req);
                break;
            case UHD_DPDK_SOCK_CLOSE:
                _uhd_dpdk_sock_release(sock_req);
                break;
            case UHD_DPDK_LCORE_TERM:
                RTE_LOG(INFO, EAL, "Terminating lcore %u\n", lcore_id);
                status = 1;
                _uhd_dpdk_config_req_compl(sock_req, 0);
                break;
            default:
                RTE_LOG(ERR, USER2, "Invalid socket request %d\n", sock_req->req_type);
                break;
            }
        }
        /* For each port, attempt to receive packets and process */
        struct uhd_dpdk_port *port = NULL;
        LIST_FOREACH(port, &t->port_list, port_entry) {
            struct ether_hdr *hdr;
            char *l2_data;
            struct rte_mbuf *bufs[UHD_DPDK_RX_BURST_SIZE];
            const uint16_t num_rx = rte_eth_rx_burst(port->id, 0,
                                       bufs, UHD_DPDK_RX_BURST_SIZE);
            if (unlikely(num_rx == 0)) {
                 continue;
            }

            for (int buf = 0; buf < num_rx; buf++) {
                uint64_t ol_flags = bufs[buf]->ol_flags;
                hdr = rte_pktmbuf_mtod(bufs[buf], struct ether_hdr *);
                l2_data = (char *) &hdr[1];
                switch (rte_be_to_cpu_16(hdr->ether_type)) {
                case ETHER_TYPE_ARP:
                    _uhd_dpdk_process_arp(port, (struct arp_hdr *) l2_data);
                    rte_pktmbuf_free(bufs[buf]);
                    break;
                case ETHER_TYPE_IPv4:
                    if (ol_flags == PKT_RX_IP_CKSUM_BAD) { /* TODO: Track IP cksum errors? */
                        RTE_LOG(WARNING, RING, "Buf %d: Bad IP cksum\n", buf);
                    } else {
                        _uhd_dpdk_process_ipv4(port, bufs[buf], (struct ipv4_hdr *) l2_data);
                    }
                    break;
                default:
                    rte_pktmbuf_free(bufs[buf]);
                    break;
                }
            }
        }
        /* For each port's TX queues, do TX */
        LIST_FOREACH(port, &t->port_list, port_entry) {
            struct uhd_dpdk_tx_queue *q = NULL;
            LIST_FOREACH(q, &port->txq_list, entry) {
                if (!rte_ring_empty(q->retry_queue)) {
                    int num_retry = _uhd_dpdk_send(port, q, q->retry_queue);
                    _uhd_dpdk_restore_bufs(port, q, num_retry);
                    if (!rte_ring_empty(q->retry_queue)) {
                        break;
                    }
                }
                if (rte_ring_empty(q->queue)) {
                    continue;
                }
                int num_tx = _uhd_dpdk_send(port, q, q->queue);
                if (num_tx > 0) {
                    _uhd_dpdk_restore_bufs(port, q, num_tx);
                } else {
                    break;
                }
            }
        }
    }

    /* Now turn off ports */
    _uhd_dpdk_disable_ports(t);

    /* Now clean up before exiting */
    int cleaning = -EAGAIN;
    while (cleaning == -EAGAIN) {
        cleaning = _uhd_dpdk_driver_cleanup(t);
    }
    return status;
}
