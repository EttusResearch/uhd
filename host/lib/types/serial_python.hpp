//
// Copyright 2017 Ettus Research
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
