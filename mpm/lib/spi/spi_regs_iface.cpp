//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <mpm/exception.hpp>
#include <mpm/spi/spi_iface.hpp>
#include <mpm/spi/spi_regs_iface.hpp>
#include <mpm/types/regs_iface.hpp>

using mpm::types::regs_iface;

/*! SPI implementation of the regs iface
 *
 * Uses spidev
 */
class spi_regs_iface_impl : public regs_iface
{
public:
    spi_regs_iface_impl(mpm::spi::spi_iface::sptr spi_iface,
        uint32_t addr_shift,
        uint32_t data_shift,
        uint64_t read_flags,
        uint64_t write_flags = 0)
        : _spi_iface(spi_iface)
        , _addr_shift(addr_shift)
        , _data_shift(data_shift)
        , _read_flags(read_flags)
        , _write_flags(write_flags)
    {
        /* nop */
    }

    uint8_t peek8(const uint32_t addr)
    {
        uint32_t transaction = 0 | (addr << _addr_shift) | _read_flags;

        uint32_t data = _spi_iface->transfer24_8(transaction);
        if ((data & 0xFFFFFF00) != 0) {
            throw mpm::runtime_error("SPI read returned too much data");
        }

        return data;
    }

    void poke8(const uint32_t addr, const uint8_t data)
    {
        uint32_t transaction = 0 | _write_flags | (addr << _addr_shift)
                               | (data << _data_shift);

        _spi_iface->transfer24_8(transaction);
    }

    uint16_t peek16(const uint32_t addr)
    {
        uint32_t transaction = 0 | (addr << _addr_shift) | _read_flags;

        uint32_t data = _spi_iface->transfer24_16(transaction);
        if ((data & 0xFFFF0000) != 0) {
            throw mpm::runtime_error("SPI read returned too much data");
        }

        return data;
    }

    void poke16(const uint32_t addr, const uint16_t data)
    {
        uint32_t transaction = 0 | _write_flags | (addr << _addr_shift)
                               | (data << _data_shift);

        _spi_iface->transfer24_16(transaction);
    }

    uint32_t peek32(const uint64_t addr)
    {
        /* Note: _addr_shift and _read_flags will be offset from the
         * TX portion of the message (first 2 bytes) rather than the
         * end of the message as is done with smaller transfers.
         */
        uint64_t transaction = 0 | (addr << _addr_shift) | _read_flags;

        uint64_t data = _spi_iface->transfer64_40(transaction);

        // Actual RX data is the last 5 bytes
        if ((data & 0xFFFFFF00000000F8) != 0) {
            throw mpm::runtime_error("SPI read returned too much data");
        }

        // Status data is the last byte
        bool ack = (data >> 2) & 0x1;
        uint8_t status = data & 0x3;
        if (!ack) {
            throw mpm::runtime_error("Ctrlport SPI read had no ACK");
        }
        if (status != 0) {
            // TODO: Differentiate error codes
            throw mpm::runtime_error("Ctrlport SPI error");
        }

        // Register data is the 4 bytes above the last one
        data = (data >> 8) & 0xFFFFFFFF;
        return data;
    }

    void poke32(const uint64_t addr, const uint32_t data)
    {
        /* Note: _addr_shift and _write_flags will be offset from the
         * TX portion of the message (first 6 bytes) rather than the
         * end of the message as is done with smaller transfers.
         */
        uint64_t transaction = 0 | _write_flags | (addr << _addr_shift)
                               | (data << _data_shift);

        _spi_iface->transfer64_40(transaction);
    }

private:
    mpm::spi::spi_iface::sptr _spi_iface;

    uint32_t _addr_shift;
    uint32_t _data_shift;
    uint64_t _read_flags;
    uint64_t _write_flags;
};

regs_iface::sptr mpm::spi::make_spi_regs_iface(mpm::spi::spi_iface::sptr spi_iface,
    uint32_t addr_shift,
    uint32_t data_shift,
    uint64_t read_flags,
    uint64_t write_flags)
{
    return std::make_shared<spi_regs_iface_impl>(
        spi_iface, addr_shift, data_shift, read_flags, write_flags);
}

mpm::types::regs_iface::sptr mpm::spi::make_spidev_regs_iface(const std::string& device,
    const int speed_hz,
    const int spi_mode,
    uint32_t addr_shift,
    uint32_t data_shift,
    uint64_t read_flags,
    uint64_t write_flags)
{
    auto spi_iface_sptr = mpm::spi::spi_iface::make_spidev(device, speed_hz, spi_mode);
    return std::make_shared<spi_regs_iface_impl>(
        spi_iface_sptr, addr_shift, data_shift, read_flags, write_flags);
}
