//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/moving_average_block_control.hpp>
#include <uhd/rfnoc/property.hpp>
#include <uhd/rfnoc/registry.hpp>
#include <string>

using namespace uhd::rfnoc;

const uint32_t moving_average_block_control::REG_SUM_LEN_ADDR = 0;
const uint32_t moving_average_block_control::REG_DIVISOR_ADDR = 4;

// User property names
const char* const PROP_KEY_SUM_LEN = "sum_len";
const char* const PROP_KEY_DIVISOR = "divisor";

class moving_average_block_control_impl : public moving_average_block_control
{
public:
    RFNOC_BLOCK_CONSTRUCTOR(moving_average_block_control), _sum_len(10), _divisor(10)
    {
        _register_props();
        this->regs().poke32(REG_SUM_LEN_ADDR, uint8_t(_sum_len));
        this->regs().poke32(REG_DIVISOR_ADDR, uint32_t(_divisor));
    }

    void set_sum_len(const uint8_t sum_len) override
    {
        set_property(PROP_KEY_SUM_LEN, static_cast<int>(sum_len), res_source_info::USER);
    }

    uint8_t get_sum_len() const override
    {
        return _sum_len;
    }

    void set_divisor(const uint32_t divisor) override
    {
        set_property(PROP_KEY_DIVISOR, static_cast<int>(divisor), res_source_info::USER);
    }

    uint32_t get_divisor() const override
    {
        return _divisor;
    }

private:
    void _register_props()
    {
        // Register user properties
        register_property(
            &_prop_sum_len, [this]() { _set_sum_len(_prop_sum_len.get()); });
        register_property(
            &_prop_divisor, [this]() { _set_divisor(_prop_divisor.get()); });
    }

    void _set_sum_len(int sum_len)
    {
        // The hardware implementation requires this value to be in the range [1, 255]
        if ((sum_len < 1) || (sum_len > 255)) {
            throw uhd::value_error(
                "Attempting to set Moving Average Block sum length to invalid value!");
        }
        // The hardware implementation causes this noc block to clear the fifo when a
        // register write occurs to the sum length. This value should only be written if
        // the value actually changes.
        if (sum_len != _sum_len) {
            _sum_len = sum_len;
            this->regs().poke32(REG_SUM_LEN_ADDR, uint8_t(_sum_len));
        }
    }

    void _set_divisor(int divisor)
    {
        // The hardware implementation requires this value to be in the range [1, 2^24-1]
        if ((divisor < 1) || (divisor > (1 << 24) - 1)) {
            throw uhd::value_error(
                "Attempting to set Moving Average Block divisor to invalid value!");
        }
        _divisor = divisor;
        this->regs().poke32(REG_DIVISOR_ADDR, uint32_t(_divisor));
    }

    /**************************************************************************
     * Attributes
     *************************************************************************/
    property_t<int> _prop_sum_len =
        property_t<int>{PROP_KEY_SUM_LEN, 10, {res_source_info::USER}};
    property_t<int> _prop_divisor =
        property_t<int>{PROP_KEY_DIVISOR, 10, {res_source_info::USER}};

    uint8_t _sum_len;
    uint32_t _divisor;
};

UHD_RFNOC_BLOCK_REGISTER_DIRECT(moving_average_block_control,
    MOVING_AVERAGE_BLOCK,
    "MovingAverage",
    CLOCK_KEY_GRAPH,
    "bus_clk")
