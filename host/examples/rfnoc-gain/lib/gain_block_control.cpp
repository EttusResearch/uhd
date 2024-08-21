//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

// Include our own header:
#include <rfnoc/gain/gain_block_control.hpp>

// These two includes are the minimum required to implement a block:
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/registry.hpp>

#include <cstdint>

using namespace rfnoc::gain;
using namespace uhd::rfnoc;

const uint32_t gain_block_control::REG_GAIN_VALUE   = 0x00;
const std::string gain_block_control::PROP_KEY_GAIN = "gain";

constexpr uint32_t DEFAULT_GAIN = 1;

class gain_block_control_impl : public gain_block_control
{
public:
    RFNOC_BLOCK_CONSTRUCTOR(gain_block_control)
    {
        register_property(&_gain);
        add_property_resolver({&_gain}, {&_gain}, [this]() {
            if (_gain.get() < 1) {
                _gain.set(1);
            }
            regs().poke32(REG_GAIN_VALUE, static_cast<uint32_t>(_gain.get()));
        });
        set_gain_value(1);
    }

    void set_gain_value(const uint32_t gain)
    {
        set_property<int>(PROP_KEY_GAIN, static_cast<int>(gain));
    }

    uint32_t get_gain_value()
    {
        return static_cast<uint32_t>(get_property<int>(PROP_KEY_GAIN));
    }

private:
    // Note: We use int instead of uint32_t here so we can use the built-in automatic
    // casting from string to int.
    property_t<int> _gain{PROP_KEY_GAIN, DEFAULT_GAIN, {res_source_info::USER}};
};

UHD_RFNOC_BLOCK_REGISTER_DIRECT(
    gain_block_control, 0xb16, "Gain", CLOCK_KEY_GRAPH, "bus_clk")
