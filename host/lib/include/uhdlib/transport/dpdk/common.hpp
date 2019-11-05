//
// Copyright 2019 Ettus Research, a National Instruments brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#ifndef _INCLUDED_UHDLIB_TRANSPORT_DPDK_COMMON_HPP_
#define _INCLUDED_UHDLIB_TRANSPORT_DPDK_COMMON_HPP_

#include <uhd/config.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhd/utils/noncopyable.hpp>
#include <uhd/utils/static.hpp>
#include <rte_ethdev.h>
#include <rte_ether.h>
#include <rte_flow.h>
#include <rte_mbuf.h>
#include <rte_mempool.h>
#include <rte_version.h>
#include <unordered_map>
#include <array>
#include <atomic>
#include <mutex>
#include <string>

/* NOTE: There are changes to rte_eth_addr in 19.x */

namespace uhd { namespace transport { namespace dpdk {

using queue_id_t = uint16_t;
using port_id_t  = uint16_t;
using ipv4_addr  = uint32_t;

/*!
 * Class representing a DPDK NIC port
 *
 * The dpdk_port object possesses all the data needed to send and receive
 * packets between this port and a remote host. A logical link should specify
 * which packets are destined for it and allocate a DMA queue with the
 * dpdk_port::alloc_queue() function. A logical link should not, however,
 * specify ARP packets for its set of received packets. That functionality is
 * reserved for the special queue 0.
 *
 * The logical link can then get the packet buffer pools associated with this
 * NIC port and use them to send and receive packets.
 *
 * dpdk_port objects are _only_ created by the dpdk_ctx.
 */
class dpdk_port
{
public:
    using uptr = std::unique_ptr<dpdk_port>;

    /*! Construct a DPDK NIC port and bring the link up
     *
     * \param port The port ID
     * \param mtu The intended MTU for the port
     * \param num_queues Number of DMA queues to reserve for this port
     * \param num_mbufs The number of packet buffers per queue
     * \param rx_pktbuf_pool A pointer to the port's RX packet buffer pool
     * \param tx_pktbuf_pool A pointer to the port's TX packet buffer pool
     * \param ipv4_address The IPv4 network address (w/ netmask)
     * \return A unique_ptr to a dpdk_port object
     */
    static dpdk_port::uptr make(port_id_t port,
        size_t mtu,
        uint16_t num_queues,
        size_t num_mbufs,
        struct rte_mempool* rx_pktbuf_pool,
        struct rte_mempool* tx_pktbuf_pool,
        std::string ipv4_address);

    dpdk_port(port_id_t port,
        size_t mtu,
        uint16_t num_queues,
        size_t num_mbufs,
        struct rte_mempool* rx_pktbuf_pool,
        struct rte_mempool* tx_pktbuf_pool,
        std::string ipv4_address);

    /*! Getter for this port's ID
     * \return this port's ID
     */
    inline port_id_t get_port_id() const
    {
        return _port;
    }

    /*! Getter for this port's MTU
     * \return this port's MTU
     */
    inline size_t get_mtu() const
    {
        return _mtu;
    }

    /*! Getter for this port's IPv4 address
     * \return this port's IPv4 address (in network order)
     */
    inline ipv4_addr get_ipv4() const
    {
        return _ipv4;
    }

    /*! Getter for this port's subnet mask
     * \return this port's subnet mask (in network order)
     */
    inline ipv4_addr get_netmask() const
    {
        return _netmask;
    }

    /*! Getter for this port's total DMA queue count, including initialized,
     * but unallocated queues
     *
     * \return The number of queues initialized on this port
     */
    inline size_t get_queue_count() const
    {
        return _num_queues;
    }

    /*! Getter for this port's RX packet buffer memory pool
     *
     * \return The RX packet buffer pool
     */
    inline struct rte_mempool* get_rx_pktbuf_pool() const
    {
        return _rx_pktbuf_pool;
    }

    /*! Getter for this port's TX packet buffer memory pool
     *
     * \return The TX packet buffer pool
     */
    inline struct rte_mempool* get_tx_pktbuf_pool() const
    {
        return _tx_pktbuf_pool;
    }

    /*! Determine if the destination address is a broadcast address for this port
     * \param dst_ipv4_addr The destination IPv4 address (in network order)
     * \return whether the destination address matches this port's broadcast address
     */
    inline bool dst_is_broadcast(const uint32_t dst_ipv4_addr) const
    {
        uint32_t network = _netmask | ((~_netmask) & dst_ipv4_addr);
        return (network == 0xffffffff);
    }

    /*! Allocate a DMA queue (TX/RX pair) and use the specified flow pattern
     * to route packets to the RX queue.
     *
     * \pattern recv_pattern The flow pattern to use for directing traffic to
     *                       the allocated RX queue.
     * \return The queue ID for the allocated queue
     * \throw uhd::runtime_error when there are no free queues
     */
    queue_id_t alloc_queue(struct rte_flow_pattern recv_pattern[]);

    /*! Free a previously allocated queue and tear down the associated flow rule
     * \param queue The ID of the queue to free
     * \throw std::out_of_range when the queue ID is not currently allocated
     */
    void free_queue(queue_id_t queue);

