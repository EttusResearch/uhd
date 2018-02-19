//
// Copyright 2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_TRANSPORT_NIRIO_ZERO_COPY_HPP
#define INCLUDED_UHD_TRANSPORT_NIRIO_ZERO_COPY_HPP

#include <uhd/transport/nirio/niusrprio_session.h>
#include <uhd/config.hpp>
#include <uhd/transport/zero_copy.hpp>
#include <uhd/types/device_addr.hpp>
#include <boost/shared_ptr.hpp>
#include <stdint.h>

namespace uhd{ namespace transport{

class UHD_API nirio_zero_copy : public virtual zero_copy_if{
public:
    typedef boost::shared_ptr<nirio_zero_copy> sptr;

    static sptr make(
        uhd::niusrprio::niusrprio_session::sptr fpga_session,
        const uint32_t instance,
        const zero_copy_xport_params &default_buff_args,
        const device_addr_t &hints = device_addr_t()
    );
};

}} //namespace

#endif /* INCLUDED_UHD_TRANSPORT_NIRIO_ZERO_COPY_HPP */
