//
// Copyright 2013-2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
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

class spi_core_3000 : uhd::noncopyable, public uhd::spi_iface
{
public:
    using sptr        = std::shared_ptr<spi_core_3000>;
    using poke32_fn_t = std::function<void(uint32_t, uint32_t)>;
    using peek32_fn_t = std::function<uint32_t(uint32_t)>;

    ~spi_core_3000(void) override = 0;

    //! makes a new spi core from iface and slave base
    static sptr make(uhd::wb_iface::sptr iface, const size_t base, const size_t readback);

    //! makes a new spi core from register iface and slave base
    static sptr make(poke32_fn_t&& poke32_fn,
        peek32_fn_t&& peek32_fn,
        const size_t base,
        const size_t reg_offset,
        const size_t readback);

    //! Set the spi clock divider to something usable
    virtual void set_divider(const double div) = 0;
};
