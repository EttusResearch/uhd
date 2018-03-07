//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#include "uhd_dpdk_fops.h"
#include "uhd_dpdk_udp.h"
#include "uhd_dpdk_driver.h"
#include <rte_ring.h>
#include <rte_malloc.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <arpa/inet.h>

/************************************************
 * I/O thread ONLY
 */

static int _alloc_txq(struct uhd_dpdk_port *port, pid_t tid, struct uhd_dpdk_tx_queue **queue)
{
    *queue = NULL;
    struct uhd_dpdk_tx_queue *q = rte_zmalloc(NULL, sizeof(*q), 0);
    if (!q) {
        RTE_LOG(ERR, USER1, "%s: Cannot allocate TX queue\n", __func__);
        return -ENOMEM;
    }
    q->tid = tid;
    LIST_INIT(&q->tx_list);

    char name[32];
    snprintf(name, sizeof(name), "tx_ring_udp_%u.%u", port->id, tid);
    q->queue = rte_ring_create(
                        name,
                        UHD_DPDK_TXQ_SIZE,
                        rte_socket_id(),
                        RING_F_SC_DEQ | RING_F_SP_ENQ
                    );
    snprintf(name, sizeof(name), "buffer_ring_udp_%u.%u", port->id, tid);
    q->freebufs = rte_ring_create(
                        name,
                        UHD_DPDK_TXQ_SIZE,
                        rte_socket_id(),
                        RING_F_SC_DEQ | RING_F_SP_ENQ
                    );
    /* Set up retry queue */
    snprintf(name, sizeof(name), "retry_queue_%u", port->id);
    q->retry_queue = rte_ring_create(
                               name,
                               UHD_DPDK_TXQ_SIZE,
                               rte_socket_id(),
                               RING_F_SC_DEQ | RING_F_SP_ENQ
                            );

    if (!q->queue || !q->freebufs || !q->retry_queue) {
        RTE_LOG(ERR, USER1, "%s: Cannot allocate TX rings\n", __func__);
        if (q->queue)
            rte_ring_free(q->queue);
        if (q->freebufs)
            rte_ring_free(q->freebufs);
        if (q->retry_queue)
            rte_ring_free(q->retry_queue);
        rte_free(q);
        return -ENOMEM;
    }
    struct rte_mbuf *bufs[UHD_DPDK_TXQ_SIZE];
    unsigned int num_bufs = rte_ring_free_count(q->freebufs);
    int buf_stat = rte_pktmbuf_alloc_bulk(port->parent->tx_pktbuf_pool, bufs, num_bufs);
    if (buf_stat) {
        rte_ring_free(q->freebufs);
        rte_ring_free(q->queue);
        rte_ring_free(q->retry_queue);
        rte_free(q);
        RTE_LOG(ERR, USER1, "%s: Cannot allocate packet buffers\n", __func__);
        return -ENOENT;
    }
    unsigned int enqd = rte_ring_enqueue_bulk(q->freebufs, (void **) bufs, num_bufs, NULL);
    if (enqd != num_bufs) {
        RTE_LOG(ERR, USER1, "%s: Cannot enqueue freebufs\n", __func__);
    }
    LIST_INSERT_HEAD(&port->txq_list, q, entry);
    *queue = q;
    return 0;
}

/* Finish setting up UDP socket (unless ARP needs to be done)
 * Not multi-thread safe!
 * This call should only be used by the thread servicing the port
 * In addition, the code below assumes simplex sockets and unique receive ports
 * FIXME: May need some way to help clean up abandoned socket requests (refcnt check...)
 */
