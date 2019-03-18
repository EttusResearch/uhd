//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef DPDK_ZERO_COPY_HPP
#define DPDK_ZERO_COPY_HPP

#include <uhdlib/transport/dpdk_common.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhd/transport/zero_copy.hpp>
#include <boost/shared_ptr.hpp>
#include <string>


namespace uhd { namespace transport {

/*!
 * A zero copy transport interface to the dpdk DMA library.
 */
class dpdk_zero_copy : public virtual zero_copy_if {
public:
    typedef boost::shared_ptr<dpdk_zero_copy> sptr;

    static sptr make(
        const struct uhd_dpdk_ctx &ctx,
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

}} // namespace uhd::transport

#endif /* DPDK_ZERO_COPY_HPP */
