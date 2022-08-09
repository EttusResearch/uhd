//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/features/discoverable_feature.hpp>
#include <uhd/types/serial.hpp>
#include <memory>

namespace uhd { namespace features {

/*!
 * The SPI peripheral configuration struct:
 * Used to configure the GPIO lines for SPI transactions
 */
struct UHD_API spi_periph_config_t
{
    //! Indicates which GPIO line to use for the CS signal.
    uint8_t periph_cs;

    //! Indicates which GPIO line to use for the SDI signal.
    uint8_t periph_sdi;

    //! Indicates which GPIO line to use for the SDO signal.
    uint8_t periph_sdo;

    //! Indicates which GPIO line to use for the SCLK signal.
    uint8_t periph_clk;
};

/*! Interface to provide access to SPI Interface.
 */
class UHD_API spi_getter_iface : public discoverable_feature
{
public:
    using sptr = std::shared_ptr<spi_getter_iface>;

    static discoverable_feature::feature_id_t get_feature_id()
    {
        return discoverable_feature::SPI_GETTER_IFACE;
    }

    std::string get_feature_name() const
    {
        return "SPI Getter Interface";
    }

    virtual ~spi_getter_iface() = default;

    /*! Return the SPI interface to read and write on.
     * \return SPI interface
     */
    virtual uhd::spi_iface::sptr get_spi_ref(
        const std::vector<uhd::features::spi_periph_config_t>& spi_periph_config)
        const = 0;
};

}} // namespace uhd::features