int _uhd_dpdk_udp_setup(struct uhd_dpdk_config_req *req)
{
    int retval = 0;
    struct uhd_dpdk_socket *sock = req->sock;
    struct uhd_dpdk_udp_priv *pdata = sock->priv;
    struct uhd_dpdk_port *port = req->sock->port;

    struct uhd_dpdk_ipv4_5tuple ht_key = {
        .sock_type = UHD_DPDK_SOCK_UDP,
        .src_ip = 0,
        .src_port = 0,
        .dst_ip = 0,
        .dst_port = pdata->dst_port
    };

    /* Are we doing RX? */
    if (sock->rx_ring) {
        /* Add to rx table */
        if (pdata->dst_port == 0) {
            /* Assign unused one in a very slow fashion */
            for (uint16_t i = 1; i > 0; i++) {
                ht_key.dst_port = htons(i);
                if (rte_hash_lookup(port->rx_table, &ht_key) == -ENOENT) {
                    pdata->dst_port = htons(i);
                    break;
                }
            }
        }

        /* Is the port STILL invalid? */
        if (pdata->dst_port == 0) {
            RTE_LOG(ERR, USER1, "%s: No available UDP ports\n", __func__);
            _uhd_dpdk_config_req_compl(req, -EADDRINUSE);
            return -EADDRINUSE;
        }

        ht_key.dst_port = pdata->dst_port;
        if (rte_hash_lookup(port->rx_table, &ht_key) > 0) {
            RTE_LOG(ERR, USER1, "%s: Cannot add to RX table\n", __func__);
            _uhd_dpdk_config_req_compl(req, -EADDRINUSE);
            return -EADDRINUSE;
        }

        char name[32];
        snprintf(name, sizeof(name), "rx_ring_udp_%u.%u", port->id, ntohs(pdata->dst_port));
        sock->rx_ring = rte_ring_create(
                            name,
                            UHD_DPDK_RXQ_SIZE,
                            rte_socket_id(),
                            RING_F_SC_DEQ | RING_F_SP_ENQ
                        );
        if (!sock->rx_ring) {
            RTE_LOG(ERR, USER1, "%s: Cannot allocate RX ring\n", __func__);
            _uhd_dpdk_config_req_compl(req, -ENOMEM);
            return -ENOMEM;
        }
        retval = rte_hash_add_key_data(port->rx_table, &ht_key, sock);
        if (retval != 0) {
            RTE_LOG(WARNING, TABLE, "Could not add new RX socket to port %d: %d\n", port->id, retval);
            rte_ring_free(sock->rx_ring);
            _uhd_dpdk_config_req_compl(req, retval);
            return retval;
        }
        _uhd_dpdk_config_req_compl(req, 0);
    }

    /* Are we doing TX? */
    if (sock->tx_ring) {
        sock->tx_ring = NULL;
        struct uhd_dpdk_tx_queue *q = NULL;
        LIST_FOREACH(q, &port->txq_list, entry) {
            if (q->tid == sock->tid) {
                LIST_INSERT_HEAD(&q->tx_list, sock, tx_entry);
                sock->tx_ring = q->queue;
                sock->rx_ring = q->freebufs;
                break;
            }
        }
        if (!sock->tx_ring) {
            retval = _alloc_txq(port, sock->tid, &q);
            if (retval) {
                _uhd_dpdk_config_req_compl(req, retval);
                return retval;
            }
            sock->tx_ring = q->queue;
            sock->rx_ring = q->freebufs;
        }
        /* If a broadcast type, just finish setup and return */
        if (is_broadcast(port, pdata->dst_ipv4_addr)) {
            LIST_INSERT_HEAD(&q->tx_list, sock, tx_entry);
            _uhd_dpdk_config_req_compl(req, 0);
            return 0;
        }
        /* Otherwise... Check for entry in ARP table */
        struct uhd_dpdk_arp_entry *entry = NULL;
        int arp_table_stat = rte_hash_lookup_data(port->arp_table, &pdata->dst_ipv4_addr, (void **) &entry);
        if (entry) {
            /* Check for null entry */
            if ((entry->mac_addr.addr_bytes[0] == 0xFF) &&
                (entry->mac_addr.addr_bytes[1] == 0xFF) &&
                (entry->mac_addr.addr_bytes[2] == 0xFF) &&
                (entry->mac_addr.addr_bytes[3] == 0xFF) &&
                (entry->mac_addr.addr_bytes[4] == 0xFF) &&
                (entry->mac_addr.addr_bytes[5] == 0xFF)) {
                arp_table_stat = -ENOENT;
            }
        } else {
            /* No entry -> Add null entry */
            entry = rte_zmalloc(NULL, sizeof(*entry), 0);
            if (!entry) {
                RTE_LOG(ERR, USER1, "%s: Cannot allocate ARP entry\n", __func__);
                _uhd_dpdk_config_req_compl(req, -ENOMEM);
                return -ENOMEM;
            }
            memset(entry->mac_addr.addr_bytes, 0xFF, ETHER_ADDR_LEN);
            LIST_INIT(&entry->pending_list);

            if (rte_hash_add_key_data(port->arp_table, &pdata->dst_ipv4_addr, entry) < 0) {
                rte_free(entry);
                RTE_LOG(ERR, USER1, "%s: Cannot add entry to ARP table\n", __func__);
                _uhd_dpdk_config_req_compl(req, -ENOMEM);
                return -ENOMEM;
            }
        }

        /* Was there a valid address? */
        if (arp_table_stat == -ENOENT) {
            /* Get valid address and have requestor continue waiting */
            int arp_stat = 0;
            do { /* Keep trying to send request if no descriptor */
                arp_stat = _uhd_dpdk_arp_request(port, pdata->dst_ipv4_addr);
            } while (arp_stat == -EAGAIN);

            if (arp_stat) {
                /* Config request errors out */
                RTE_LOG(ERR, USER1, "%s: Cannot make ARP request\n", __func__);
                _uhd_dpdk_config_req_compl(req, arp_stat);
                return arp_stat;
            }
            /* Append req to pending list. Will signal later. */
            LIST_INSERT_HEAD(&entry->pending_list, req, entry);
            LIST_INSERT_HEAD(&q->tx_list, sock, tx_entry);
        } else {
            /* We have a valid address. All good. */
            LIST_INSERT_HEAD(&q->tx_list, sock, tx_entry);
            _uhd_dpdk_config_req_compl(req, 0);
        }
    }
    return 0;
}

