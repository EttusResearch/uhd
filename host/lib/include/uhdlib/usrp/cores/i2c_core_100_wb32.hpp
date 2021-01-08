//
// Copyright 2011-2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/types/serial.hpp>
#include <uhd/types/wb_iface.hpp>
#include <uhd/utils/noncopyable.hpp>
#include <memory>

class i2c_core_100_wb32 : uhd::noncopyable, public uhd::i2c_iface
{
public:
    typedef std::shared_ptr<i2c_core_100_wb32> sptr;

    ~i2c_core_100_wb32(void) override = 0;

    //! makes a new i2c core from iface and slave base
    static sptr make(uhd::wb_iface::sptr iface, const size_t base);

    virtual void set_clock_rate(const double rate) = 0;
};
