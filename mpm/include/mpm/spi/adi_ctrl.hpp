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

#include <mpm/spi_iface.hpp>

struct mpm_spiSettings_t
{
    static mpm_spiSettings_t* make(spiSettings_t *sps) {
        return reinterpret_cast<mpm_spiSettings_t *>(sps);
    }

    spiSettings_t spi_settings;
    mpm::spi_iface *spi_iface;
};

