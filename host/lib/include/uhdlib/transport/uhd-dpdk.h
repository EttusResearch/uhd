//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#ifndef _UHD_DPDK_H_
#define _UHD_DPDK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <rte_mbuf.h>

/* For MAC address */
struct eth_addr {
    uint8_t addr[6];
};

/* Opaque type representing a socket
 * May NOT be shared between threads
 */
struct uhd_dpdk_socket;

/* Only support UDP sockets currently */
enum uhd_dpdk_sock_type {
    UHD_DPDK_SOCK_UDP = 0,
    UHD_DPDK_SOCK_TYPE_COUNT
};

/**
 * Init UHD-DPDK environment, including DPDK's EAL.
 * This will make available information about the DPDK-assigned NIC devices.
 * 
 * @param argc passed directly to rte_eal_init()
 * @param argv passed directly to rte_eal_init()
 *
 * @return Returns negative error code if there were issues, else 0
 */
int uhd_dpdk_init(int argc, const char **argv);

/**
 * Start UHD-DPDK networking stack. Bring ports up (link UP).
 * uhd_dpdk_init() must be called first.
 *
 * Offload capabilities will be used if available
 *
 * @param num_ports number of network interfaces to map
 * @param port_thread_mapping array of num_ports entries specifying which thread
 *     will drive the I/O for a given port (determined by array index)
 * @param num_mbufs number of packets in each packet buffer pool (multiplied by num_ports)
 *     There is one RX and one TX buffer pool per CPU socket
 * @param mbuf_cache_size Number of packet buffers to put in core-local cache
 * @param mtu Maximum frame size
 *
 * @return Returns negative error code if there were issues, else 0
 */
int uhd_dpdk_start(unsigned int num_ports, int *port_thread_mapping,
                   int num_mbufs, int mbuf_cache_size, int mtu);

/**
 * @return Returns number of ports registered to DPDK.
 *         Returns negative error value if uhd-dpdk hasn't been init'd
 */
int uhd_dpdk_port_count(void);

/**
 * @return Returns 0 if link is down, 1 if link is up, and negative error code
 *         if error occurred.
 */
int uhd_dpdk_port_link_status(unsigned int portid);

/**
 * @return Returns Ethernet MAC address of requested port
 *
 * @param portid ID number of network interface
 */
struct eth_addr uhd_dpdk_get_eth_addr(unsigned int portid);

/**
 * Get IPv4 address of requested port
 *
 * @param portid ID number of network interface
 * @param ipv4_addr pointer to uint32_t where ipv4 address is stored
 *     Must be non-NULL
 * @param netmask pointer to uint32_t where netmask is stored
 *     May be left NULL
 *
 * @return Returns
 *  0 = success
 *  nonzero = failure
 */
int uhd_dpdk_get_ipv4_addr(unsigned int portid, uint32_t *ipv4_addr, uint32_t *netmask);

/**
 * Sets IPv4 address of requested port
 *
 * @param portid ID number of network interface
 * @param ipv4_addr must be in network format
 * @param netmask must be in network format
 *
 * @return Return values:
 * 0 = success
 * nonzero = failure
 */
int uhd_dpdk_set_ipv4_addr(unsigned int portid, uint32_t ipv4_addr, uint32_t netmask);

/**
 * Create new socket of type sock_type on port portid
 * Copies needed info from sockarg
 * Do NOT share struct uhd_dpdk_socket between threads!
 *
 * @param portid ID number of network interface
 * @param t Type of socket to create (only UDP supported currently)
 * @param sockarg Pointer to arguments for corresponding socket type
 *
 * @return Returns pointer to socket structure on success, else NULL
 */
struct uhd_dpdk_socket* uhd_dpdk_sock_open(unsigned int portid,
                                           enum uhd_dpdk_sock_type t, void *sockarg);

/**
 * Close socket created by uhd_dpdk_sock_open
 *
 * Note: Outstanding packet buffers must still be freed by user
 *
 * @param sock Socket to close
 *
 * @return Returns
 *  0 = success
 *  nonzero = failure
 */
int uhd_dpdk_sock_close(struct uhd_dpdk_socket *sock);

/**
 * Arguments for a UDP socket
 * All address/port data should be provided in network format
 */
struct uhd_dpdk_sockarg_udp {
    /*! True for TX socket, false for RX socket */
    bool     is_tx;
    /*! True to filter broadcast packets, else recv */
    bool     filter_bcast;
    /*! Local udp port. This is dst_port for RX, src_port for TX */
    uint16_t local_port;
    /*! Remote udp port. This is dst_port for TX */
    uint16_t remote_port;
    /*! IPv4 address for destination (TX) */
    uint32_t dst_addr;
    /*! Number of buffers in ring */
    size_t   num_bufs;
};

