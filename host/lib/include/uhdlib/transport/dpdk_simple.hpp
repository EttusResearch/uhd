//
// Copyright 2019 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/transport/udp_simple.hpp>

namespace uhd { namespace transport {

class dpdk_simple : public udp_simple
{
public:
    virtual ~dpdk_simple(void) = 0;

    static udp_simple::sptr make_connected(
        const std::string& addr, const std::string& port);

    static udp_simple::sptr make_broadcast(
        const std::string& addr, const std::string& port);

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
