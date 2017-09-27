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

