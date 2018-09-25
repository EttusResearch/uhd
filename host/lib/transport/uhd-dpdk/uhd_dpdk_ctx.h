//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#ifndef _UHD_DPDK_CTX_H_
#define _UHD_DPDK_CTX_H_

#include <stdint.h>
#include <sys/queue.h>
#include <sys/types.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_hash.h>
#include <rte_eal.h>
#include <rte_atomic.h>
#include <uhdlib/transport/uhd-dpdk.h>
//#include <pthread.h>

/* For nice scheduling options later, make sure to separate RX and TX activity */

#define UHD_DPDK_MAX_SOCKET_CNT 1024
#define UHD_DPDK_MAX_PENDING_SOCK_REQS 16
#define UHD_DPDK_MAX_WAITERS UHD_DPDK_MAX_SOCKET_CNT
#define UHD_DPDK_TXQ_SIZE 64
#define UHD_DPDK_TX_BURST_SIZE (UHD_DPDK_TXQ_SIZE - 1)
#define UHD_DPDK_RXQ_SIZE 128
#define UHD_DPDK_RX_BURST_SIZE (UHD_DPDK_RXQ_SIZE - 1)

struct uhd_dpdk_port;
struct uhd_dpdk_tx_queue;

/**
 *
 * All memory allocation for port, rx_ring, and tx_queue owned by I/O thread
 * Rest owned by user thread
 *
 * port: port servicing this socket
 * tid: thread ID that owns this socket (to be associated with TX queue)
 * sock_type: Type of socket
 * priv: Private data, based on sock_type
 * rx_ring: pointer to individual rx_ring (created during init--Also used as free buffer ring for TX)
 * tx_queue: pointer to tx queue structure
 * tx_buf_count: Number of buffers currently outside the rings
 * tx_entry: List node for TX Queue tracking
 *
 * If a user closes a socket without outstanding TX buffers, user must free the
 * buffers. Otherwise, that memory will be leaked, and usage will grow.
 */
struct uhd_dpdk_socket {
    struct uhd_dpdk_port *port;
    pthread_t tid;
    enum uhd_dpdk_sock_type sock_type;
    void *priv;
    struct rte_ring *rx_ring;
    struct uhd_dpdk_tx_queue *tx_queue;
    int tx_buf_count;
    LIST_ENTRY(uhd_dpdk_socket) tx_entry;
};
LIST_HEAD(uhd_dpdk_tx_head, uhd_dpdk_socket);

/************************************************
 * Configuration and Blocking
 ************************************************/
struct uhd_dpdk_wait_req;

enum uhd_dpdk_sock_req {
    UHD_DPDK_SOCK_OPEN = 0,
    UHD_DPDK_SOCK_CLOSE,
    UHD_DPDK_LCORE_TERM,
    UHD_DPDK_SOCK_REQ_COUNT
};

/**
 * port: port associated with this request
 * sock: socket associated with this request
 * req_type: Open, Close, or terminate lcore
 * cond: Used to sleep until socket creation is finished
 * mutex: associated with cond
 * entry: List node for requests pending ARP responses
 * priv: private data
 * retval: Result of call (needed post-wakeup)
 *
 * config_reqs are assumed not to time out
 * The interaction with wait_reqs currently makes this impossible to do safely
 */
struct uhd_dpdk_config_req {
    struct uhd_dpdk_port *port;
    struct uhd_dpdk_socket *sock;
    enum uhd_dpdk_sock_req req_type;
    struct uhd_dpdk_wait_req *waiter;
    LIST_ENTRY(uhd_dpdk_config_req) entry;
    void *priv;
    int retval;
};
LIST_HEAD(uhd_dpdk_config_head, uhd_dpdk_config_req);

/************************************************
 * RX Table
 ************************************************/
struct uhd_dpdk_arp_entry {
    struct ether_addr mac_addr;
    struct uhd_dpdk_config_head pending_list; /* Config reqs pending ARP--Thread-unsafe */
};

struct uhd_dpdk_ipv4_5tuple {
    enum uhd_dpdk_sock_type sock_type;
    uint32_t src_ip;
    uint32_t dst_ip;
    uint16_t src_port;
    uint16_t dst_port;
};

/**
 * Entry for RX table
 * req used for blocking calls to RX
 */
struct uhd_dpdk_rx_entry {
    struct uhd_dpdk_socket *sock;
    struct uhd_dpdk_wait_req *waiter;
};

/************************************************
 * TX Queues
 *
 * 1 TX Queue per socket sending through a hardware port
 * All memory allocation owned by I/O thread
 *
 * tid: thread id
 * queue: TX queue holding threads prepared packets (via send())
 * retry_queue: queue holding packets that couldn't be sent
 * freebufs: queue holding empty buffers
 * waiter: Request to wait for a free buffer
 * tx_list: list of sockets using this queue
 * entry: list node for port to track TX queues
 *
 * queue, retry_queue, and freebufs are single-producer, single-consumer queues
 * retry_queue wholly-owned by I/O thread
 * For queue, user thread is producer, I/O thread is consumer
 * For freebufs, user thread is consumer, I/O thread is consumer
 *
 * All queues are same size
 * 1. Buffers start in freebufs (user gets buffers from freebufs)
 * 2. User submits packet to queue
 * 3. If packet couldn't be sent, it is (re)enqueued on retry_queue
 ************************************************/
