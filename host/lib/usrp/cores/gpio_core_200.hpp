//
// Copyright 2011,2014,2015 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef INCLUDED_LIBUHD_USRP_GPIO_CORE_200_HPP
#define INCLUDED_LIBUHD_USRP_GPIO_CORE_200_HPP

#include <uhd/config.hpp>
#include <uhd/usrp/dboard_iface.hpp>
#include <boost/assign.hpp>
#include <boost/cstdint.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <uhd/types/wb_iface.hpp>
#include <map>

typedef enum {
    GPIO_CTRL,
    GPIO_DDR,
    GPIO_OUT,
    GPIO_ATR_0X,
    GPIO_ATR_RX,
    GPIO_ATR_TX,
    GPIO_ATR_XX
} gpio_attr_t;

typedef std::map<gpio_attr_t,std::string> gpio_attr_map_t;
static const gpio_attr_map_t gpio_attr_map =
    boost::assign::map_list_of
        (GPIO_CTRL,   "CTRL")
        (GPIO_DDR,    "DDR")
        (GPIO_OUT,    "OUT")
        (GPIO_ATR_0X, "ATR_0X")
        (GPIO_ATR_RX, "ATR_RX")
        (GPIO_ATR_TX, "ATR_TX")
        (GPIO_ATR_XX, "ATR_XX")
;

class gpio_core_200 : boost::noncopyable{
public:
    typedef boost::shared_ptr<gpio_core_200> sptr;

    typedef uhd::usrp::dboard_iface::unit_t unit_t;
    typedef uhd::usrp::dboard_iface::atr_reg_t atr_reg_t;

    virtual ~gpio_core_200(void) = 0;

    //! makes a new GPIO core from iface and slave base
    static sptr make(uhd::wb_iface::sptr iface, const size_t base, const size_t rb_addr);

    //! 1 = ATR
    virtual void set_pin_ctrl(const unit_t unit, const boost::uint16_t value) = 0;

    virtual void set_atr_reg(const unit_t unit, const atr_reg_t atr, const boost::uint16_t value) = 0;

    //! 1 = OUTPUT
    virtual void set_gpio_ddr(const unit_t unit, const boost::uint16_t value) = 0;

    virtual void set_gpio_out(const unit_t unit, const boost::uint16_t value) = 0;

    virtual boost::uint16_t read_gpio(const unit_t unit) = 0;

};

//! Simple wrapper for 32 bit write only
class gpio_core_200_32wo : boost::noncopyable{
public:
    typedef boost::shared_ptr<gpio_core_200_32wo> sptr;

    typedef uhd::usrp::dboard_iface::atr_reg_t atr_reg_t;

    virtual ~gpio_core_200_32wo(void) = 0;

    static sptr make(uhd::wb_iface::sptr iface, const size_t);

    virtual void set_atr_reg(const atr_reg_t atr, const boost::uint32_t value) = 0;

    virtual void set_all_regs(const boost::uint32_t value) = 0;
};

#endif /* INCLUDED_LIBUHD_USRP_GPIO_CORE_200_HPP */
