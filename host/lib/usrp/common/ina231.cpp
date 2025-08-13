//
// Copyright 2025 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhdlib/usrp/common/ina231.hpp>

namespace {
constexpr uint8_t REG_CONFIGURATION = 0x0;
constexpr uint8_t REG_SHUNT_VOLTAGE = 0x1;
constexpr uint8_t REG_BUS_VOLTAGE   = 0x2;
constexpr uint8_t REG_POWER         = 0x3;
constexpr uint8_t REG_CURRENT       = 0x4;
constexpr uint8_t REG_CALIBRATION   = 0x5;
constexpr uint8_t REG_MASK_ENABLE   = 0x6;
constexpr uint8_t REG_ALERT_LIMIT   = 0x7;
} // namespace

class ina231_impl : public ina231_iface
{
public:
    ina231_impl(uhd::i2c_iface::sptr i2c,
        const uint16_t chip_addr,
        const double shunt_resistance,
        const double max_current)
        : _i2c(i2c), _chip_addr(chip_addr)
    {
        _current_lsb = max_current / (1 << 15);
        // Formula taken from INA231 datasheet
        uint16_t cal_value =
            static_cast<uint16_t>((0.00512) / (_current_lsb * shunt_resistance));
        uint8_t cal_value_upper = (cal_value >> 8) & 0xFF;
        uint8_t cal_value_lower = cal_value & 0xFF;
        _i2c->write_i2c(_chip_addr, {REG_CALIBRATION, cal_value_upper, cal_value_lower});
    }

    double read_bus_voltage() override
    {
        _i2c->write_i2c(_chip_addr, {REG_BUS_VOLTAGE});
        uhd::byte_vector_t reg_value = _i2c->read_i2c(_chip_addr, 2);
        uint16_t bus_voltage         = (reg_value[0]) << 8 | reg_value[1];
        // Formula taken from INA231 datasheet, 1.25mV is LSB
        return bus_voltage * (1.25 / 1000.0);
    }

    double read_power() override
    {
        _i2c->write_i2c(_chip_addr, {REG_POWER});
        uhd::byte_vector_t reg_value = _i2c->read_i2c(_chip_addr, 2);
        // Formula taken from INA231 datasheet
        double power_lsb = _current_lsb * 25;
        uint16_t power   = (reg_value[0]) << 8 | reg_value[1];
        return power * power_lsb;
    }

    double read_current() override
    {
        _i2c->write_i2c(_chip_addr, {REG_CURRENT});
        uhd::byte_vector_t reg_value = _i2c->read_i2c(_chip_addr, 2);
        uint16_t current             = (reg_value[0]) << 8 | reg_value[1];
        return current * _current_lsb;
    }

private:
    uhd::i2c_iface::sptr _i2c;
    uint16_t _chip_addr;
    double _current_lsb;
};

ina231_iface::sptr ina231_iface::make(uhd::i2c_iface::sptr i2c,
    const uint16_t chip_addr,
    const double shunt_resistance,
    const double max_current)
{
    return std::make_shared<ina231_impl>(i2c, chip_addr, shunt_resistance, max_current);
}