struct uhd_dpdk_tx_queue {
    pthread_t tid;
    struct rte_ring *queue;
    struct rte_ring *retry_queue;
    struct rte_ring *freebufs;
    struct uhd_dpdk_wait_req *waiter;
    struct uhd_dpdk_tx_head tx_list;
    LIST_ENTRY(uhd_dpdk_tx_queue) entry;
};
LIST_HEAD(uhd_dpdk_txq_head, uhd_dpdk_tx_queue);

/************************************************
 * Port structure
 *
 * All memory allocation owned by I/O thread
 *
 * id: hardware port id (for DPDK)
 * parent: I/O thread servicing this port
 * mac_addr: MAC address of this port
 * ipv4_addr: IPv4 address of this port
 * netmask: Subnet mask of this port
 * arp_table: ARP cache for this port
 * rx_table: Mapping of 5-tuple key to sockets for RX
 * txq_list: List of TX queues associated with this port
 * port_entry: List node entry for I/O thread to track
 ************************************************/
struct uhd_dpdk_port {
    unsigned int id;
    struct uhd_dpdk_thread *parent;
    struct ether_addr mac_addr;
    uint32_t ipv4_addr; /* FIXME: Check this before allowing a socket!!! */
    uint32_t netmask;
    /* Key = IP addr
     * Value = MAC addr (ptr to uhd_dpdk_arp_entry)
     */
    struct rte_hash *arp_table;
    /* hash map of RX sockets
     * Key = uhd_dpdk_ipv4_5tuple
     * Value = uhd_dpdk_socket
     */
    struct rte_hash *rx_table;
    /* doubly-linked list of TX sockets */
    struct uhd_dpdk_txq_head txq_list;
    LIST_ENTRY(uhd_dpdk_port) port_entry;
};

LIST_HEAD(uhd_dpdk_port_head, uhd_dpdk_port);

/************************************************
 * Thread/lcore-private data structure
 *
 * All data owned by global context
 *
 * id: lcore id (from DPDK)
 * rx_pktbuf_pool: memory pool for generating buffers for RX packets
 * tx_pktbuf_pool: memory pool for generating buffers for TX packets
 * num_ports: Number of ports this lcore is servicing
 * port_list: List of ports this lcore is servicing
 * sock_req_ring: Queue for user threads to submit service requests to the lcore
 *
 * sock_req_ring is a multi-producer, single-consumer queue
 * It must NOT BE ACCESSED SIMULTANEOUSLY by two threads not using SCHED_OTHER(cfs)
 *
 * For threads that have ports:
 * Launch individually
 * For threads without ports:
 * Do not launch unless user specifically does it themselves.
 * Should also have master lcore returned to user
 * REMEMBER: Without args, DPDK creates an lcore for each CPU core!
 */
struct uhd_dpdk_thread {
    unsigned int lcore;
    cpu_set_t cpu_affinity;
    struct rte_mempool *rx_pktbuf_pool;
    struct rte_mempool *tx_pktbuf_pool;
    int num_ports;
    struct uhd_dpdk_port_head port_list;
    struct rte_ring *sock_req_ring;
    struct rte_ring *waiter_ring;
};


/************************************************
 * One global context
 *
 * num_threads: Number of DPDK lcores tracked
 * num_ports: Number of DPDK/NIC ports tracked
 * threads: Array of all lcores/threads
 * ports: Array of all DPDK/NIC ports
 * rx_pktbuf_pools: Array of all packet buffer pools for RX
 * tx_pktbuf_pools: Array of all packet buffer pools for TX
 *
 * The packet buffer pools are memory pools that are associated with a CPU
 * socket. They will provide storage close to the socket to accommodate NUMA
 * nodes.
 ************************************************/
struct uhd_dpdk_ctx {
    unsigned int num_threads;
    unsigned int num_ports;
    struct uhd_dpdk_thread *threads;
    struct uhd_dpdk_port *ports;
    struct rte_mempool *rx_pktbuf_pools[RTE_MAX_NUMA_NODES];
    struct rte_mempool *tx_pktbuf_pools[RTE_MAX_NUMA_NODES];
};

extern struct uhd_dpdk_ctx *ctx;

static inline struct uhd_dpdk_port * find_port(unsigned int portid)
{
    if (!ctx)
        return NULL;

    for (unsigned int i = 0; i < ctx->num_ports; i++) {
        struct uhd_dpdk_port *p = &ctx->ports[i];
        if (p->id == portid) {
            return p;
        }
    }
    return NULL;
}

#endif /* _UHD_DPDK_CTX_H_ */