    /*!
     * Process ARP request/reply
     */
    // int process_arp(struct rte_mempool *tx_pktbuf_pool, struct arp_hdr *arp_frame);

private:
    /*!
     * Construct and transmit an ARP reply (for the given ARP request)
     */
    int _arp_reply(struct rte_mempool* tx_pktbuf_pool, struct arp_hdr* arp_req);

    port_id_t _port;
    size_t _mtu;
    struct rte_mempool* _rx_pktbuf_pool;
    struct rte_mempool* _tx_pktbuf_pool;
    struct ether_addr _mac_addr;
    ipv4_addr _ipv4;
    ipv4_addr _netmask;
    size_t _num_queues;
    std::vector<queue_id_t> _free_queues;
    std::unordered_map<queue_id_t, struct rte_flow*> _flow_rules;
    /* Need ARP table
     * To implement ARP service, maybe create ARP xport
     * Then need dpdk_udp_link and dpdk_raw_link
     *
     * ...Or just do it inline with dpdk_ctx
     *
     * And link can just save the result (do it during constructor)
     *
     *
     * But what about the service that _responds_ to ARP requests?!
     *
     * Maybe have to connect a DPDK link in stages:
     * First, create the ARP service and attach it to the dpdk_ctx
     *  dpdk_ctx must own the links...?
     *  Or! Always burn a DMA engine for ARP
     *
     * Maybe have a shared_ptr to an ARP service here?
     */
    std::mutex _mutex;
};


/*!
 * Handles initialization of DPDK, configuration of ports, setting up DMA
 * engines/queues, etc.
 */
class dpdk_ctx : uhd::noncopyable, public std::enable_shared_from_this<dpdk_ctx>
{
public:
    using sptr = std::shared_ptr<dpdk_ctx>;

    /*! Factory to generate the single dpdk_ctx instance, but with the ability
     * to release the resources
     *
     * FIXME: Without C++17, this mechanism has a race condition between
     * creating and destroying the single instance. Don't do those at the
     * same time. (shared_from_this() doesn't check the weak_ptr in C++14)
     */
    static dpdk_ctx::sptr get();

    dpdk_ctx(void);
    ~dpdk_ctx(void);

    /*!
     * Init DPDK environment, including DPDK's EAL.
     * This will make available information about the DPDK-assigned NIC devices.
     *
     * \param user_args User args passed in to override config files
     */
    void init(const device_addr_t& user_args);

    /*!
     * Get port from provided MAC address
     * \param mac_addr MAC address
     * \return pointer to port if match found, else nullptr
     */
    dpdk_port* get_port(struct ether_addr mac_addr) const;

    /*!
     * Get port structure from provided port ID
     * \param port Port ID
     * \return pointer to port if match found, else nullptr
     */
    dpdk_port* get_port(port_id_t port) const;

    /*!
     * \return Returns number of ports registered to DPDK.
     */
    int get_port_count(void);

    /*!
     * \param port_id NIC port ID
     * \return Returns number of DMA queues available for a given port
     */
    int get_port_queue_count(port_id_t portid);

    /*!
     * \param portid NIC port ID
     * \return Returns 0 if link is down, 1 if link is up
     */
    int get_port_link_status(port_id_t portid) const;

    /*!
     * Get port ID for routing packet destined for given address
     * \param addr Destination address
     * \return port ID from routing table
     */
    int get_route(const std::string& addr) const;

    /*!
     * \return whether init() has been called
     */
    bool is_init_done(void) const;

private:
    /*! Convert the args to DPDK's EAL args and Initialize the EAL
     *
     * \param eal_args The global DPDK configuration
     */
    void _eal_init(const device_addr_t& eal_args);

    /*! Either allocate or return a pointer to the RX packet buffer pool for the
     * given CPU socket
     *
     * \param cpu_socket The CPU socket ID
     * \param num_bufs Number of buffers to allocate to the pool
     * \return A pointer to the memory pool
     */
    struct rte_mempool* _get_rx_pktbuf_pool(unsigned int cpu_socket, size_t num_bufs);

    /*! Either allocate or return a pointer to the TX packet buffer pool for the
     * given CPU socket
     *
     * \param cpu_socket The CPU socket ID
     * \param num_bufs Number of buffers to allocate to the pool
     * \return A pointer to the memory pool
     */
    struct rte_mempool* _get_tx_pktbuf_pool(unsigned int cpu_socket, size_t num_bufs);

    size_t _mtu;
    int _num_mbufs;
    int _mbuf_cache_size;
    std::mutex _init_mutex;
    std::atomic<bool> _init_done;
    uhd::dict<uint32_t, port_id_t> _routes;
    std::unordered_map<port_id_t, dpdk_port::uptr> _ports;
    std::vector<struct rte_mempool*> _rx_pktbuf_pools;
    std::vector<struct rte_mempool*> _tx_pktbuf_pools;
};

}}} // namespace uhd::transport::dpdk

#endif /* _INCLUDED_UHDLIB_TRANSPORT_DPDK_COMMON_HPP_ */
