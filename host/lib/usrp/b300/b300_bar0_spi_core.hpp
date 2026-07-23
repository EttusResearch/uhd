//
// Copyright 2026 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/types/serial.hpp>
#include <uhd/types/wb_iface.hpp>
#include <uhd/utils/noncopyable.hpp>
#include <functional>
#include <memory>

// Copied from spi_core_3000 but with an added wait until ready loop.
class b300_bar0_spi_core : uhd::noncopyable, public uhd::spi_iface
{
public:
    using sptr        = std::shared_ptr<b300_bar0_spi_core>;
    using poke32_fn_t = std::function<void(uint32_t, uint32_t)>;
    using peek32_fn_t = std::function<uint32_t(uint32_t)>;

    ~b300_bar0_spi_core(void) override = 0;

    //! makes a new spi core from iface and slave base
    static sptr make(uhd::wb_iface::sptr iface, const size_t base, const size_t readback);

    //! Set the spi clock divider to something usable
    virtual void set_divider(const double div) = 0;
};
