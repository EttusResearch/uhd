//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#include "uhd_dpdk_ctx.h"
#include "uhd_dpdk_wait.h"
#include "uhd_dpdk_udp.h"
#include "uhd_dpdk_driver.h"
#include <stdlib.h>
#include <sched.h>
#include <rte_cycles.h>
#include <rte_errno.h>
#include <rte_malloc.h>
#include <rte_log.h>

/* FIXME: Descriptor ring size: Replace with configurable values */
#define DEFAULT_RING_SIZE 512

/* FIXME: This needs to be protected */
struct uhd_dpdk_ctx *ctx = NULL;

/**
 * TODO: Probably should provide way to get access to thread for a given port
 * UHD's first calling thread will be the master thread
 * In UHD, maybe check thread, and if it is different, pass work to that thread and optionally wait() on it (some condition variable)
 */

/* TODO: For nice scheduling options later, make sure to separate RX and TX activity */

int uhd_dpdk_port_count(void)
{
    if (!ctx)
        return -ENODEV;
    return ctx->num_ports;
}

int uhd_dpdk_port_link_status(unsigned int portid)
{
    if (!ctx)
        return -ENODEV;

    struct uhd_dpdk_port *p = find_port(portid);
    if (p) {
        struct rte_eth_link link;
        rte_eth_link_get_nowait(p->id, &link);
        return link.link_status;
    }
    return -ENODEV;
}

struct eth_addr uhd_dpdk_get_eth_addr(unsigned int portid)
{
    struct eth_addr retval;
    memset(retval.addr, 0xff, ETHER_ADDR_LEN);

    struct uhd_dpdk_port *p = find_port(portid);
    if (p) {
        memcpy(retval.addr, p->mac_addr.addr_bytes, ETHER_ADDR_LEN);
    }

    return retval;
}

int uhd_dpdk_get_ipv4_addr(unsigned int portid, uint32_t *ipv4_addr, uint32_t *netmask)
{
    if (!ipv4_addr)
        return -EINVAL;
    struct uhd_dpdk_port *p = find_port(portid);
    if (p) {
        *ipv4_addr = p->ipv4_addr;
        if (netmask) {
            *netmask = p->netmask;
        }
        return 0;
    }
    return -ENODEV;
}

int uhd_dpdk_set_ipv4_addr(unsigned int portid, uint32_t ipv4_addr, uint32_t netmask)
{
    struct uhd_dpdk_port *p = find_port(portid);
    if (p) {
        p->ipv4_addr = ipv4_addr;
        p->netmask = netmask;
        return 0;
    }
    return -ENODEV;
}

/*
 * Initialize a given port using default settings and with the RX buffers
 * coming from the mbuf_pool passed as a parameter.
 * FIXME: Starting with assumption of one thread/core per port
 */
