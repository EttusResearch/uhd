//
// Copyright 2017 Ettus Research (National Instruments)
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

#pragma once

#include <memory>

namespace mpm {

    /*! \brief Intermediate object to SPI
     *
     */
    class spi_iface
    {
    public:
        typedef std::shared_ptr<spi_iface> sptr;

        enum class spi_wire_mode_t {
            THREE_WIRE_MODE = 0,
            FOUR_WIRE_MODE = 1
        };

        enum class spi_endianness_t {
            LSB_FIRST = 0,
            MSB_FIRST = 1
        };

        virtual void write_byte(
            const uint16_t addr,
            const uint8_t data
        ) = 0;

        virtual void write_bytes(
            const uint16_t *addr,
            const uint8_t *data,
            const uint32_t count
        ) = 0;

        virtual uint8_t read_byte(const uint16_t addr) = 0;

        virtual void write_field(
            const uint16_t addr,
            const uint8_t field_val,
            const uint8_t mask,
            const uint8_t start_bit
        ) = 0;

        virtual uint8_t read_field(
                const uint16_t addr,
                const uint8_t mask,
                const uint8_t start_bit
        ) = 0;

        virtual spi_wire_mode_t get_wire_mode() const = 0;
        virtual spi_endianness_t get_endianness() const = 0;

        virtual size_t get_chip_select() const = 0;
    };

} /* namespace mpm */