int _uhd_dpdk_udp_release(struct uhd_dpdk_config_req *req)
{
    struct uhd_dpdk_socket *sock = req->sock;
    struct uhd_dpdk_port *port = req->sock->port;
    struct uhd_dpdk_config_req *conf_req = NULL;
    struct uhd_dpdk_udp_priv *pdata = (struct uhd_dpdk_udp_priv *) sock->priv;
    if (sock->tx_ring) {
        /* Remove from tx_list */
        LIST_REMOVE(sock, tx_entry);
        /* Check for entry in ARP table */
        struct uhd_dpdk_arp_entry *entry = NULL;
        rte_hash_lookup_data(port->arp_table, &pdata->dst_ipv4_addr, (void **) &entry);
        if (entry) {
            LIST_FOREACH(conf_req, &entry->pending_list, entry) {
                if (conf_req->sock == sock) {
                    LIST_REMOVE(conf_req, entry);
                    break;
                }
            }
        }

        /* Add outstanding buffers back to TX queue's freebufs */
        struct rte_mbuf *freebufs[UHD_DPDK_TXQ_SIZE];
        int status = rte_pktmbuf_alloc_bulk(port->parent->tx_pktbuf_pool, freebufs, sock->tx_buf_count);
        if (status) {
            RTE_LOG(ERR, USER1, "%d %s: Could not restore %u TX buffers in bulk!\n", status, __func__, sock->tx_buf_count);
        }

        unsigned int enqd = rte_ring_enqueue_bulk(sock->rx_ring, (void **) freebufs, sock->tx_buf_count, NULL);
        if (enqd != (unsigned int) sock->tx_buf_count) {
            RTE_LOG(ERR, USER1, "%s: Could not enqueue TX buffers!\n", __func__);
            return status;
        }
    } else if (sock->rx_ring) {
        struct uhd_dpdk_ipv4_5tuple ht_key = {
            .sock_type = UHD_DPDK_SOCK_UDP,
            .src_ip = 0,
            .src_port = 0,
            .dst_ip = 0,
            .dst_port = pdata->dst_port
        };
        rte_hash_del_key(port->rx_table, &ht_key);
        struct rte_mbuf *mbuf = NULL;
        while (rte_ring_dequeue(sock->rx_ring, (void **) &mbuf) == 0) {
            rte_pktmbuf_free(mbuf);
        }
        rte_ring_free(sock->rx_ring);
    }

    _uhd_dpdk_config_req_compl(req, 0);
    return 0;
}

/* Configure a socket for UDP
 * TODO: Make sure EVERYTHING is configured in socket
 * FIXME: Make all of this part of the user thread, except hash table interaction
 * and list handling
 */
void uhd_dpdk_udp_open(struct uhd_dpdk_config_req *req,
                       struct uhd_dpdk_sockarg_udp *arg)
{
    if (!req)
        return;

    if (!arg) {
        req->retval = -EINVAL;
        return;
    }

    struct uhd_dpdk_socket *sock = req->sock;
    pid_t tid = syscall(__NR_gettid);
    sock->tid = tid;

    /* Create private data */
    struct uhd_dpdk_udp_priv *data = (struct uhd_dpdk_udp_priv *) rte_zmalloc(NULL, sizeof(*data), 0);
    if (!data) {
        req->retval = -ENOMEM;
        return;
    }
    sock->priv = data;

    data->dst_ipv4_addr = arg->dst_addr;
    if (arg->is_tx) {
        data->src_port = arg->local_port;
        data->dst_port = arg->remote_port;
        sock->tx_ring = (struct rte_ring *) sock;
    } else {
        data->src_port = arg->remote_port;
        data->dst_port = arg->local_port;
        sock->rx_ring = (struct rte_ring *) sock;
    }

    /* TODO: Add support for I/O thread calling (skip locking and sleep) */
    /* Add to port's config queue */
    pthread_mutex_lock(&req->mutex);
    if (rte_ring_enqueue(req->sock->port->parent->sock_req_ring, req)) {
        pthread_mutex_unlock(&req->mutex);
        rte_free(data);
        req->retval = -ENOSPC;
        return;
    }
    struct timespec timeout;
    clock_gettime(CLOCK_MONOTONIC, &timeout);
    timeout.tv_sec += 1;
    pthread_cond_timedwait(&req->cond, &req->mutex, &timeout);
    pthread_mutex_unlock(&req->mutex);

