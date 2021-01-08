//
// Copyright 2012 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/types/wb_iface.hpp>
#include <boost/utility.hpp>
#include <memory>

class user_settings_core_3000 : public uhd::wb_iface
{
public:
    ~user_settings_core_3000() override {}

    static sptr make(wb_iface::sptr iface,
        const wb_addr_type sr_base_addr,
        const wb_addr_type rb_reg_addr);
};
