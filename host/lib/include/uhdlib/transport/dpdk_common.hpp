//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_DPDK_COMMON_HPP
#define INCLUDED_DPDK_COMMON_HPP

#include <uhd/config.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/utils/noncopyable.hpp>
#include <array>
#include <atomic>
#include <mutex>
#include <string>

namespace uhd { namespace transport {

class uhd_dpdk_ctx : uhd::noncopyable {
public:
    UHD_SINGLETON_FCN(uhd_dpdk_ctx, get);

    ~uhd_dpdk_ctx(void);

    /*!
     * Initialize uhd-dpdk (and do only once)
     * \param user_args User args passed in to override config files
     */
    void init(const device_addr_t &user_args);

    /*!
     * Get MTU of NICs used by DPDK
     *
     * \return Number of Bytes in MTU
     */
    size_t get_mtu(void) const;

    /*!
     * Get port ID from provided MAC address
     * \param mac_addr MAC address
     * \param port_id Int to write ID of port corresponding to MAC address
     * \return 0 if match found, else no match
     */
    int get_port_id(std::array<uint8_t, 6> mac_addr, unsigned int &port_id) const;

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
    bool is_init_done(void) const;

private:
    uhd_dpdk_ctx(void);

    size_t _mtu;
    std::mutex _init_mutex;
    std::atomic<bool> _init_done;
    uhd::dict<uint32_t, unsigned int> _routes;
};

}} // namespace uhd::transport

#endif /* INCLUDED_DPDK_COMMON_HPP */