static inline int uhd_dpdk_port_init(struct uhd_dpdk_port *port,
                                     struct rte_mempool *rx_mbuf_pool,
                                     unsigned int mtu)
{
    int retval;

    /* Check for a valid port */
    if (port->id >= rte_eth_dev_count())
        return -ENODEV;

    /* Set up Ethernet device with defaults (1 RX ring, 1 TX ring) */
    retval = rte_eth_dev_set_mtu(port->id, mtu);
    if (retval) {
        uint16_t actual_mtu;
        RTE_LOG(WARNING, EAL, "%d: Could not set mtu to %d\n", retval, mtu);
        rte_eth_dev_get_mtu(port->id, &actual_mtu);
        RTE_LOG(WARNING, EAL, "Current mtu=%d\n", actual_mtu);
        mtu = actual_mtu;
    }

    // Require checksum offloads
    struct rte_eth_dev_info dev_info;
    rte_eth_dev_info_get(port->id, &dev_info);
    uint64_t rx_offloads = DEV_RX_OFFLOAD_IPV4_CKSUM;
    uint64_t tx_offloads = DEV_TX_OFFLOAD_IPV4_CKSUM;
    if ((dev_info.rx_offload_capa & rx_offloads) != rx_offloads) {
        RTE_LOG(WARNING, EAL, "%d: Only supports RX offloads 0x%0llx\n", port->id, dev_info.rx_offload_capa);
        rte_exit(EXIT_FAILURE, "Missing required RX offloads\n");
    }
    if ((dev_info.tx_offload_capa & tx_offloads) != tx_offloads) {
        RTE_LOG(WARNING, EAL, "%d: Only supports TX offloads 0x%0llx\n", port->id, dev_info.tx_offload_capa);
        rte_exit(EXIT_FAILURE, "Missing required TX offloads\n");
    }

    struct rte_eth_conf port_conf = {
        .rxmode = {
            .offloads = rx_offloads | DEV_RX_OFFLOAD_JUMBO_FRAME,
            .max_rx_pkt_len = mtu,
            .jumbo_frame = 1,
            .hw_ip_checksum = 1,
            .ignore_offload_bitfield = 0,
        },
        .txmode = {
            .offloads = tx_offloads,
        }
    };
    retval = rte_eth_dev_configure(port->id, 1, 1, &port_conf);
    if (retval != 0)
        return retval;

    uint16_t rx_desc = DEFAULT_RING_SIZE;
    uint16_t tx_desc = DEFAULT_RING_SIZE;
    retval = rte_eth_dev_adjust_nb_rx_tx_desc(port->id, &rx_desc, &tx_desc);
    if (retval != 0)
        return retval;

    if (rx_desc != DEFAULT_RING_SIZE)
        RTE_LOG(WARNING, EAL, "RX descriptors changed to %d\n", rx_desc);
    if (tx_desc != DEFAULT_RING_SIZE)
        RTE_LOG(WARNING, EAL, "TX descriptors changed to %d\n", tx_desc);

    retval = rte_eth_rx_queue_setup(port->id, 0, rx_desc,
                 rte_eth_dev_socket_id(port->id), NULL, rx_mbuf_pool);
    if (retval < 0)
        return retval;

    struct rte_eth_txconf txconf = {
        .offloads = DEV_TX_OFFLOAD_IPV4_CKSUM
    };
    retval = rte_eth_tx_queue_setup(port->id, 0, tx_desc,
                 rte_eth_dev_socket_id(port->id), &txconf);
    if (retval < 0)
        goto port_init_fail;

    /* Create the hash table for the RX sockets */
    char name[32];
    snprintf(name, sizeof(name), "rx_table_%u", port->id);
    struct rte_hash_parameters hash_params = {
        .name = name,
        .entries = UHD_DPDK_MAX_SOCKET_CNT,
        .key_len = sizeof(struct uhd_dpdk_ipv4_5tuple),
        .hash_func = NULL,
        .hash_func_init_val = 0,
    };
    port->rx_table = rte_hash_create(&hash_params);
    if (port->rx_table == NULL) {
        retval = rte_errno;
        goto port_init_fail;
    }

    /* Create ARP table */
    snprintf(name, sizeof(name), "arp_table_%u", port->id);
    hash_params.name = name;
    hash_params.entries = UHD_DPDK_MAX_SOCKET_CNT;
    hash_params.key_len = sizeof(uint32_t);
    hash_params.hash_func = NULL;
    hash_params.hash_func_init_val = 0;
    port->arp_table = rte_hash_create(&hash_params);
    if (port->arp_table == NULL) {
        retval = rte_errno;
        goto free_rx_table;
    }

    /* Set up list for TX queues */
    LIST_INIT(&port->txq_list);

    /* Start the Ethernet port. */
    retval = rte_eth_dev_start(port->id);
    if (retval < 0) {
        goto free_arp_table;
    }

    /* Display the port MAC address. */
    rte_eth_macaddr_get(port->id, &port->mac_addr);
    RTE_LOG(INFO, EAL, "Port %u MAC: %02x %02x %02x %02x %02x %02x\n",
                (unsigned)port->id,
                port->mac_addr.addr_bytes[0], port->mac_addr.addr_bytes[1],
                port->mac_addr.addr_bytes[2], port->mac_addr.addr_bytes[3],
                port->mac_addr.addr_bytes[4], port->mac_addr.addr_bytes[5]);

    return 0;

free_arp_table:
    rte_hash_free(port->arp_table);
free_rx_table:
    rte_hash_free(port->rx_table);
port_init_fail:
    return rte_errno;
}

static int uhd_dpdk_thread_init(struct uhd_dpdk_thread *thread, unsigned int lcore)
{
    if (!ctx || !thread)
        return -EINVAL;

    unsigned int socket_id = rte_lcore_to_socket_id(lcore);
    thread->lcore = lcore;
    thread->rx_pktbuf_pool = ctx->rx_pktbuf_pools[socket_id];
    thread->tx_pktbuf_pool = ctx->tx_pktbuf_pools[socket_id];
    LIST_INIT(&thread->port_list);

    char name[32];
    snprintf(name, sizeof(name), "sockreq_ring_%u", lcore);
    thread->sock_req_ring = rte_ring_create(
                               name,
                               UHD_DPDK_MAX_PENDING_SOCK_REQS,
                               socket_id,
                               RING_F_SC_DEQ
                            );
    if (!thread->sock_req_ring)
        return -ENOMEM;
    snprintf(name, sizeof(name), "waiter_ring_%u", lcore);
    thread->waiter_ring = rte_ring_create(
                               name,
                               UHD_DPDK_MAX_WAITERS,
                               socket_id,
                               RING_F_SC_DEQ
                            );
    if (!thread->waiter_ring)
        return -ENOMEM;
    return 0;
}

