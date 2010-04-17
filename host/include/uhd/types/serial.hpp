//
// Copyright 2010 Ettus Research LLC
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

#ifndef INCLUDED_UHD_TYPES_SERIAL_HPP
#define INCLUDED_UHD_TYPES_SERIAL_HPP

#include <uhd/config.hpp>
#include <boost/cstdint.hpp>
#include <vector>

namespace uhd{

    /*!
     * Byte vector typedef for passing data in and out of I2C interfaces.
     */
    typedef std::vector<boost::uint8_t> byte_vector_t;

    /*!
     * The SPI configuration struct:
     * Used to configure a SPI transaction interface.
     */
    struct UHD_API spi_config_t{
        /*!
         * The edge type specifies when data is valid
         * relative to the edge of the serial clock.
         */
        enum edge_t{
            EDGE_RISE = 'r',
            EDGE_FALL = 'f'
        };

        //! on what edge is the mosi data valid?
        edge_t mosi_edge;

        //! on what edge is the miso data valid?
        edge_t miso_edge;

        /*!
         * Create a new spi config.
         * \param edge the default edge for mosi and miso
         */
        spi_config_t(edge_t edge = EDGE_RISE);
    };

} //namespace uhd

#endif /* INCLUDED_UHD_TYPES_SERIAL_HPP */
