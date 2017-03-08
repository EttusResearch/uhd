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

#include <mpm/tests/tests_spi_iface.hpp>

       /**************************************************************************
         * spi_iface API calls
         *************************************************************************/
namespace mpm{

    void tests_spi_iface::write_byte(
            const uint16_t addr,
            const uint8_t data
            ) {
            _regs[addr] = data;
        }

    void tests_spi_iface::write_bytes(
            const uint16_t *addr,
            const uint8_t *data,
            const uint32_t count
            ) {
            for (size_t i = 0; i < count; i++) {
                _regs[addr[i]] = data[i];
            }
        }

    uint8_t tests_spi_iface::read_byte(const uint16_t addr)
        {
            if (_regs.count(addr)) {
                return _regs.at(addr);
            }
            return _default_val;
        }

    void tests_spi_iface::write_field(
            const uint16_t addr,
            const uint8_t field_val,
            const uint8_t mask,
            const uint8_t start_bit
            ) {
            const uint8_t old_value = read_byte(addr);
            _regs[addr] = (old_value & ~mask) | ((field_val << start_bit) & mask);
        }

    uint8_t tests_spi_iface::read_field(
            const uint16_t addr,
            const uint8_t mask,
            const uint8_t start_bit
            ) {
            return (read_byte(addr) & mask) >> start_bit;
        }


    spi_iface::spi_wire_mode_t tests_spi_iface::get_wire_mode() const{
        return spi_iface::spi_wire_mode_t::THREE_WIRE_MODE;
    }

    spi_iface::spi_endianness_t tests_spi_iface::get_endianness() const{
        return spi_iface::spi_endianness_t::LSB_FIRST;
    }
    size_t tests_spi_iface::get_chip_select() const{
        return 0;
    }
}
