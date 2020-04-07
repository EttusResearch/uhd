//
// Copyright 2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/transport/nirio/niusrprio_session.h>
#include <uhd/transport/zero_copy.hpp>
#include <uhd/types/device_addr.hpp>
#include <stdint.h>
#include <memory>

namespace uhd { namespace transport {

class UHD_API nirio_zero_copy : public virtual zero_copy_if
{
public:
    typedef std::shared_ptr<nirio_zero_copy> sptr;

    static sptr make(uhd::niusrprio::niusrprio_session::sptr fpga_session,
        const uint32_t instance,
        const zero_copy_xport_params& default_buff_args,
        const device_addr_t& hints = device_addr_t());
};

}} // namespace uhd::transport
