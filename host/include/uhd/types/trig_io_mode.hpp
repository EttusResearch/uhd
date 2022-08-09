//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

namespace uhd {

enum class trig_io_mode_t {
    //! Output PPS signal from port
    PPS_OUTPUT,
    //! Use port as input
    INPUT,
    //! Port is turned off
    OFF
};

} // namespace uhd