int uhd_dpdk_init(int argc, const char **argv)
{
    /* Init context only once */
    if (ctx)
        return 1;

    /* Grabs arguments intended for DPDK's EAL */
    int ret = rte_eal_init(argc, (char **) argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");

    ctx = (struct uhd_dpdk_ctx *) rte_zmalloc("uhd_dpdk_ctx", sizeof(*ctx), rte_socket_id());
    if (!ctx)
        return -ENOMEM;

    ctx->num_threads = rte_lcore_count();
    if (ctx->num_threads <= 1)
        rte_exit(EXIT_FAILURE, "Error: No worker threads enabled\n");

    /* Check that we have ports to send/receive on */
    ctx->num_ports = rte_eth_dev_count();
    if (ctx->num_ports < 1)
        rte_exit(EXIT_FAILURE, "Error: Found no ports\n");

    /* Get memory for thread and port data structures */
    ctx->threads = rte_zmalloc("uhd_dpdk_thread", RTE_MAX_LCORE*sizeof(struct uhd_dpdk_thread), 0);
    if (!ctx->threads)
        rte_exit(EXIT_FAILURE, "Error: Could not allocate memory for thread data\n");
    ctx->ports = rte_zmalloc("uhd_dpdk_port", ctx->num_ports*sizeof(struct uhd_dpdk_port), 0);
    if (!ctx->ports)
        rte_exit(EXIT_FAILURE, "Error: Could not allocate memory for port data\n");

    for (size_t i = 0; i < ctx->num_ports; i++) {
        struct uhd_dpdk_port *port = &ctx->ports[i];
        port->id = i;
        rte_eth_macaddr_get(port->id, &port->mac_addr);
    }

    return 0;
}

int uhd_dpdk_start(unsigned int num_ports, int *port_thread_mapping,
                   int num_mbufs, int mbuf_cache_size, int mtu)
{
    if (!ctx)
        return -EIO;

    if ((num_ports == 0) || (port_thread_mapping == NULL)) {
        return -EINVAL;
    }

    if (ctx->num_ports < num_ports)
        rte_exit(EXIT_FAILURE, "Error: User requested more ports than available\n");

    /* Initialize the thread data structures */
    for (int i = rte_get_next_lcore(-1, 1, 0);
        (i < RTE_MAX_LCORE);
        i = rte_get_next_lcore(i, 1, 0))
    {
        /* Do one mempool of RX/TX per socket */
        unsigned int socket_id = rte_lcore_to_socket_id(i);
        /* FIXME Probably want to take into account actual number of ports per socket */
        if (ctx->tx_pktbuf_pools[socket_id] == NULL) {
            /* Creates a new mempool in memory to hold the mbufs.
             * This is done for each CPU socket
             */
            const int mbuf_size = mtu + 2048 + RTE_PKTMBUF_HEADROOM;
            char name[32];
            snprintf(name, sizeof(name), "rx_mbuf_pool_%u", socket_id);
            ctx->rx_pktbuf_pools[socket_id] = rte_pktmbuf_pool_create(
                                               name,
                                               ctx->num_ports*num_mbufs,
                                               mbuf_cache_size,
                                               0,
                                               mbuf_size,
                                               socket_id
                                           );
            snprintf(name, sizeof(name), "tx_mbuf_pool_%u", socket_id);
            ctx->tx_pktbuf_pools[socket_id] = rte_pktmbuf_pool_create(
                                               name,
                                               ctx->num_ports*num_mbufs,
                                               mbuf_cache_size,
                                               0,
                                               mbuf_size,
                                               socket_id
                                           );
            if ((ctx->rx_pktbuf_pools[socket_id]== NULL) ||
                (ctx->tx_pktbuf_pools[socket_id]== NULL))
                rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");
        }

        if (uhd_dpdk_thread_init(&ctx->threads[i], i) < 0)
            rte_exit(EXIT_FAILURE, "Error initializing thread %i\n", i);
    }

    unsigned master_lcore = rte_get_master_lcore();

    /* Assign ports to threads and initialize the port data structures */
    for (unsigned int i = 0; i < num_ports; i++) {
        int thread_id = port_thread_mapping[i];
        if (thread_id < 0)
            continue;
        if (((unsigned int) thread_id) == master_lcore)
            RTE_LOG(WARNING, EAL, "User requested master lcore for port %u\n", i);
        if (ctx->threads[thread_id].lcore != (unsigned int) thread_id)
            rte_exit(EXIT_FAILURE, "Requested inactive lcore %u for port %u\n", (unsigned int) thread_id, i);

        struct uhd_dpdk_port *port = &ctx->ports[i];
        port->parent = &ctx->threads[thread_id];
        ctx->threads[thread_id].num_ports++;
        LIST_INSERT_HEAD(&ctx->threads[thread_id].port_list, port, port_entry);

        /* Initialize port. */
        if (uhd_dpdk_port_init(port, port->parent->rx_pktbuf_pool, mtu) != 0)
            rte_exit(EXIT_FAILURE, "Cannot init port %"PRIu8 "\n",
                    i);
    }

    RTE_LOG(INFO, EAL, "Waiting for links to come up...\n");
    rte_delay_ms(1000);
    for (unsigned int i = 0; i < num_ports; i++) {
        struct uhd_dpdk_port *port = &ctx->ports[i];
        if (!port->parent)
            continue;
        struct rte_eth_link link;
        rte_eth_link_get(i, &link);
        RTE_LOG(INFO, EAL, "Port %u UP: %d, %u Mbps\n", i,
            link.link_status, link.link_speed);
    }
    RTE_LOG(INFO, EAL, "Init DONE!\n");

    /* FIXME: Create functions to do this */
    RTE_LOG(INFO, EAL, "Starting I/O threads!\n");

    cpu_set_t io_cpuset;
    CPU_ZERO(&io_cpuset);
    for (int i = rte_get_next_lcore(-1, 1, 0);
        (i < RTE_MAX_LCORE);
        i = rte_get_next_lcore(i, 1, 0))
    {
        struct uhd_dpdk_thread *t = &ctx->threads[i];
        if (!LIST_EMPTY(&t->port_list)) {
            rte_eal_remote_launch(_uhd_dpdk_driver_main, NULL, ctx->threads[i].lcore);
            struct uhd_dpdk_wait_req *waiter = uhd_dpdk_waiter_alloc(UHD_DPDK_WAIT_SIMPLE);
            if (!waiter) {
                rte_exit(EXIT_FAILURE, "%s: Failed to get wait request\n", __func__);
            }
            uhd_dpdk_waiter_get(waiter);
            uhd_dpdk_waiter_wait(waiter, -1, &ctx->threads[i]);
            uhd_dpdk_waiter_put(waiter);
            CPU_OR(&io_cpuset, &io_cpuset, &t->cpu_affinity);
       }
    }
    cpu_set_t user_cpuset;
    CPU_ZERO(&user_cpuset);
    for (int i = 0; i < CPU_SETSIZE; i++) {
        CPU_SET(i, &user_cpuset);
    }
    CPU_XOR(&user_cpuset, &user_cpuset, &io_cpuset);
    if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &user_cpuset)) {
        rte_exit(EXIT_FAILURE, "%s: Failed to set CPU affinity\n", __func__);
    }
    return 0;
}

