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

#include <mpm/spi_iface.hpp>
#include <unordered_map>
#include <memory>
namespace mpm {
    class tests_spi_iface : public virtual spi_iface
    {
    public:

        /**************************************************************************
         * spi_iface API calls
         *************************************************************************/

        typedef std::shared_ptr<tests_spi_iface> sptr;
        static sptr make(){
            return std::make_shared<tests_spi_iface>();
        };

        void write_byte(
            const uint16_t addr,
            const uint8_t data
            );

        void write_bytes(
            const uint16_t *addr,
            const uint8_t *data,
            const uint32_t count
            );

        uint8_t read_byte(const uint16_t addr);

        void write_field(
            const uint16_t addr,
            const uint8_t field_val,
            const uint8_t mask,
            const uint8_t start_bit
            );

        uint8_t read_field(
            const uint16_t addr,
            const uint8_t mask,
            const uint8_t start_bit
            );
        spi_wire_mode_t get_wire_mode() const;
        spi_endianness_t get_endianness() const;
        size_t get_chip_select() const;

    private:
        std::unordered_map<uint16_t, uint8_t> _regs;
        uint8_t _default_val = 0;
    };
}
