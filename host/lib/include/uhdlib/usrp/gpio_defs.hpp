//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_USRP_GPIO_DEFS_LIB_HPP
#define INCLUDED_LIBUHD_USRP_GPIO_DEFS_LIB_HPP

#include <uhd/usrp/gpio_defs.hpp>
#include <map>
#include <string>

namespace uhd { namespace usrp { namespace gpio_atr {

    enum gpio_atr_mode_t {
        MODE_ATR  = 0,   //Output driven by the auto-transmit-receive engine
        MODE_GPIO = 1    //Output value is static
    };

    enum gpio_ddr_t {
        DDR_INPUT   = 0,
        DDR_OUTPUT  = 1
    };

    enum gpio_attr_t {
        GPIO_SRC,
        GPIO_CTRL,
        GPIO_DDR,
        GPIO_OUT,
        GPIO_ATR_0X,
        GPIO_ATR_RX,
        GPIO_ATR_TX,
        GPIO_ATR_XX,
        GPIO_READBACK
    };

    static const std::string GPIO_ATTR_SRC = "SRC";
    //! Attribute name for GPIO control.
    static const std::string GPIO_ATTR_CTRL = "CTRL";
    //! Attribute name for GPIO data direction register.
    static const std::string GPIO_ATTR_DDR = "DDR";
    //! Attribute name for GPIO ouput value.
    static const std::string GPIO_ATTR_OUT = "OUT";
    //! Attribute name for GPIO ATR idle state register.
    static const std::string GPIO_ATTR_ATR0X = "ATR_0X";
    //! Attribute name for GPIO ATR receive only register.
    static const std::string GPIO_ATTR_ATRRX = "ATR_RX";
    //! Attribute name for GPIO ATR transmit only register.
    static const std::string GPIO_ATTR_ATRTX = "ATR_TX";
    //! Attribute name for GPIO ATR  full duplex state register.
    static const std::string GPIO_ATTR_ATRXX = "ATR_XX";
    //!  Attribute name for GPIO READBACK  register.
    static const std::string GPIO_ATTR_READBACK = "READBACK";

    typedef std::map<gpio_attr_t, std::string> gpio_attr_map_t;

    static const gpio_attr_map_t gpio_attr_map{
        {GPIO_SRC,       GPIO_ATTR_SRC},
        {GPIO_CTRL,      GPIO_ATTR_CTRL},
        {GPIO_DDR,       GPIO_ATTR_DDR},
        {GPIO_OUT,       GPIO_ATTR_OUT},
        {GPIO_ATR_0X,    GPIO_ATTR_ATR0X},
        {GPIO_ATR_RX,    GPIO_ATTR_ATRRX},
        {GPIO_ATR_TX,    GPIO_ATTR_ATRTX},
        {GPIO_ATR_XX,    GPIO_ATTR_ATRXX},
        {GPIO_READBACK,  GPIO_ATTR_READBACK}
    };

    static const std::map<gpio_attr_t, std::map<uint32_t, std::string>> attr_value_map{
        {GPIO_CTRL, {{0, "ATR"},   {1, "GPIO"}}},
        {GPIO_DDR,  {{0, "INPUT"}, {1, "OUTPUT"}}}
    };

    static const std::map<std::string, gpio_attr_t> gpio_attr_rev_map{
        {GPIO_ATTR_SRC,      GPIO_SRC},
        {GPIO_ATTR_CTRL,     GPIO_CTRL},
        {GPIO_ATTR_DDR,      GPIO_DDR},
        {GPIO_ATTR_OUT,      GPIO_OUT},
        {GPIO_ATTR_ATR0X,    GPIO_ATR_0X},
        {GPIO_ATTR_ATRRX,    GPIO_ATR_RX},
        {GPIO_ATTR_ATRTX,    GPIO_ATR_TX},
        {GPIO_ATTR_ATRXX,    GPIO_ATR_XX},
        {GPIO_ATTR_READBACK, GPIO_READBACK}
    };

    static const gpio_attr_map_t default_attr_value_map{
        {GPIO_SRC,  "RADIO_0/0"},
        {GPIO_CTRL, "GPIO"},
        {GPIO_DDR,  "INPUT"}
    };

    static const std::map<std::string, uint32_t> gpio_level_map{
        {"HIGH",  1},
        {"LOW",   0},
        {"ON",    1},
        {"OFF",   0},
        {"TRUE",  1},
        {"FALSE", 0}
    };

    static const std::map<std::string, uint32_t> gpio_direction{
        {"OUT",    1},
        {"IN",     0},
        {"OUTPUT", 1},
        {"INPUT",  0}
    };

    static const std::map<std::string, uint32_t> gpio_ctrl_mode{
        {"ATR",  0},
        {"GPIO", 1}
    };

    static const std::map<std::string, std::map<std::string, uint32_t>> gpio_attr_value_pair{
        {GPIO_ATTR_CTRL,     uhd::usrp::gpio_atr::gpio_ctrl_mode},
        {GPIO_ATTR_DDR,      uhd::usrp::gpio_atr::gpio_direction},
        {GPIO_ATTR_OUT,      uhd::usrp::gpio_atr::gpio_level_map},
        {GPIO_ATTR_ATR0X,    uhd::usrp::gpio_atr::gpio_level_map},
        {GPIO_ATTR_ATRRX,    uhd::usrp::gpio_atr::gpio_level_map},
        {GPIO_ATTR_ATRTX,    uhd::usrp::gpio_atr::gpio_level_map},
        {GPIO_ATTR_ATRXX,    uhd::usrp::gpio_atr::gpio_level_map},
        {GPIO_ATTR_READBACK, uhd::usrp::gpio_atr::gpio_level_map}
    };

}}} //namespaces

#endif /* INCLUDED_LIBUHD_USRP_GPIO_DEFS_LIB_HPP */
