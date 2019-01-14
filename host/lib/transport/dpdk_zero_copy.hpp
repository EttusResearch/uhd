//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef DPDK_ZERO_COPY_HPP
#define DPDK_ZERO_COPY_HPP

#include <uhd/config.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/transport/zero_copy.hpp>
#include <boost/shared_ptr.hpp>
#include <string>
#include <vector>
#include <mutex>


namespace uhd { namespace transport {

class uhd_dpdk_ctx : boost::noncopyable {
public:
    UHD_SINGLETON_FCN(uhd_dpdk_ctx, get);

    ~uhd_dpdk_ctx(void);

    /*!
     * Initialize uhd-dpdk (and do only once)
     * \param eal_args Arguments to pass to DPDK rte_eal_init() function
     * \param num_ports Size of port_thread_mapping array (also number in use)
     * \param port_thread_mapping Map NICs to threads: index=port, value=thread
     * \param num_mbufs Number of packet buffers for each port's memory pool
     * \param mbuf_cache_size Size of per-core packet buffer cache from mempool
     * \param mtu MTU of NIC ports
     */
    void init(const dict<std::string, std::string> &eal_args, unsigned int num_ports,
              int *port_thread_mapping, int num_mbufs, int mbuf_cache_size,
              size_t mtu);

    /*!
     * Get port ID from provided MAC address
     * \param mac_addr MAC address
     * \param port_id Int to write ID of port corresponding to MAC address
     * \return 0 if match found, else no match
     */
    int get_port_id(std::array<uint8_t, 6> mac_addr, unsigned int &port_id);

    /*!
     * Get port ID for routing packet destined for given address
     * \param addr Destination address
     * \return port ID from routing table
     */
    int get_route(const std::string &addr) const;

    /*!
     * Set IPv4 address and subnet mask of given NIC port
     * Not thread-safe. Should only be written before ports are in use.
     * \param port_id NIC port ID
     * \param ipv4_addr IPv4 address to write
     * \param netmask Subnet mask identifying network number in ipv4_addr
     * \return 0 if successful, else error
     */
    int set_ipv4_addr(unsigned int port_id, uint32_t ipv4_addr, uint32_t netmask);

    /*!
     * \return whether init() has been called
     */
    bool is_init_done(void);

private:
    uhd_dpdk_ctx(void);

    size_t _mtu;
    std::mutex _init_mutex;
    std::atomic<bool> _init_done;
};

/*!
 * A zero copy transport interface to the dpdk DMA library.
 */
class dpdk_zero_copy : public virtual zero_copy_if {
public:
    typedef boost::shared_ptr<dpdk_zero_copy> sptr;

    static sptr make(
        struct uhd_dpdk_ctx &ctx,
        const unsigned int dpdk_port_id,
        const std::string &addr,
        const std::string &remote_port,
        const std::string &local_port, /* 0 = auto-assign */
        const zero_copy_xport_params &default_buff_args,
        const device_addr_t &hints
    );

    virtual uint16_t get_local_port(void) const = 0;

    virtual std::string get_local_addr(void) const = 0;

    virtual uint32_t get_drop_count(void) const = 0;
};

}}

#endif /* DPDK_ZERO_COPY_HPP */
