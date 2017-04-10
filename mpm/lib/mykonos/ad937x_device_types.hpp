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

// Include this to get all types for ad937x_device functions

#include <uhd/types/direction.hpp>
#include <uhd/types/ranges.hpp>

namespace mpm {
    namespace ad937x {
        namespace device {

            struct api_version_t {
                uint32_t silicon_ver;
                uint32_t major_ver;
                uint32_t minor_ver;
                uint32_t build_ver;
            };

            struct arm_version_t {
                uint8_t major_ver;
                uint8_t minor_ver;
                uint8_t rc_ver;
            };

            enum class chain_t { ONE, TWO };
        }
    }
}

