//
// Copyright 2011-2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_USRP_I2C_CORE_100_WB32_HPP
#define INCLUDED_LIBUHD_USRP_I2C_CORE_100_WB32_HPP

#include <uhd/config.hpp>
#include <uhd/types/serial.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <uhd/types/wb_iface.hpp>

class i2c_core_100_wb32 : boost::noncopyable, public uhd::i2c_iface{
public:
    typedef boost::shared_ptr<i2c_core_100_wb32> sptr;

    virtual ~i2c_core_100_wb32(void) = 0;

    //! makes a new i2c core from iface and slave base
    static sptr make(uhd::wb_iface::sptr iface, const size_t base);

    virtual void set_clock_rate(const double rate) = 0;
};

#endif /* INCLUDED_LIBUHD_USRP_I2C_CORE_100_WB32_HPP */
