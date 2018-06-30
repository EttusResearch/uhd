//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
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
