//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

// Include our own header:
#include <rfnoc/example/gain_block_control.hpp>

// These two includes are the minimum required to implement a block:
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/registry.hpp>

using namespace rfnoc::example;
using namespace uhd::rfnoc;

const uint32_t gain_block_control::REG_GAIN_VALUE = 0x00;

class gain_block_control_impl : public gain_block_control
{
public:
    RFNOC_BLOCK_CONSTRUCTOR(gain_block_control) {}

    void set_gain_value(const uint32_t gain)
    {
        regs().poke32(REG_GAIN_VALUE, gain);
    }

    uint32_t get_gain_value()
    {
        return regs().peek32(REG_GAIN_VALUE);
    }

private:
};

UHD_RFNOC_BLOCK_REGISTER_DIRECT(
    gain_block_control, 0xb16, "Gain", CLOCK_KEY_GRAPH, "bus_clk")
