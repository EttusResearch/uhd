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
constexpr size_t NUM_PORTS         = 2;
constexpr size_t NUM_PINS_PER_PORT = 12;

// Start of Port B pin numbers relative to Port A:
constexpr size_t PORT_NUMBER_OFFSET = 16;

// These values should match the values in MPM's x4xx_periphs.py "DIO_PORT_MAP"
constexpr uint32_t PORTA_MAPPING[12] = {1, 0, 2, 3, 5, 4, 6, 7, 9, 8, 10, 11};
constexpr uint32_t PORTB_MAPPING[12] = {10, 11, 9, 8, 6, 7, 5, 4, 2, 3, 1, 0};
} // namespace

const char* uhd::rfnoc::x400::GPIO_BANK_NAME = "GPIO";

gpio_control::gpio_control(uhd::usrp::x400_rpc_iface::sptr rpcc,
    uhd::rfnoc::mpmd_mb_controller::sptr mb_control,
    uhd::wb_iface::sptr iface)
    : _rpcc(rpcc), _mb_control(mb_control), _regs(iface)
{
    _rpcc->dio_set_port_mapping("DIO");
    _rpcc->dio_set_voltage_level("PORTA", "3V3");
    _rpcc->dio_set_voltage_level("PORTB", "3V3");

    // Hardcode new ATR (channel ATRs are combined into 4-bit index)
    // Note that we emulate classic ATR
    _regs->poke32(gpio_regmap::CLASSIC_MODE_OFFSET, 0x0);

    // Initialize everything as inputs
    _rpcc->dio_set_pin_directions("PORTA", 0x0);
    _rpcc->dio_set_pin_directions("PORTB", 0x0);

    for (size_t bank = 0; bank < 4; bank++) {
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

    if (is_atr_attr(attr)) {
        const uint32_t rf1_mask = build_rf1_mask();

        for (size_t i = 0; i < 4; i++) {
            const uint32_t previous_value = unmap_dio(_gpios[i]->get_attr_reg(attr));
            const uint32_t new_value = (previous_value & rf1_mask) | (value & ~rf1_mask);
            _gpios[i]->set_gpio_attr(attr, map_dio(new_value));
        }

        // Set the RF1 settings
        for (const auto subattr : {uhd::usrp::gpio_atr::GPIO_ATR_0X,
                 uhd::usrp::gpio_atr::GPIO_ATR_RX,
                 uhd::usrp::gpio_atr::GPIO_ATR_TX,
                 uhd::usrp::gpio_atr::GPIO_ATR_XX}) {
            const uint32_t previous_value =
                unmap_dio(_gpios[atr_attr_index(attr)]->get_attr_reg(subattr));
            const uint32_t new_value = (previous_value & ~rf1_mask) | (value & rf1_mask);
            _gpios[atr_attr_index(attr)]->set_gpio_attr(subattr, map_dio(new_value));
        }
    } else {
        const uint32_t internal_value = map_dio(value);
        _gpios[0]->set_gpio_attr(attr, internal_value);
    }
}

uint32_t gpio_control::build_rf1_mask()
{
    auto porta_sources = _mb_control->get_gpio_src("GPIO0");
    auto portb_sources = _mb_control->get_gpio_src("GPIO1");

    uint32_t rf1_mask = 0;
    for (size_t i = 0; i < 12; i++) {
        if (porta_sources[i].find("RF1") != std::string::npos) {
            rf1_mask |= 1 << i;
        }
        if (portb_sources[i].find("RF1") != std::string::npos) {
            rf1_mask |= 1 << (i + 12);
        }
    }

    return rf1_mask;
}

size_t gpio_control::atr_attr_index(const uhd::usrp::gpio_atr::gpio_attr_t attr)
{
    return attr == uhd::usrp::gpio_atr::GPIO_ATR_0X   ? 0
           : attr == uhd::usrp::gpio_atr::GPIO_ATR_RX ? 1
           : attr == uhd::usrp::gpio_atr::GPIO_ATR_TX ? 2
           : attr == uhd::usrp::gpio_atr::GPIO_ATR_XX ? 3
                                                      : 0;
}

bool gpio_control::is_atr_attr(const uhd::usrp::gpio_atr::gpio_attr_t attr)
{
    return attr == uhd::usrp::gpio_atr::GPIO_ATR_0X
           || attr == uhd::usrp::gpio_atr::GPIO_ATR_RX
           || attr == uhd::usrp::gpio_atr::GPIO_ATR_TX
           || attr == uhd::usrp::gpio_atr::GPIO_ATR_XX;
}

uint32_t gpio_control::unmap_dio(const uint32_t raw_form)
{
    uint32_t result = 0;
    for (size_t i = 0; i < NUM_PINS_PER_PORT; i++) {
        if ((raw_form & (1 << i)) != 0) {
            result |= 1 << _mapper.unmap_value(i);
        }
    }
    for (size_t i = PORT_NUMBER_OFFSET; i < PORT_NUMBER_OFFSET + NUM_PINS_PER_PORT; i++) {
        if ((raw_form & (1 << i)) != 0) {
            result |= 1 << _mapper.unmap_value(i);
        }
    }
    return result;
}

uint32_t gpio_control::map_dio(const uint32_t user_form)
{
    uint32_t result = 0;
    for (size_t i = 0; i < NUM_PORTS * NUM_PINS_PER_PORT; i++) {
        if ((user_form & (1 << i)) != 0) {
            result |= 1 << _mapper.map_value(i);
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
        return unmap_dio(raw_value);
    }

    if (is_atr_attr(attr)) {
        const uint32_t rf1_mask = build_rf1_mask();

        // Grab the values for each channel
        const uint32_t rf0_atr = unmap_dio(_gpios[0]->get_attr_reg(attr));
        const uint32_t rf1_atr = unmap_dio(
            _gpios[atr_attr_index(attr)]->get_attr_reg(uhd::usrp::gpio_atr::GPIO_ATR_0X));
        return (rf0_atr & ~rf1_mask) | (rf1_atr & rf1_mask);
    }

    const uint32_t raw_value = _gpios[0]->get_attr_reg(attr);
    return unmap_dio(raw_value);
}

uint32_t uhd::rfnoc::x400::x400_gpio_port_mapping::map_value(const uint32_t& value)
{
    const uint32_t bank           = value >= NUM_PINS_PER_PORT ? 1 : 0;
    uint32_t pin_intern           = value % NUM_PINS_PER_PORT;
    const uint32_t* const mapping = bank == 1 ? PORTB_MAPPING : PORTA_MAPPING;
    for (size_t i = 0; i < NUM_PINS_PER_PORT; i++) {
        if (mapping[i] == pin_intern) {
            return i + (bank * PORT_NUMBER_OFFSET);
        }
    }
    throw uhd::lookup_error(
        std::string("Could not find corresponding GPIO pin number for given SPI pin ")
        + std::to_string(value));
}

uint32_t uhd::rfnoc::x400::x400_gpio_port_mapping::unmap_value(const uint32_t& value)
{
    const uint32_t bank           = value >= PORT_NUMBER_OFFSET ? 1 : 0;
    uint32_t pin_number           = value % PORT_NUMBER_OFFSET;
    const uint32_t* const mapping = bank == 1 ? PORTB_MAPPING : PORTA_MAPPING;
    UHD_ASSERT_THROW(pin_number < NUM_PINS_PER_PORT);
    return mapping[pin_number] + (bank * NUM_PINS_PER_PORT);
}
