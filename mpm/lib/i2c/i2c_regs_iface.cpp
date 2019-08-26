//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <mpm/exception.hpp>
#include <mpm/i2c/i2c_iface.hpp>
#include <mpm/i2c/i2c_regs_iface.hpp>
#include <mpm/types/regs_iface.hpp>

using mpm::types::regs_iface;

/*! I2C implementation of the regs iface
 *
 * Uses i2cdev
 */
class i2c_regs_iface_impl : public regs_iface
{
public:
    i2c_regs_iface_impl(mpm::i2c::i2c_iface::sptr i2c_iface, const size_t reg_addr_size)
        : _i2c_iface(i2c_iface), _reg_addr_size(reg_addr_size)
    {
        if (reg_addr_size > 4) {
            throw mpm::runtime_error("reg_addr_size too largs for i2c_regs_iface");
        }
    }

    uint8_t peek8(const uint32_t addr)
    {
        uint8_t rx[1];
        uint8_t tx[5];
        int i = 0;
        for (; i < _reg_addr_size; i++) {
            tx[i] = 0xff & (addr >> 8 * (_reg_addr_size - i - 1));
        }

        int err = _i2c_iface->transfer(tx, _reg_addr_size, rx, 1);
        if (err) {
            throw mpm::runtime_error("I2C read failed");
        }

        return rx[0];
    }

    void poke8(const uint32_t addr, const uint8_t data)
    {
        uint8_t tx[5];
        int i = 0;
        for (; i < _reg_addr_size; i++) {
            tx[i] = 0xff & (addr >> 8 * (_reg_addr_size - i - 1));
        }
        tx[i] = data;

        int err = _i2c_iface->transfer(tx, _reg_addr_size + 1, NULL, 0);
        if (err) {
            throw mpm::runtime_error("I2C write failed");
        }
    }

    uint16_t peek16(const uint32_t addr)
    {
        uint8_t rx[2];
        uint8_t tx[5];
        int i = 0;
        for (; i < _reg_addr_size; i++) {
            tx[i] = 0xff & (addr >> 8 * (_reg_addr_size - i - 1));
        }

        int err = _i2c_iface->transfer(tx, _reg_addr_size, rx, 2);
        if (err) {
            throw mpm::runtime_error("I2C read failed");
        }

        uint16_t data = rx[0];
        data          = (data << 8) | rx[1];
        return data;
    }

    void poke16(const uint32_t addr, const uint16_t data)
    {
        uint8_t tx[6];
        int i = 0;
        for (; i < _reg_addr_size; i++) {
            tx[i] = 0xff & (addr >> 8 * (_reg_addr_size - i - 1));
        }
        tx[i]     = (data >> 8) & 0xff;
        tx[i + 1] = data & 0xff;

        int err = _i2c_iface->transfer(tx, _reg_addr_size + 2, NULL, 0);
        if (err) {
            throw mpm::runtime_error("I2C write failed");
        }
    }

    uint32_t peek32(const uint64_t addr)
    {
        throw mpm::not_implemented_error(
            "I2C regs iface does not implement 32 bit transactions.");
    }

    void poke32(const uint64_t addr, const uint32_t data)
    {
        throw mpm::not_implemented_error(
            "I2C regs iface does not implement 32 bit transactions.");
    }

private:
    mpm::i2c::i2c_iface::sptr _i2c_iface;

    const size_t _reg_addr_size;
};

regs_iface::sptr mpm::i2c::make_i2c_regs_iface(
    mpm::i2c::i2c_iface::sptr i2c_iface, const size_t reg_addr_size)
{
    return std::make_shared<i2c_regs_iface_impl>(i2c_iface, reg_addr_size);
}

mpm::types::regs_iface::sptr mpm::i2c::make_i2cdev_regs_iface(const std::string& bus,
    const uint16_t addr,
    const bool ten_bit_addr,
    const int timeout_ms,
    const size_t reg_addr_size)
{
    auto i2c_iface_sptr =
        mpm::i2c::i2c_iface::make_i2cdev(bus, addr, ten_bit_addr, timeout_ms);
    return std::make_shared<i2c_regs_iface_impl>(i2c_iface_sptr, reg_addr_size);
}
