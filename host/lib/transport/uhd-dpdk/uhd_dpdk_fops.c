//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#include "uhd_dpdk_fops.h"
#include "uhd_dpdk_udp.h"
#include "uhd_dpdk_wait.h"
#include <rte_malloc.h>
#include <rte_ip.h>

/************************************************
 * I/O thread ONLY
 *
 * TODO: Decide whether to allow blocking on mutex
 *     This would cause the I/O thread to sleep, which isn't desireable
 *     Could throw in a "request completion cleanup" section in I/O thread's
 *     main loop, though. Just keep trying until the requesting thred is woken
 *     up. This would be to handle the case where the thread hadn't finished
 *     setting itself up to wait on the condition variable, but the I/O thread
 *     still got the request.
 */
int _uhd_dpdk_config_req_compl(struct uhd_dpdk_config_req *req, int retval)
{
    req->retval = retval;
    int stat = _uhd_dpdk_waiter_wake(req->waiter, req->sock->port->parent);
    return stat;
}

int _uhd_dpdk_sock_setup(struct uhd_dpdk_config_req *req)
{
    int stat = 0;
    switch (req->sock->sock_type) {
    case UHD_DPDK_SOCK_UDP:
        stat = _uhd_dpdk_udp_setup(req);
        break;
    default:
        stat = -EINVAL;
        _uhd_dpdk_config_req_compl(req, -EINVAL);
    }
    return stat;
}

int _uhd_dpdk_sock_release(struct uhd_dpdk_config_req *req)
{
    int stat = 0;
    switch (req->sock->sock_type) {
    case UHD_DPDK_SOCK_UDP:
        stat = _uhd_dpdk_udp_release(req);
        break;
    default:
        stat = -EINVAL;
        _uhd_dpdk_config_req_compl(req, -EINVAL);
    }

    return stat;
}

int _uhd_dpdk_sock_rx_key(struct uhd_dpdk_socket *sock,
                          struct uhd_dpdk_ipv4_5tuple *key)
{
    int stat = 0;
    if (!key)
        return -EINVAL;

    switch (sock->sock_type) {
    case UHD_DPDK_SOCK_UDP:
        stat = _uhd_dpdk_udp_rx_key(sock, key);
        break;
    default:
        stat = -EINVAL;
    }
    return stat;
}
/************************************************
 * API calls
 */
struct uhd_dpdk_socket* uhd_dpdk_sock_open(unsigned int portid,
                                           enum uhd_dpdk_sock_type t, void *sockarg)
{
    if (!ctx || (t >= UHD_DPDK_SOCK_TYPE_COUNT)) {
        return NULL;
    }

    struct uhd_dpdk_port *port = find_port(portid);
    if (!port) {
        return NULL;
    }

    if (!port->ipv4_addr) {
        RTE_LOG(WARNING, EAL, "Please set IPv4 address for port %u before opening socket\n", portid);
        return NULL;
    }

    struct uhd_dpdk_config_req *req = (struct uhd_dpdk_config_req *) rte_zmalloc(NULL, sizeof(*req), 0);
    if (!req) {
        return NULL;
    }

    req->waiter = uhd_dpdk_waiter_alloc(UHD_DPDK_WAIT_SIMPLE);
    if (!req->waiter) {
        req->retval = -ENOMEM;
        goto sock_open_end;
    }

    struct uhd_dpdk_socket *s = (struct uhd_dpdk_socket *) rte_zmalloc(NULL, sizeof(*s), 0);
    if (!s) {
        goto sock_open_end;
    }

    s->port = port;
    req->sock = s;
    req->req_type = UHD_DPDK_SOCK_OPEN;
    req->sock->sock_type = t;
    req->retval = -ETIMEDOUT;

    switch (t) {
    case UHD_DPDK_SOCK_UDP:
        uhd_dpdk_udp_open(req, sockarg);
        break;
    default:
        break;
    }

    if (req->retval) {
        rte_free(s);
        s = NULL;
    }

sock_open_end:
    if (req->waiter)
        uhd_dpdk_waiter_put(req->waiter);
    rte_free(req);
    return s;
}

