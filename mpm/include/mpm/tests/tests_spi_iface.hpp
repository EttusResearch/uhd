//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#pragma once

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
