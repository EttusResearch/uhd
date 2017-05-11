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

#include <mpm/types/regs_iface.hpp>
#include <mpm/spi/spi_iface.hpp>
#include <mpm/spi/spi_regs_iface.hpp>
#include <mpm/exception.hpp>

using mpm::types::regs_iface;

/*! SPI implementation of the regs iface
 *
 * Uses spidev
 */
class spi_regs_iface_impl : public regs_iface
{
public:

    spi_regs_iface_impl(
        mpm::spi::spi_iface::sptr spi_iface,
        uint32_t addr_shift,
        uint32_t data_shift,
        uint32_t read_flags,
        uint32_t write_flags = 0
    ) : _spi_iface(spi_iface),
        _addr_shift(addr_shift),
        _data_shift(data_shift),
        _read_flags(read_flags),
        _write_flags(write_flags)
    {
        /* nop */
    }

    uint8_t peek8(
        const uint32_t addr
    ) {
        uint32_t transaction = 0
            | (addr << _addr_shift)
            | _read_flags
        ;

        uint32_t data = _spi_iface->transfer24_8(transaction);
        if ((data & 0xFFFFFF00) != 0) {
            throw mpm::runtime_error("SPI read returned too much data");
        }

        return uint8_t(data & 0xFF);
    }

    void poke8(
        const uint32_t addr,
        const uint8_t data
    ) {
        uint32_t transaction = 0
            | _write_flags
            | (addr << _addr_shift)
            | (data << _data_shift)
        ;

        _spi_iface->transfer24_8(transaction);
    }

private:
    mpm::spi::spi_iface::sptr _spi_iface;

    uint32_t _addr_shift;
    uint32_t _data_shift;
    uint32_t _read_flags;
    uint32_t _write_flags;
};

regs_iface::sptr mpm::spi::make_spi_regs_iface(
    mpm::spi::spi_iface::sptr spi_iface,
    uint32_t addr_shift,
    uint32_t data_shift,
    uint32_t read_flags,
    uint32_t write_flags
) {
    return std::make_shared<spi_regs_iface_impl>(
        spi_iface,
        addr_shift,
        data_shift,
        read_flags,
        write_flags
    );
}

mpm::types::regs_iface::sptr mpm::spi::make_spidev_regs_iface(
    const std::string &device,
    const int speed_hz,
    const int spi_mode,
    uint32_t addr_shift,
    uint32_t data_shift,
    uint32_t read_flags,
    uint32_t write_flags
) {
    auto spi_iface_sptr = mpm::spi::spi_iface::make_spidev(
        device, speed_hz, spi_mode
    );
    return std::make_shared<spi_regs_iface_impl>(
        spi_iface_sptr,
        addr_shift,
        data_shift,
        read_flags,
        write_flags
    );
}