int uhd_dpdk_sock_close(struct uhd_dpdk_socket *sock)
{
    if (!ctx || !sock)
        return -EINVAL;

    struct uhd_dpdk_config_req *req = (struct uhd_dpdk_config_req *) rte_zmalloc(NULL, sizeof(*req), 0);
    if (!req)
        return -ENOMEM;

    req->waiter = uhd_dpdk_waiter_alloc(UHD_DPDK_WAIT_SIMPLE);
    if (!req->waiter) {
        rte_free(req);
        return -ENOMEM;
    }
    req->sock = sock;
    req->req_type = UHD_DPDK_SOCK_CLOSE;
    req->retval = -ETIMEDOUT;

    switch (sock->sock_type) {
    case UHD_DPDK_SOCK_UDP:
        uhd_dpdk_udp_close(req);
        break;
    default:
        break;
    }

    uhd_dpdk_waiter_put(req->waiter);

    if (req->retval) {
        rte_free(req);
        return req->retval;
    }

    rte_free(sock);
    return 0;
}

int uhd_dpdk_request_tx_bufs(struct uhd_dpdk_socket *sock, struct rte_mbuf **bufs,
                             unsigned int num_bufs, int timeout)
{
    if (!sock || !bufs || !num_bufs) {
        return -EINVAL;
    }
    *bufs = NULL;

    if (!sock->tx_queue)
        return -EINVAL;

    if (!sock->tx_queue->freebufs)
        return -EINVAL;

    struct rte_ring *freebufs = sock->tx_queue->freebufs;
    unsigned int num_tx = rte_ring_count(freebufs);
    if (timeout != 0 && num_tx == 0) {
        struct uhd_dpdk_wait_req *req =
            uhd_dpdk_waiter_alloc(UHD_DPDK_WAIT_TX_BUF);
        req->sock = sock;
        uhd_dpdk_waiter_wait(req, timeout, sock->port->parent);
        uhd_dpdk_waiter_put(req);
        num_tx = rte_ring_count(freebufs);
        if (!num_tx)
            return -ETIMEDOUT;
    }
    num_tx = (num_tx < num_bufs) ? num_tx : num_bufs;
    if (rte_ring_dequeue_bulk(freebufs, (void **) bufs, num_tx, NULL) == 0)
        return -ENOENT;
    sock->tx_buf_count += num_tx;
    return num_tx;
}

int uhd_dpdk_send(struct uhd_dpdk_socket *sock, struct rte_mbuf **bufs,
                  unsigned int num_bufs)
{
    if (!sock || !bufs || !num_bufs)
        return -EINVAL;
    if (!sock->tx_queue)
        return -EINVAL;
    if (!sock->tx_queue->queue)
        return -EINVAL;
    struct rte_ring *tx_ring = sock->tx_queue->queue;
    unsigned int num_tx = rte_ring_free_count(tx_ring);
    num_tx = (num_tx < num_bufs) ? num_tx : num_bufs;
    switch (sock->sock_type) {
    case UHD_DPDK_SOCK_UDP:
        for (unsigned int i = 0; i < num_tx; i++) {
            uhd_dpdk_udp_prep(sock, bufs[i]);
        }
        break;
    default:
        RTE_LOG(ERR, USER1, "%s: Unsupported sock type\n", __func__);
        return -EINVAL;
    }
    int status = rte_ring_enqueue_bulk(tx_ring, (void **) bufs, num_tx, NULL);
    if (status == 0) {
        RTE_LOG(ERR, USER1, "Invalid shared usage of TX ring detected\n");
        return status;
    }
    sock->tx_buf_count -= num_tx;
    return num_tx;
}

