//
// Copyright 2019 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_DPDK_SIMPLE_HPP
#define INCLUDED_DPDK_SIMPLE_HPP

#include <uhd/utils/noncopyable.hpp>
#include <uhdlib/transport/dpdk_common.hpp>

namespace uhd { namespace transport {

class dpdk_simple : uhd::noncopyable
{
public:
    typedef boost::shared_ptr<dpdk_simple> sptr;

    virtual ~dpdk_simple(void) = 0;

    /*!
     * Make a new connected dpdk transport:
     * This transport is for sending and receiving
     * between this host and a single endpoint.
     * The primary usage for this transport will be control transactions.
     *
     * The address must be an ipv4 address.
     * The port must be a number.
     *
     * \param addr a string representing the destination address
     * \param port a string representing the destination port
     */
    static sptr make_connected(struct uhd_dpdk_ctx &ctx,
        const std::string &addr, const std::string &port);

    /*!
     * Make a new broadcasting dpdk transport:
     * This transport can send broadcast datagrams
     * and receive datagrams from multiple sources.
     * The primary usage for this transport will be to discover devices.
     *
     * The address must be an ipv4 address.
     * The port must be a number.
     *
     * \param addr a string representing the destination address
     * \param port a string representing the destination port
     */
    static sptr make_broadcast(struct uhd_dpdk_ctx &ctx,
        const std::string &addr, const std::string &port);

    /*!
     * Request a single send buffer of specified size.
     *
     * \param buf a pointer to place to write buffer location
     * \return the maximum length of the buffer in Bytes
     */
    virtual size_t get_tx_buf(void** buf) = 0;

    /*!
     * Send and release outstanding buffer
     *
     * \param number of bytes sent (releases buffer if sent)
     */
    virtual size_t send(size_t length) = 0;

    /*!
     * Receive a single packet.
     * Buffer provided by transport (must be freed).
     *
     * \param buf a pointer to place to write buffer location
     * \param timeout the timeout in seconds
     * \return the number of bytes received or zero on timeout
     */
    virtual size_t recv(void **buf, double timeout = 0.1) = 0;

    /*!
     * Return/free receive buffer
     */
    virtual void put_rx_buf(void) = 0;

    /*!
     * Get the last IP address as seen by recv().
     * Only use this with the broadcast socket.
     */
    virtual std::string get_recv_addr(void) = 0;

    /*!
     * Get the IP address for the destination
     */
    virtual std::string get_send_addr(void) = 0;
};

}} // namespace uhd::transport

#endif /* INCLUDED_DPDK_SIMPLE_HPP */
