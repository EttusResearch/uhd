//
// Copyright 2011-2012,2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_USRP_I2C_CORE_200_HPP
#define INCLUDED_LIBUHD_USRP_I2C_CORE_200_HPP

#include <uhd/config.hpp>
#include <uhd/types/serial.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <uhd/types/wb_iface.hpp>

class i2c_core_200 : boost::noncopyable, public uhd::i2c_iface{
public:
    typedef boost::shared_ptr<i2c_core_200> sptr;

    virtual ~i2c_core_200(void) = 0;

    //! makes a new i2c core from iface and slave base
    static sptr make(uhd::wb_iface::sptr iface, const size_t base, const size_t readback);
};

#endif /* INCLUDED_LIBUHD_USRP_I2C_CORE_200_HPP */
