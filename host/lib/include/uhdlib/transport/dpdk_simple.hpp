//
// Copyright 2019 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_DPDK_SIMPLE_HPP
#define INCLUDED_DPDK_SIMPLE_HPP

#include <uhd/transport/udp_simple.hpp>
#include <uhdlib/transport/dpdk_common.hpp>

namespace uhd { namespace transport {

class dpdk_simple : public udp_simple
{
public:
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
    static udp_simple::sptr make_connected(struct uhd_dpdk_ctx &ctx,
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
    static udp_simple::sptr make_broadcast(struct uhd_dpdk_ctx &ctx,
        const std::string &addr, const std::string &port);

    /*!
     * Send a single buffer.
     * Blocks until the data is sent.
     * \param buff single asio buffer
     * \return the number of bytes sent
     */
    virtual size_t send(const boost::asio::const_buffer& buff) = 0;

    /*!
     * Receive into the provided buffer.
     * Blocks until data is received or a timeout occurs.
     * \param buff a mutable buffer to receive into
     * \param timeout the timeout in seconds
     * \return the number of bytes received or zero on timeout
     */
    virtual size_t recv(
        const boost::asio::mutable_buffer& buff, double timeout = 0.1) = 0;

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
