//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include "../ad937x_device_types.hpp"
#include "../adi/t_mykonos.h"

#include "mpm/ad937x/ad937x_ctrl_types.hpp"

#include <vector>
#include <unordered_map>

// C++14 requires std::hash includes a specialization for enums, but gcc doesn't do that yet
// Remove this when that happens
namespace std {
    template <> struct hash<uhd::direction_t>
    {
        size_t operator()(const uhd::direction_t & x) const
        {
            return static_cast<std::size_t>(x);
        }
    };

    template <> struct hash<mpm::ad937x::device::chain_t>
    {
        size_t operator()(const mpm::ad937x::device::chain_t & x) const
        {
            return static_cast<std::size_t>(x);
        }
    };
}

// collection of the 5 attributes that define the gain pins for a channel in Mykonos
struct ad937x_gain_ctrl_channel_t
{
    uint8_t enable;
    uint8_t inc_step;
    uint8_t dec_step;
    mykonosGpioSelect_t inc_pin;
    mykonosGpioSelect_t dec_pin;

    ad937x_gain_ctrl_channel_t(mykonosGpioSelect_t inc_pin, mykonosGpioSelect_t dec_pin);

private:
    const static uint8_t DEFAULT_GAIN_STEP;
    const static bool DEFAULT_ENABLE;
};

// logically maps ad937x_gain_ctrl_channels by direction and channel number
struct ad937x_gain_ctrl_config_t
{
    std::unordered_map<uhd::direction_t,
        std::unordered_map<mpm::ad937x::device::chain_t, ad937x_gain_ctrl_channel_t>> config;

    ad937x_gain_ctrl_config_t(mpm::ad937x::gpio::gain_pins_t gain_pins);
};


