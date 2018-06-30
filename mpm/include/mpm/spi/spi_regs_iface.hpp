//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <mpm/types/regs_iface.hpp>
#include <mpm/spi/spi_iface.hpp>

namespace mpm { namespace spi {

    mpm::types::regs_iface::sptr make_spi_regs_iface(
        mpm::spi::spi_iface::sptr spi_iface,
        uint32_t addr_shift,
        uint32_t data_shift,
        uint32_t read_flags,
        uint32_t write_flags = 0
    );

    /*! Convenience factory for regs_iface based on SPI based on spidev
     */
    mpm::types::regs_iface::sptr make_spidev_regs_iface(
        const std::string &device,
        const int speed_hz,
        const int spi_mode,
        uint32_t addr_shift,
        uint32_t data_shift,
        uint32_t read_flags,
        uint32_t write_flags = 0
    );

}}; /* namespace mpm::spi */