    if (req->retval)
        rte_free(data);
}

void uhd_dpdk_udp_close(struct uhd_dpdk_config_req *req)
{
    if (!req)
        return;

    pthread_mutex_lock(&req->mutex);
    if (rte_ring_enqueue(req->sock->port->parent->sock_req_ring, req)) {
        pthread_mutex_unlock(&req->mutex);
        rte_free(req->sock->priv);
        req->retval = -ENOSPC;
        return;
    }
    struct timespec timeout;
    clock_gettime(CLOCK_MONOTONIC, &timeout);
    timeout.tv_sec += 1;
    pthread_cond_timedwait(&req->cond, &req->mutex, &timeout);
    pthread_mutex_unlock(&req->mutex);
    rte_free(req->sock->priv);
}

/*
 * Note: I/O thread will fill in destination MAC address (doesn't happen here)
 */
static void uhd_dpdk_ipv4_prep(struct uhd_dpdk_port *port,
                                     struct rte_mbuf *mbuf,
                                     uint32_t dst_ipv4_addr,
                                     uint8_t proto_id,
                                     uint32_t payload_len)
{
    struct ether_hdr *eth_hdr = rte_pktmbuf_mtod(mbuf, struct ether_hdr *);
    struct ipv4_hdr *ip_hdr = (struct ipv4_hdr *) &eth_hdr[1];

    ether_addr_copy(&port->mac_addr, &eth_hdr->s_addr);
    eth_hdr->ether_type = rte_cpu_to_be_16(ETHER_TYPE_IPv4);

    ip_hdr->version_ihl = 0x40 | 5;
    ip_hdr->type_of_service = 0;
    ip_hdr->total_length = rte_cpu_to_be_16(20 + payload_len);
    ip_hdr->packet_id = 0;
    ip_hdr->fragment_offset = rte_cpu_to_be_16(IPV4_HDR_DF_FLAG);
    ip_hdr->time_to_live = 64;
    ip_hdr->next_proto_id = proto_id;
    ip_hdr->hdr_checksum = 0; /* FIXME: Assuming hardware can offload */
    ip_hdr->src_addr = port->ipv4_addr;
    ip_hdr->dst_addr = dst_ipv4_addr;

    mbuf->pkt_len = sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr) + payload_len;
    mbuf->data_len = sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr) + payload_len;
}

int uhd_dpdk_udp_prep(struct uhd_dpdk_socket *sock,
                      struct rte_mbuf *mbuf)
{
    struct ether_hdr *eth_hdr;
    struct ipv4_hdr *ip_hdr;
    struct udp_hdr *tx_hdr;
    struct uhd_dpdk_port *port = sock->port;
    struct uhd_dpdk_udp_priv *pdata = (struct uhd_dpdk_udp_priv *) sock->priv;

    if (unlikely(mbuf == NULL || pdata == NULL || port == NULL))
        return -EINVAL;

    uint32_t udp_data_len = mbuf->data_len;
    uhd_dpdk_ipv4_prep(port,
                       mbuf,
                       pdata->dst_ipv4_addr,
                       0x11,
                       8 + udp_data_len);

    eth_hdr = rte_pktmbuf_mtod(mbuf, struct ether_hdr *);
    ip_hdr = (struct ipv4_hdr *) &eth_hdr[1];
    tx_hdr = (struct udp_hdr *) &ip_hdr[1];

    tx_hdr->src_port = pdata->src_port;
    tx_hdr->dst_port = pdata->dst_port;
    tx_hdr->dgram_len = rte_cpu_to_be_16(8 + udp_data_len);
    tx_hdr->dgram_cksum = 0;

    return 0;
}

int uhd_dpdk_udp_get_info(struct uhd_dpdk_socket *sock,
                          struct uhd_dpdk_sockarg_udp *sockarg)
{
    if (unlikely(sock == NULL || sockarg == NULL))
        return -EINVAL;
    if (sock->sock_type != UHD_DPDK_SOCK_UDP)
        return -EINVAL;

    struct uhd_dpdk_udp_priv *pdata = (struct uhd_dpdk_udp_priv *) sock->priv;
        if (sock->tx_ring) {
        sockarg->is_tx = true;
        sockarg->local_port = pdata->src_port;
        sockarg->remote_port = pdata->dst_port;
        sockarg->dst_addr = pdata->dst_ipv4_addr;
    } else {
        sockarg->is_tx = false;
        sockarg->local_port = pdata->dst_port;
        sockarg->remote_port = pdata->src_port;
        sockarg->dst_addr = 0;
    }
    return 0;
}

