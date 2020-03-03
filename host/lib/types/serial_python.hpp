//
// Copyright 2017-2018 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_SERIAL_PYTHON_HPP
#define INCLUDED_UHD_SERIAL_PYTHON_HPP

#include <uhd/types/serial.hpp>

void export_spi_config(py::module& m)
{
    using spi_config_t = uhd::spi_config_t;
    using spi_edge_t   = spi_config_t::edge_t;

    py::enum_<spi_edge_t>(m, "spi_edge")
        .value("EDGE_RISE", spi_edge_t::EDGE_RISE)
        .value("EDGE_FALL", spi_edge_t::EDGE_FALL);

    py::class_<spi_config_t>(m, "spi_config")
        .def(py::init<spi_edge_t>())

        // Properties
        .def_readwrite("mosi_edge", &spi_config_t::mosi_edge)
        .def_readwrite("miso_edge", &spi_config_t::miso_edge)
        .def_readwrite("use_custom_divider", &spi_config_t::use_custom_divider)
        .def_readwrite("divider", &spi_config_t::divider);
}

#endif /* INCLUDED_UHD_SERIAL_PYTHON_HPP */
