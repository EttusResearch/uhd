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

#include <mpm/chips/lmk04828_spi_iface.hpp>
#include <mpm/spi/spi_regs_iface.hpp>

using namespace mpm::spi;

static const int LMK_SPI_SPEED_HZ = 1000000;
static const size_t LMK_ADDR_SHIFT = 8;
static const size_t LMK_DATA_SHIFT = 0;
static const size_t LMK_READ_FLAG = 1 << 23;
static const size_t LMK_WRITE_FLAG = 0;

mpm::types::regs_iface::sptr mpm::chips::make_lmk04828_iface(
        const std::string &spi_device
) {
    return make_spi_regs_iface(
        spi_iface::make_spidev(spi_device, LMK_SPI_SPEED_HZ),
        LMK_ADDR_SHIFT,
        LMK_DATA_SHIFT,
        LMK_READ_FLAG,
        LMK_WRITE_FLAG
    );
}

