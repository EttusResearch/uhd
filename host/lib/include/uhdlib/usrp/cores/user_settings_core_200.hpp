//
// Copyright 2012,2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/types/wb_iface.hpp>
#include <uhd/utils/noncopyable.hpp>
#include <boost/utility.hpp>
#include <memory>

class user_settings_core_200 : uhd::noncopyable
{
public:
    typedef std::shared_ptr<user_settings_core_200> sptr;
    typedef std::pair<uint8_t, uint32_t> user_reg_t;

    virtual ~user_settings_core_200(void) = 0;

    static sptr make(uhd::wb_iface::sptr iface, const size_t base);

    virtual void set_reg(const user_reg_t& reg) = 0;
};
