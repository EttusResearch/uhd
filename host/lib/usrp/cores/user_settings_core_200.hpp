//
// Copyright 2012,2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_USRP_USER_SETTINGS_CORE_200_HPP
#define INCLUDED_LIBUHD_USRP_USER_SETTINGS_CORE_200_HPP

#include <uhd/config.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <uhd/types/wb_iface.hpp>

class user_settings_core_200 : boost::noncopyable{
public:
    typedef boost::shared_ptr<user_settings_core_200> sptr;
    typedef std::pair<uint8_t, uint32_t> user_reg_t;

    virtual ~user_settings_core_200(void) = 0;

    static sptr make(uhd::wb_iface::sptr iface, const size_t base);

    virtual void set_reg(const user_reg_t &reg) = 0;
};

#endif /* INCLUDED_LIBUHD_USRP_USER_SETTINGS_CORE_200_HPP */
