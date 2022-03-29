//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/transport/frame_buff.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhdlib/transport/adapter_info.hpp>
#include <rte_ethdev.h>
#include <rte_ether.h>
#include <rte_flow.h>
#include <rte_mbuf.h>
#include <rte_mempool.h>
#include <rte_spinlock.h>
#include <rte_version.h>
#include <unordered_map>
#include <array>
#include <atomic>
#include <mutex>
#include <set>
#include <string>

/* NOTE: There are changes to all the network standard fields in 19.x */

/*
 * Substituting old values to support DPDK 18.11 with the API changes in DPDK 19.
 * There were no functional changes in the DPDK calls utilized by UHD between
 * these two API versions.
 */
#if RTE_VER_YEAR < 19 || (RTE_VER_YEAR == 19 && RTE_VER_MONTH < 8)
// 18.11 data types
#define rte_ether_addr ether_addr
#define rte_ether_hdr  ether_hdr
#define rte_arp_hdr    arp_hdr
#define arp_hardware   arp_hrd
#define arp_protocol   arp_pro
#define arp_hlen       arp_hln
#define arp_plen       arp_pln
#define arp_opcode     arp_op
#define rte_ipv4_hdr   ipv4_hdr
#define rte_udp_hdr    udp_hdr
// 18.11 constants
#define RTE_ETHER_TYPE_IPV4  ETHER_TYPE_IPv4
#define RTE_ETHER_TYPE_ARP   ETHER_TYPE_ARP
#define RTE_ETHER_ADDR_LEN   ETHER_ADDR_LEN
#define RTE_ETHER_CRC_LEN    ETHER_CRC_LEN
#define RTE_ETHER_HDR_LEN    ETHER_HDR_LEN
#define RTE_IPV4_HDR_DF_FLAG IPV4_HDR_DF_FLAG
#define RTE_ARP_HRD_ETHER    ARP_HRD_ETHER
#define RTE_ARP_OP_REPLY     ARP_OP_REPLY
#define RTE_ARP_OP_REQUEST   ARP_OP_REQUEST
// 18.11 functions
#define rte_ether_addr_copy    ether_addr_copy
#define rte_ether_format_addr  ether_format_addr
#define rte_is_zero_ether_addr is_zero_ether_addr
#endif

namespace uhd { namespace transport {

class dpdk_io_service;

namespace dpdk {

struct arp_entry;

using queue_id_t = uint16_t;
using port_id_t  = uint16_t;
using rte_ipv4_addr  = uint32_t;

class dpdk_adapter_info : public adapter_info
{
public:
    dpdk_adapter_info(port_id_t port) : _port(port) {}
    ~dpdk_adapter_info() {}

    std::string to_string()
    {
        return std::string("DPDK:") + std::to_string(_port);
    }

    bool operator==(const dpdk_adapter_info& rhs) const
    {
        return (_port == rhs._port);
    }

private:
    // Port ID
    port_id_t _port;
};


/*!
 * Packet/Frame buffer class for DPDK
 *
 * This class is intended to be placed in the private area of the rte_mbuf, and
 * its memory is part of the rte_mbuf, so its life is tied to the underlying
 * buffer (or more precisely, the encapsulating one).
 */
class dpdk_frame_buff : public frame_buff
{
public:
    dpdk_frame_buff(struct rte_mbuf* mbuf) : _mbuf(mbuf)
    {
        _data        = rte_pktmbuf_mtod(mbuf, void*);
        _packet_size = 0;
    }

    ~dpdk_frame_buff() = default;

    /*!
     * Simple getter for the underlying rte_mbuf.
     * The rte_mbuf may need further modification before sending packets,
     * like adjusting the IP and UDP lengths.
     */
    inline struct rte_mbuf* get_pktmbuf()
    {
        return _mbuf;
    }

    /*!
     * Move the data pointer by the indicated size, to some desired
     * encapsulated frame.
     *
     * \param hdr_size Size (in bytes) of the headers to skip. Can be negative
     *                 to pull the header back.
     */
    inline void header_jump(ssize_t hdr_size)
    {
        _data = (void*)((uint8_t*)_data + hdr_size);
    }

