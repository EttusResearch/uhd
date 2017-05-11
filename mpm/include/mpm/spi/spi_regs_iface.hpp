//
// Copyright 2017 Ettus Research (National Instruments)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