/* FIXME: This will be changed once we have functions to handle the threads */
int uhd_dpdk_destroy(void)
{
    if (!ctx)
        return -ENODEV;

    struct uhd_dpdk_config_req *req = (struct uhd_dpdk_config_req *) rte_zmalloc(NULL, sizeof(*req), 0);
    if (!req)
        return -ENOMEM;

    req->waiter = uhd_dpdk_waiter_alloc(UHD_DPDK_WAIT_SIMPLE);
    if (!req->waiter) {
        rte_free(req);
        return -ENOMEM;
    }

    req->req_type = UHD_DPDK_LCORE_TERM;

    for (int i = rte_get_next_lcore(-1, 1, 0);
        (i < RTE_MAX_LCORE);
        i = rte_get_next_lcore(i, 1, 0))
    {
        struct uhd_dpdk_thread *t = &ctx->threads[i];

        if (LIST_EMPTY(&t->port_list))
            continue;

        if (rte_eal_get_lcore_state(t->lcore) == FINISHED)
            continue;

        if (rte_ring_enqueue(t->sock_req_ring, req)) {
            RTE_LOG(ERR, USER2, "Failed to terminate thread %d\n", i);
            rte_free(req->waiter);
            rte_free(req);
            return -ENOSPC;
        }
        uhd_dpdk_config_req_submit(req, 1, t);
    }

    uhd_dpdk_waiter_put(req->waiter);
    rte_free(req);
    return 0;
}

