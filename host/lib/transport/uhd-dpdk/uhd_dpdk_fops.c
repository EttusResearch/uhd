//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#include "uhd_dpdk_fops.h"
#include "uhd_dpdk_udp.h"
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
    int stat = pthread_mutex_trylock(&req->mutex);
    if (stat) {
        RTE_LOG(ERR, USER1, "%s: Could not lock req mutex\n", __func__);
        return stat;
    }
    stat = pthread_cond_signal(&req->cond);
    pthread_mutex_unlock(&req->mutex);
    if (stat) {
        RTE_LOG(ERR, USER1, "%s: Could not signal req cond\n", __func__);
        return stat;
    }
    return 0;
}

int _uhd_dpdk_sock_setup(struct uhd_dpdk_config_req *req)
{
    int stat = 0;
    switch (req->sock_type) {
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
    switch (req->sock_type) {
    case UHD_DPDK_SOCK_UDP:
        stat = _uhd_dpdk_udp_release(req);
        break;
    default:
        stat = -EINVAL;
        _uhd_dpdk_config_req_compl(req, -EINVAL);
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

    struct uhd_dpdk_socket *s = (struct uhd_dpdk_socket *) rte_zmalloc(NULL, sizeof(*s), 0);
    if (!s) {
        goto sock_open_end;
    }

    s->port = port;
    req->sock = s;
    req->req_type = UHD_DPDK_SOCK_OPEN;
    req->sock_type = t;
    req->retval = -ETIMEDOUT;
    pthread_mutex_init(&req->mutex, NULL);
    pthread_condattr_t condattr;
    pthread_condattr_init(&condattr);
    pthread_condattr_setclock(&condattr, CLOCK_MONOTONIC);
    pthread_cond_init(&req->cond, &condattr);

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
    req->sock = sock;
    req->req_type = UHD_DPDK_SOCK_CLOSE;
    req->sock_type = sock->sock_type;
    req->retval = -ETIMEDOUT;
    pthread_mutex_init(&req->mutex, NULL);
    pthread_cond_init(&req->cond, NULL);

    switch (sock->sock_type) {
    case UHD_DPDK_SOCK_UDP:
        uhd_dpdk_udp_close(req);
        break;
    default:
        break;
    }

    if (req->retval) {
        rte_free(req);
        return req->retval;
    }

    rte_free(sock);
    return 0;
}

/*
 * TODO:
 *     Add blocking calls with timeout
 *     Implementation would involve a condition variable, like config reqs
 *     Also would create a cleanup section in I/O main loop (like config reqs)
 */
int uhd_dpdk_request_tx_bufs(struct uhd_dpdk_socket *sock, struct rte_mbuf **bufs,
                             unsigned int num_bufs)
{
    if (!sock || !bufs || !num_bufs) {
        return -EINVAL;
    }
    *bufs = NULL;

    if (!sock->tx_ring)
        return -EINVAL;

    unsigned int num_tx = rte_ring_count(sock->rx_ring);
    num_tx = (num_tx < num_bufs) ? num_tx : num_bufs;
    if (rte_ring_dequeue_bulk(sock->rx_ring, (void **) bufs, num_tx, NULL) == 0)
        return -ENOENT;
    sock->tx_buf_count += num_tx;
    return num_tx;
}

int uhd_dpdk_send(struct uhd_dpdk_socket *sock, struct rte_mbuf **bufs,
                  unsigned int num_bufs)
{
    if (!sock || !bufs || !num_bufs)
        return -EINVAL;
    if (!sock->tx_ring)
        return -EINVAL;
    unsigned int num_tx = rte_ring_free_count(sock->tx_ring);
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
    int status = rte_ring_enqueue_bulk(sock->tx_ring, (void **) bufs, num_tx, NULL);
    if (status == 0) {
        RTE_LOG(ERR, USER1, "Invalid shared usage of TX ring detected\n");
        return status;
    }
    sock->tx_buf_count -= num_tx;
    return num_tx;
}

/*
 * TODO:
 *     Add blocking calls with timeout
 */
int uhd_dpdk_recv(struct uhd_dpdk_socket *sock, struct rte_mbuf **bufs,
                  unsigned int num_bufs, unsigned int timeout)
{
    if (!sock || !bufs || !num_bufs)
        return -EINVAL;
    if (!sock->rx_ring)
        return -EINVAL;
    unsigned int num_rx = rte_ring_count(sock->rx_ring);
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

int uhd_dpdk_get_drop_count(struct uhd_dpdk_socket *sock, uint32_t *count)
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
