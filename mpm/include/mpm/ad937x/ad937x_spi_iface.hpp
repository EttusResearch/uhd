//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <mpm/types/regs_iface.hpp>

namespace mpm { namespace chips {

    /*! Return a peek/poke interface to the LMK04828
     *
     * Assumption is it is attached to a spidev
     */
    mpm::types::regs_iface::sptr make_ad937x_iface(
            const std::string &spi_device
    );

}}; /* namespace mpm::chips */