/**
 * Brings all ports and threads down in preparation for a clean program exit
 *
 * All sockets will need to be closed by the user for a thread to terminate in
 * this function.
 */
int uhd_dpdk_destroy(void);

/**
 * Requests num_bufs buffers from sock. Places pointers to buffers in bufs table.
 *
 * @param sock pointer to socket
 * @param bufs pointer to array of buffers (to store buffer locations)
 * @param num_bufs number of buffers requested
 * @param timeout Time (in us) to wait for a buffer
 *
 * @return Returns number of buffers retrieved or negative error code
 */
int uhd_dpdk_request_tx_bufs(struct uhd_dpdk_socket *sock, struct rte_mbuf **bufs, unsigned int num_bufs, int timeout);

/**
 * Enqueues num_bufs buffers in sock TX buffer. Uses pointers to buffers in bufs table.
 *
 * @param sock pointer to socket
 * @param bufs pointer to array of buffers (to retrieve buffer locations)
 * @param num_bufs number of buffers requested
 *
 * @return Returns number of buffers enqueued or negative error code
 */
int uhd_dpdk_send(struct uhd_dpdk_socket *sock, struct rte_mbuf **bufs, unsigned int num_bufs);

/**
 * Dequeues num_bufs buffers from sock RX buffer. Uses pointers to buffers in bufs table.
 *
 * @param sock pointer to socket
 * @param bufs pointer to array of buffers (to store buffer locations)
 * @param num_bufs number of buffers requested
 * @param timeout Time (in us) to wait for a packet
 *
 * @return Returns number of buffers dequeued or negative error code
 *
 * NOTE: MUST free buffers with uhd_dpdk_free_buf once finished
 */
int uhd_dpdk_recv(struct uhd_dpdk_socket *sock, struct rte_mbuf **bufs,
                  unsigned int num_bufs, int timeout);

/**
 * Frees buffer previously received from uhd_dpdk_recv
 *      (or unused ones from uhd_dpdk_request_tx_bufs)
 *
 * @param buf pointer to packet buffer
 */
void uhd_dpdk_free_buf(struct rte_mbuf *buf);

/**
 * Returns pointer to start of data segment of packet buffer
 *
 * @param sock Socket associated with packet buffer
 * @param buf pointer to packet buffer
 */
void * uhd_dpdk_buf_to_data(struct uhd_dpdk_socket *sock, struct rte_mbuf *buf);

/**
 * Returns size of data segment of packet buffer (in bytes)
 *
 * This is protocol-dependent. A UDP socket will return the UDP payload size.
 *
 * @param sock Socket associated with packet buffer
 * @param buf pointer to packet buffer
 *
 * @return Return 0 for success, else failed
 */
int uhd_dpdk_get_len(struct uhd_dpdk_socket *sock, struct rte_mbuf *buf);

/**
 * Get IPv4 address of sender (for UDP RX socket)
 *
 * @param sock Socket associated with packet buffer
 * @param buf pointer to packet buffer
 * @param ipv4_addr pointer to buffer where ipv4 address will be written
 *
 * @return Return 0 for success, else failed
 */
int uhd_dpdk_get_src_ipv4(struct uhd_dpdk_socket *sock, struct rte_mbuf *buf,
                          uint32_t *ipv4_addr);

/**
 * Get info (local port, remote port, dst addr, etc.) for UDP socket
 *
 * @param sock Socket to get information from
 * @param sockarg Pointer to location where information will be stored
 *
 * @return Return 0 for success, else failed
 */
int uhd_dpdk_udp_get_info(struct uhd_dpdk_socket *sock, struct uhd_dpdk_sockarg_udp *sockarg);


/***********************************************
 * Statistics
 ***********************************************/
/**
 * Get dropped packet count of provided socket
 *
 * @param sock Socket to get information from
 * @param count Pointer to location where information will be stored
 *
 * @return Return 0 for success, else failed
 */
int uhd_dpdk_get_drop_count(struct uhd_dpdk_socket *sock, size_t *count);

/**
 * Get transferred packet count of provided socket
 * Currently only tracks received packets (i.e. for RX)
 *
 * @param sock Socket to get information from
 * @param count Pointer to location where information will be stored
 *
 * @return Return 0 for success, else failed
 */
int uhd_dpdk_get_xfer_count(struct uhd_dpdk_socket *sock, size_t *count);

#ifdef __cplusplus
}
#endif
#endif /* _UHD_DPDK_H_ */
