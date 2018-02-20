//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <mpm/ad937x/ad937x_spi_iface.hpp>
#include <mpm/spi/spi_regs_iface.hpp>

using namespace mpm::spi;

static const int    MYK_SPI_SPEED_HZ = 20000000;
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


