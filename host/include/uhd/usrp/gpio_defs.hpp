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

#ifndef INCLUDED_LIBUHD_USRP_GPIO_DEFS_HPP
#define INCLUDED_LIBUHD_USRP_GPIO_DEFS_HPP

#include <uhd/config.hpp>
#include <boost/assign.hpp>
#include <boost/utility.hpp>
#include <map>

namespace uhd { namespace usrp { namespace gpio_atr {

enum gpio_atr_reg_t {
    ATR_REG_IDLE        = int('i'),
    ATR_REG_TX_ONLY     = int('t'),
    ATR_REG_RX_ONLY     = int('r'),
    ATR_REG_FULL_DUPLEX = int('f')
};

enum gpio_atr_mode_t {
    MODE_ATR  = 0,   //Output driven by the auto-transmit-receive engine
    MODE_GPIO = 1    //Output value is static
};

enum gpio_ddr_t {
    DDR_INPUT   = 0,
    DDR_OUTPUT  = 1
};

enum gpio_attr_t {
    GPIO_CTRL,
    GPIO_DDR,
    GPIO_OUT,
    GPIO_ATR_0X,
    GPIO_ATR_RX,
    GPIO_ATR_TX,
    GPIO_ATR_XX
};

typedef std::map<gpio_attr_t, std::string> gpio_attr_map_t;

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

}}} //namespaces

#endif /* INCLUDED_LIBUHD_USRP_GPIO_DEFS_HPP */
