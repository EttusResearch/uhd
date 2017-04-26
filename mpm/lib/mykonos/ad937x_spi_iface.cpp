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

#include <mpm/ad937x/ad937x_spi_iface.hpp>
#include <mpm/spi/spi_regs_iface.hpp>

using namespace mpm::spi;

static const int    MYK_SPI_SPEED_HZ = 1000000;
static const size_t MYK_ADDR_SHIFT = 8;
static const size_t MYK_DATA_SHIFT = 0;
static const size_t MYK_READ_FLAG = 1 << 23;
static const size_t MYK_WRITE_FLAG = 0;

mpm::types::regs_iface::sptr mpm::chips::make_ad937x_iface(
        const std::string &spi_device
) {
    return make_spi_regs_iface(
        spi_iface::make_spidev(spi_device, MYK_SPI_SPEED_HZ),
        MYK_ADDR_SHIFT,
        MYK_DATA_SHIFT,
        MYK_READ_FLAG,
        MYK_WRITE_FLAG
    );
}