    //! Embedded list node's next ptr
    dpdk_frame_buff* next = nullptr;
    //! Embedded list node's prev ptr
    dpdk_frame_buff* prev = nullptr;

private:
    struct rte_mbuf* _mbuf;
};


/*!
 * The size (in bytes) of the private area reserved within the rte_mbuf.
 * This portion of the rte_mbuf is used for the embedded dpdk_frame_buff data
 * structure.
 */
constexpr size_t DPDK_MBUF_PRIV_SIZE =
    RTE_ALIGN(sizeof(struct dpdk_frame_buff), RTE_MBUF_PRIV_ALIGN);

/*!
 * Class representing a DPDK NIC port
 *
 * The dpdk_port object possesses all the data needed to send and receive
 * packets between this port and a remote host.
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
     * \param num_desc The number of descriptors per DMA queue
     * \param rx_pktbuf_pool A pointer to the port's RX packet buffer pool
     * \param tx_pktbuf_pool A pointer to the port's TX packet buffer pool
     * \param rte_ipv4_address The IPv4 network address (w/ netmask)
     * \return A unique_ptr to a dpdk_port object
     */
    static dpdk_port::uptr make(port_id_t port,
        size_t mtu,
        uint16_t num_queues,
        uint16_t num_desc,
        struct rte_mempool* rx_pktbuf_pool,
        struct rte_mempool* tx_pktbuf_pool,
        std::string rte_ipv4_address);

    dpdk_port(port_id_t port,
        size_t mtu,
        uint16_t num_queues,
        uint16_t num_desc,
        struct rte_mempool* rx_pktbuf_pool,
        struct rte_mempool* tx_pktbuf_pool,
        std::string rte_ipv4_address);

    ~dpdk_port();

    /*! Getter for this port's ID
     * \return this port's ID
     */
    inline port_id_t get_port_id() const
    {
        return _port;
    }

    inline dpdk_adapter_info get_adapter_info() const
    {
        return dpdk_adapter_info(_port);
    }

    /*! Getter for this port's MAC address
     * \return this port's MAC address
     */
    inline rte_ether_addr get_mac_addr() const
    {
        return _mac_addr;
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
    inline rte_ipv4_addr get_ipv4() const
    {
        return _ipv4;
    }

    /*! Getter for this port's subnet mask
     * \return this port's subnet mask (in network order)
     */
    inline rte_ipv4_addr get_netmask() const
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
     * \param dst_rte_ipv4_addr The destination IPv4 address (in network order)
     * \return whether the destination address matches this port's broadcast address
     */
    inline bool dst_is_broadcast(const rte_ipv4_addr dst_rte_ipv4_addr) const
    {
        uint32_t network = _netmask | ((~_netmask) & dst_rte_ipv4_addr);
        return (network == 0xffffffff);
    }

    /*!
     * Allocate a UDP port and return it in network order
     *
     * \param udp_port UDP port to attempt to allocate. Use 0 for no preference.
     * \return 0 for failure, else the allocated UDP port in network order.
     */
    uint16_t alloc_udp_port(uint16_t udp_port);

private:
    friend uhd::transport::dpdk_io_service;

    /*!
     * Construct and transmit an ARP reply (for the given ARP request)
     */
    int _arp_reply(queue_id_t queue_id, struct rte_arp_hdr* arp_req);

    port_id_t _port;
    size_t _mtu;
    size_t _num_queues;
    struct rte_mempool* _rx_pktbuf_pool;
    struct rte_mempool* _tx_pktbuf_pool;
    struct rte_ether_addr _mac_addr;
    rte_ipv4_addr _ipv4;
    rte_ipv4_addr _netmask;

    // Structures protected by mutex
    std::mutex _mutex;
    std::set<uint16_t> _udp_ports;
    uint16_t _next_udp_port = 0xffff;

    // Structures protected by spin lock
    rte_spinlock_t _spinlock = RTE_SPINLOCK_INITIALIZER;
    std::unordered_map<rte_ipv4_addr, struct arp_entry*> _arp_table;
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
    dpdk_port* get_port(struct rte_ether_addr mac_addr) const;

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
     * Get port for routing packet destined for given address
     * \param addr Destination address
     * \return pointer to the port from routing table
     */
    dpdk_port* get_route(const std::string& addr) const;

    /*!
     * \return whether init() has been called
     */
    bool is_init_done(void) const;

    /*! Return a reference to an IO service given a port ID
     */
    std::shared_ptr<uhd::transport::dpdk_io_service> get_io_service(const size_t port_id);

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
    int _link_init_timeout;
    std::mutex _init_mutex;
    std::atomic<bool> _init_done;
    uhd::dict<uint32_t, port_id_t> _routes;
    std::unordered_map<port_id_t, dpdk_port::uptr> _ports;
    std::vector<struct rte_mempool*> _rx_pktbuf_pools;
    std::vector<struct rte_mempool*> _tx_pktbuf_pools;
    // Store all the I/O services, and also store the corresponding port ID
    std::map<std::shared_ptr<uhd::transport::dpdk_io_service>, std::vector<size_t>>
        _io_srv_portid_map;
};

} // namespace dpdk
}} // namespace uhd::transport
