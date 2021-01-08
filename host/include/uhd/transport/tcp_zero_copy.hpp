//
// Copyright 2010-2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/transport/zero_copy.hpp>
#include <uhd/types/device_addr.hpp>
#include <memory>

namespace uhd { namespace transport {

/*!
 * The zero copy TCP transport.
 * This transport provides the uhd zero copy interface
 * on top of a standard tcp socket from boost asio.
 */
struct UHD_API tcp_zero_copy : public virtual zero_copy_if
{
    ~tcp_zero_copy(void) override;

    /*!
     * Make a new zero copy TCP transport:
     * This transport is for sending and receiving
     * between this host and a single endpoint.
     * The primary usage for this transport will be data transactions.
     *
     * The address will be resolved, it can be a host name or ipv4.
     * The port will be resolved, it can be a port type or number.
     *
     * \param addr a string representing the destination address
     * \param port a string representing the destination port
     * \param hints optional parameters to pass to the underlying transport
     */
    static zero_copy_if::sptr make(const std::string& addr,
        const std::string& port,
        const device_addr_t& hints = device_addr_t());
};

}} // namespace uhd::transport