int uhd_dpdk_recv(struct uhd_dpdk_socket *sock, struct rte_mbuf **bufs,
                  unsigned int num_bufs, int timeout)
{
    if (!sock || !bufs || !num_bufs)
        return -EINVAL;
    if (!sock->rx_ring)
        return -EINVAL;

    unsigned int num_rx = rte_ring_count(sock->rx_ring);
    if (timeout != 0 && num_rx == 0) {
        struct uhd_dpdk_wait_req *req =
            uhd_dpdk_waiter_alloc(UHD_DPDK_WAIT_RX);
        req->sock = sock;
        uhd_dpdk_waiter_wait(req, timeout, sock->port->parent);
        uhd_dpdk_waiter_put(req);
        num_rx = rte_ring_count(sock->rx_ring);
        if (!num_rx)
            return -ETIMEDOUT;
    }

    num_rx = (num_rx < num_bufs) ? num_rx : num_bufs;
    if (num_rx) {
        unsigned int avail = 0;
        unsigned int status = rte_ring_dequeue_bulk(sock->rx_ring,
                                    (void **) bufs, num_rx, &avail);
        if (status == 0) {
            RTE_LOG(ERR, USER1, "Invalid shared usage of RX ring detected\n");
            RTE_LOG(ERR, USER1, "Requested %u, but %u available\n",
                                 num_rx, avail);
            return -ENOENT;
        }
    }
    return num_rx;
}

void uhd_dpdk_free_buf(struct rte_mbuf *buf)
{
    rte_pktmbuf_free(buf);
}

void * uhd_dpdk_buf_to_data(struct uhd_dpdk_socket *sock, struct rte_mbuf *buf)
{
    if (!sock || !buf)
        return NULL;

    /* TODO: Support for more types? */
    switch (sock->sock_type) {
    case UHD_DPDK_SOCK_UDP:
        return rte_pktmbuf_mtod_offset(buf, void *, sizeof(struct ether_hdr) +
                                                    sizeof(struct ipv4_hdr) +
                                                    sizeof(struct udp_hdr));
    default:
        return NULL;
    }
}


int uhd_dpdk_get_len(struct uhd_dpdk_socket *sock, struct rte_mbuf *buf)
{
    if (!sock || !buf)
        return -EINVAL;

    if (sock->sock_type != UHD_DPDK_SOCK_UDP)
        return -EINVAL;

    struct udp_hdr *hdr = (struct udp_hdr *) ((uint8_t *) uhd_dpdk_buf_to_data(sock, buf) - sizeof(struct udp_hdr));
    if (!hdr)
        return -EINVAL;

    /* Report dgram length - header */
    return ntohs(hdr->dgram_len) - 8;
}

int uhd_dpdk_get_src_ipv4(struct uhd_dpdk_socket *sock, struct rte_mbuf *buf,
                          uint32_t *ipv4_addr)
{
    if (!sock || !buf || !ipv4_addr)
        return -EINVAL;

    if (sock->sock_type != UHD_DPDK_SOCK_UDP)
        return -EINVAL;

    struct ipv4_hdr *hdr = rte_pktmbuf_mtod_offset(buf, struct ipv4_hdr *,
                                                   sizeof(struct ether_hdr));

    *ipv4_addr = hdr->src_addr;
    return 0;
}

int uhd_dpdk_get_drop_count(struct uhd_dpdk_socket *sock, size_t *count)
{
    if (!sock)
        return -EINVAL;
    if (sock->sock_type != UHD_DPDK_SOCK_UDP)
        return -EINVAL;
    if (!sock->priv)
        return -ENODEV;

    struct uhd_dpdk_udp_priv *pdata = (struct uhd_dpdk_udp_priv *) sock->priv;
    *count = pdata->dropped_pkts;
    return 0;
}

int uhd_dpdk_get_xfer_count(struct uhd_dpdk_socket *sock, size_t *count)
{
    if (!sock)
        return -EINVAL;
    if (sock->sock_type != UHD_DPDK_SOCK_UDP)
        return -EINVAL;
    if (!sock->priv)
        return -ENODEV;

    struct uhd_dpdk_udp_priv *pdata = (struct uhd_dpdk_udp_priv *) sock->priv;
    *count = pdata->xferd_pkts;
    return 0;
}
