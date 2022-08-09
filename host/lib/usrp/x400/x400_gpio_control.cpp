//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "x400_gpio_control.hpp"

using namespace uhd::rfnoc::x400;

namespace {
namespace gpio_regmap {
// Relative to the channel's ATR base
constexpr uint32_t ATR_IDLE_OFFSET = 0x0;
constexpr uint32_t ATR_RX_OFFSET   = 0x4;
constexpr uint32_t ATR_TX_OFFSET   = 0x8;
constexpr uint32_t ATR_XX_OFFSET   = 0xC;

constexpr uint32_t ATR_STRIDE = 0x10;

// Relative to the radio control base
constexpr uint32_t CLASSIC_MODE_OFFSET = 0x44;
constexpr uint32_t DDR_OFFSET          = 0x48;
constexpr uint32_t DISABLED_OFFSET     = 0x4C;
constexpr uint32_t READBACK_OFFSET     = 0x50;

constexpr uint32_t DIO_MIRROR_WINDOW = 0x1000;

// Relative to the DIO register map
constexpr uint32_t DIO_DIRECTION_REG = 0x4;
} // namespace gpio_regmap

// There are two ports, each with 12 pins
constexpr size_t NUM_PINS_PER_PORT = 12;

// These values should match the values in MPM's x4xx_periphs.py "DIO_PORT_MAP"
constexpr uint32_t PORTA_MAPPING[12] = {1, 0, 2, 3, 5, 4, 6, 7, 9, 8, 10, 11};
constexpr uint32_t PORTB_MAPPING[12] = {10, 11, 9, 8, 6, 7, 5, 4, 2, 3, 1, 0};
} // namespace

const char* uhd::rfnoc::x400::GPIO_BANK_NAME = "GPIO";

gpio_control::gpio_control(uhd::usrp::x400_rpc_iface::sptr rpcc, uhd::wb_iface::sptr iface)
    : _rpcc(rpcc), _regs(iface)
{
    _rpcc->dio_set_port_mapping("DIO");
    _rpcc->dio_set_voltage_level("PORTA", "3V3");
    _rpcc->dio_set_voltage_level("PORTB", "3V3");

    // Hardcode classic ATR (channels operate independently)
    _regs->poke32(gpio_regmap::CLASSIC_MODE_OFFSET, 0x1);

    // Initialize everything as inputs
    _rpcc->dio_set_pin_directions("PORTA", 0x0);
    _rpcc->dio_set_pin_directions("PORTB", 0x0);

    for (size_t bank = 0; bank < 2; bank++) {
        const wb_iface::wb_addr_type atr_base = bank * gpio_regmap::ATR_STRIDE;
        usrp::gpio_atr::gpio_atr_offsets regmap{
            atr_base + gpio_regmap::ATR_IDLE_OFFSET,
            atr_base + gpio_regmap::ATR_RX_OFFSET,
            atr_base + gpio_regmap::ATR_TX_OFFSET,
            atr_base + gpio_regmap::ATR_XX_OFFSET,
            gpio_regmap::DDR_OFFSET,
            gpio_regmap::DISABLED_OFFSET,
            gpio_regmap::READBACK_OFFSET,
        };
        _gpios.push_back(usrp::gpio_atr::gpio_atr_3000::make(_regs, regmap));
    }
}

void gpio_control::set_gpio_attr(
    const uhd::usrp::gpio_atr::gpio_attr_t attr, const uint32_t value)
{
    if (attr == uhd::usrp::gpio_atr::GPIO_DDR) {
        // We have to adjust the MB CPLD as well. MPM takes care of coordinating
        // the FPGA and the CPLD.
        _rpcc->dio_set_pin_directions("PORTA", value & 0xFFF);
        _rpcc->dio_set_pin_directions("PORTB", value >> 12);
    }

    _gpios[0]->set_gpio_attr(attr, internalize_value(value));
    if (is_atr_attr(attr)) {
        _gpios[1]->set_gpio_attr(attr, internalize_value(value));
    }
}

bool gpio_control::is_atr_attr(const uhd::usrp::gpio_atr::gpio_attr_t attr)
{
    return attr == uhd::usrp::gpio_atr::GPIO_ATR_0X || attr == uhd::usrp::gpio_atr::GPIO_ATR_RX
           || attr == uhd::usrp::gpio_atr::GPIO_ATR_TX || attr == uhd::usrp::gpio_atr::GPIO_ATR_XX;
}

uint32_t gpio_control::internalize_value(const uint32_t value)
{
    return (value & 0xFFF) | ((value & 0x00FFF000) << 4);
}

uint32_t gpio_control::publicize_value(const uint32_t value)
{
    return (value & 0xFFF) | ((value & 0x0FFF0000) >> 4);
}

uint32_t gpio_control::unmap_dio(const uint32_t bank, const uint32_t raw_form)
{
    const uint32_t* const mapping = bank == 1 ? PORTB_MAPPING : PORTA_MAPPING;
    uint32_t result               = 0;
    for (size_t i = 0; i < NUM_PINS_PER_PORT; i++) {
        if ((raw_form & (1 << i)) != 0) {
            result |= 1 << mapping[i];
        }
    }
    return result;
}

uint32_t gpio_control::get_gpio_attr(const uhd::usrp::gpio_atr::gpio_attr_t attr)
{
    if (attr == uhd::usrp::gpio_atr::GPIO_DDR) {
        // Retrieve the actual state from the FPGA mirror of the CPLD state
        const uint32_t raw_value = _regs->peek32(
            gpio_regmap::DIO_MIRROR_WINDOW + gpio_regmap::DIO_DIRECTION_REG);
        return (unmap_dio(1, raw_value >> 16) << NUM_PINS_PER_PORT)
               | unmap_dio(0, raw_value & 0xFFFF);
    }

    return publicize_value(_gpios[0]->get_attr_reg(attr));
}
