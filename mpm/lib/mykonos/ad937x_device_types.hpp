//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

// Include this to get all types for ad937x_device functions

#include <uhd/types/direction.hpp>
#include <uhd/types/ranges.hpp>

namespace mpm {
    namespace ad937x {
        namespace device {
            enum class build_type_t { RELEASE, DEBUG, TEST_OBJECT };

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
                build_type_t build_type;
            };

            enum class chain_t { ONE, TWO };
        }
    }
}

