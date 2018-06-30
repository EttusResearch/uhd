//
// Copyright 2012 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_USRP_USER_SETTINGS_CORE_3000_HPP
#define INCLUDED_LIBUHD_USRP_USER_SETTINGS_CORE_3000_HPP

#include <uhd/config.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <uhd/types/wb_iface.hpp>

class user_settings_core_3000 : public uhd::wb_iface {
public:
    virtual ~user_settings_core_3000() {}

    static sptr make(
        wb_iface::sptr iface,
        const wb_addr_type sr_base_addr, const wb_addr_type rb_reg_addr);
};

#endif /* INCLUDED_LIBUHD_USRP_USER_SETTINGS_CORE_3000_HPP */
