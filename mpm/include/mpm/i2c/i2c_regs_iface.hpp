//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <mpm/i2c/i2c_iface.hpp>
#include <mpm/types/regs_iface.hpp>

namespace mpm { namespace i2c {

/*! The regs_iface class can only be used for certain i2c devices
 * For more control over the length of write and read data, use the lower-level
 * i2c_iface
 */
mpm::types::regs_iface::sptr make_i2c_regs_iface(
    mpm::i2c::i2c_iface::sptr i2c_iface, const size_t reg_addr_size);

/*! Convenience factory for regs_iface based on i2c based on i2cdev
 */
mpm::types::regs_iface::sptr make_i2cdev_regs_iface(const std::string& bus,
    const uint16_t addr,
    const bool ten_bit_addr,
    const int timeout_ms,
    const size_t reg_addr_size);

}}; /* namespace mpm::i2c */
