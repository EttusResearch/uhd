//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <mpm/types/regs_iface.hpp>
#include <chrono>

struct ad9371_spiSettings_t
{
    static ad9371_spiSettings_t* make(spiSettings_t *sps) {
        return reinterpret_cast<ad9371_spiSettings_t *>(sps);
    }

    explicit ad9371_spiSettings_t(mpm::types::regs_iface*);

    // spiSetting_t MUST be the first data member so that the
    // reinterpret_cast in make() works
    spiSettings_t spi_settings;
    mpm::types::regs_iface* spi_iface;
    std::chrono::time_point<std::chrono::steady_clock> timeout_start;
    std::chrono::microseconds timeout_duration;
};

enum ad9371_spi_errors_t : uint32_t
{
    SPI_READ_ERROR  = 4096,
    SPI_WRITE_ERROR = 4097,
};

