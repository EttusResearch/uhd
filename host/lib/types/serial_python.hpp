//
// Copyright 2017-2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_SERIAL_PYTHON_HPP
#define INCLUDED_UHD_SERIAL_PYTHON_HPP

#include <uhd/types/serial.hpp>

void export_spi_config()
{
    using spi_config_t = uhd::spi_config_t;
    using spi_edge_t   = spi_config_t::edge_t;

    bp::enum_<spi_edge_t>("spi_edge")
        .value("EDGE_RISE" , spi_edge_t::EDGE_RISE)
        .value("EDGE_FALL",  spi_edge_t::EDGE_FALL)
        ;

    bp::class_<spi_config_t>("spi_config", bp::init<spi_edge_t>())

        // Properties
        .add_property("mosi_edge"         , &spi_config_t::mosi_edge         )
        .add_property("miso_edge"         , &spi_config_t::miso_edge         )
        .add_property("use_custom_divider", &spi_config_t::use_custom_divider)
        .add_property("divider"           , &spi_config_t::divider           )
        ;
}

#endif /* INCLUDED_UHD_SERIAL_PYTHON_HPP */
