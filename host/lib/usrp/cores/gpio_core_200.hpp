//
// Copyright 2011,2014,2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_USRP_GPIO_CORE_200_HPP
#define INCLUDED_LIBUHD_USRP_GPIO_CORE_200_HPP

#include <uhd/config.hpp>
#include <uhd/usrp/dboard_iface.hpp>
#include <uhd/usrp/gpio_defs.hpp>
#include <boost/assign.hpp>
#include <stdint.h>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <uhd/types/wb_iface.hpp>
#include <map>

class gpio_core_200 : boost::noncopyable{
public:
    typedef boost::shared_ptr<gpio_core_200> sptr;

    typedef uhd::usrp::dboard_iface::unit_t unit_t;
    typedef uhd::usrp::dboard_iface::atr_reg_t atr_reg_t;

    virtual ~gpio_core_200(void) = 0;

    //! makes a new GPIO core from iface and slave base
    static sptr make(
        uhd::wb_iface::sptr iface, const size_t base, const size_t rb_addr);

    //! 1 = ATR
    virtual void set_pin_ctrl(
        const unit_t unit, const uint16_t value, const uint16_t mask) = 0;

    virtual uint16_t get_pin_ctrl(unit_t unit) = 0;

    virtual void set_atr_reg(
        const unit_t unit, const atr_reg_t atr, const uint16_t value, const uint16_t mask) = 0;

    virtual uint16_t get_atr_reg(unit_t unit, atr_reg_t reg) = 0;

    //! 1 = OUTPUT
    virtual void set_gpio_ddr(
        const unit_t unit, const uint16_t value, const uint16_t mask) = 0;

    virtual uint16_t get_gpio_ddr(unit_t unit) = 0;

    virtual void set_gpio_out(
        const unit_t unit, const uint16_t value, const uint16_t mask) = 0;

    virtual uint16_t get_gpio_out(unit_t unit) = 0;

    virtual uint16_t read_gpio(const unit_t unit) = 0;
};

//! Simple wrapper for 32 bit write only
class gpio_core_200_32wo : boost::noncopyable{
public:
    typedef boost::shared_ptr<gpio_core_200_32wo> sptr;

    typedef uhd::usrp::dboard_iface::atr_reg_t atr_reg_t;

    virtual ~gpio_core_200_32wo(void) = 0;

    static sptr make(uhd::wb_iface::sptr iface, const size_t);

    virtual void set_ddr_reg() = 0;

    virtual void set_atr_reg(const atr_reg_t atr, const uint32_t value) = 0;

    virtual void set_all_regs(const uint32_t value) = 0;
};

#endif /* INCLUDED_LIBUHD_USRP_GPIO_CORE_200_HPP */
