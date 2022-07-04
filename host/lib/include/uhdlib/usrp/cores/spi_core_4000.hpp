//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/types/serial.hpp>
#include <uhd/types/wb_iface.hpp>
#include <uhd/utils/noncopyable.hpp>
#include <uhdlib/usrp/cores/gpio_port_mapper.hpp>
#include <functional>
#include <memory>

namespace uhd { namespace cores {
class spi_core_4000 : uhd::noncopyable, public uhd::spi_iface
{
public:
    using sptr        = std::shared_ptr<spi_core_4000>;
    using mapper_sptr = std::shared_ptr<uhd::mapper::gpio_port_mapper>;
    using poke32_fn_t = std::function<void(uint32_t, uint32_t)>;
    using peek32_fn_t = std::function<uint32_t(uint32_t)>;

    virtual ~spi_core_4000(void) = default;

    //! makes a new spi core from iface and peripheral base
    static sptr make(uhd::wb_iface::sptr iface, const size_t base, const size_t readback);

    //! makes a new spi core from register iface and peripheral
    static sptr make(poke32_fn_t&& poke32_fn,
        peek32_fn_t&& peek_fn,
        const size_t spi_periph_cfg,
        const size_t spi_transaction_cfg,
        const size_t spi_transaction_go,
        const size_t spi_status,
        const size_t spi_controller_info,
        const mapper_sptr port_mapper);

    //! Configures the SPI transaction. The vector index refers to the peripheral number.
    virtual void set_spi_periph_config(
        const std::vector<uhd::features::spi_periph_config_t>& spi_periph_configs) = 0;
};

}} // namespace uhd::cores
