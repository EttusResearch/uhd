//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

extern "C" {
#include "mpm/rfdc/rfdc_throw.h"
}
#include <mpm/exception.hpp>
#include <string>

/**
 * A function to throw MPM exceptions from within the Xilinx RFdc API
 */
void rfdc_throw(const char* msg)
{
    if (msg) {
        std::string error_msg(msg);
        throw mpm::assertion_error("Error in RFDC code: " + error_msg);
    } else {
        throw mpm::assertion_error("Error in RFDC code.");
    }
}
