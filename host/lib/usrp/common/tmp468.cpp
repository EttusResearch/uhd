//
// Copyright 2025 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/utils/algorithm.hpp>
#include <uhdlib/usrp/common/tmp468.hpp>
#include <uhdlib/utils/narrow.hpp>
#include <cmath>
#include <map>

std::map<uint8_t, uint8_t> sensor_to_nfactor_addr = {{1, 0x41},
    {2, 0x49},
    {3, 0x51},
    {4, 0x59},
    {5, 0x61},
    {6, 0x69},
    {7, 0x71},
    {8, 0x79}};

class tmp468_impl : public tmp468_iface
{
public:
    tmp468_impl(uhd::i2c_iface::sptr i2c, uint16_t chip_addr)
        : _i2c(i2c), _chip_addr(chip_addr)
    {
    }

    // Temperature conversion procedure taken from the tmp468 datasheet
    double read_temperature(const tmp_sensor_t sensor) override
    {
        _i2c->write_i2c(_chip_addr, {sensor});
        uhd::byte_vector_t temp_in_bytes = _i2c->read_i2c(_chip_addr, 2);
        uint16_t temp_int                = (temp_in_bytes[0] << 8) | temp_in_bytes[1];
        temp_int >>= 3;
        if (temp_int & 0x1000) {
            int16_t magnitude = -((~temp_int & 0x1FFF) + 1);
            return magnitude * 0.0625;
        } else {
            return temp_int * 0.0625;
        }
    }

    void set_ideality_factor(
        const tmp_sensor_t sensor, const double ideality_factor) override
    {
        // Taken from equation 3 in the tmp468 datasheet
        int ideality_setting = std::round(((1.008 * 2088) / ideality_factor) - 2088);
        ideality_setting     = uhd::clip<int>(ideality_setting, -128, 127);
        int8_t reg_value     = uhd::narrow_cast<int8_t>(ideality_setting);
        _i2c->write_i2c(_chip_addr,
            {sensor_to_nfactor_addr[sensor], static_cast<uint8_t>(reg_value), 0x00});
    }

private:
    uhd::i2c_iface::sptr _i2c;
    uint16_t _chip_addr;
};

tmp468_iface::sptr tmp468_iface::make(uhd::i2c_iface::sptr i2c, uint16_t chip_addr)
{
    return std::make_shared<tmp468_impl>(i2c, chip_addr);
}
