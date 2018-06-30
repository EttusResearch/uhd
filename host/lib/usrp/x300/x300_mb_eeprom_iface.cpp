//
// Copyright 2013-2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

/*
 * x300_mb_eeprom_iface
 * This interface was created to prevent MB EEPROM corruption while reading
 * data during discovery.  For devices with firmware version newer than 5.0,
 * the EEPROM data is read into firmware memory and available without
 * claiming the device.  For devices with firmware versions 5.0 and older,
 * the code makes sure to claim the device before driving the I2C bus.  This
 * has the unfortunate side effect of preventing multiple processes from
 * discovering the device simultaneously, but is far better than having EEPROM
 * corruption.
 */

#include "x300_mb_eeprom_iface.hpp"
#include "x300_fw_common.h"
#include "x300_regs.hpp"
#include "x300_impl.hpp"
#include <uhd/exception.hpp>
#include <uhd/utils/platform.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/byteswap.hpp>
#include <boost/thread.hpp>

using namespace uhd;

static const uint32_t X300_FW_SHMEM_IDENT_MIN_VERSION = 0x50001;

class x300_mb_eeprom_iface_impl : public x300_mb_eeprom_iface
{
public:

    x300_mb_eeprom_iface_impl(wb_iface::sptr wb, i2c_iface::sptr i2c) : _wb(wb), _i2c(i2c)
    {
        _compat_num = _wb->peek32(X300_FW_SHMEM_ADDR(X300_FW_SHMEM_COMPAT_NUM));
    }

    ~x300_mb_eeprom_iface_impl()
    {
        /* NOP */
    }

    /*!
     * Write bytes over the i2c.
     * \param addr the address
     * \param buf the vector of bytes
     */
    void write_i2c(
        uint16_t addr,
        const byte_vector_t &buf
    )
    {
        UHD_ASSERT_THROW(addr == MBOARD_EEPROM_ADDR);
        if (x300_impl::claim_status(_wb) != x300_impl::CLAIMED_BY_US)
        {
            throw uhd::io_error("Attempted to write MB EEPROM without claim to device.");
        }
        _i2c->write_i2c(addr, buf);
    }

    /*!
     * Read bytes over the i2c.
     * \param addr the address
     * \param num_bytes number of bytes to read
     * \return a vector of bytes
     */
    byte_vector_t read_i2c(
        uint16_t addr,
        size_t num_bytes
    )
    {
        UHD_ASSERT_THROW(addr == MBOARD_EEPROM_ADDR);
        byte_vector_t bytes;
        if (_compat_num > X300_FW_SHMEM_IDENT_MIN_VERSION)
        {
            bytes = read_eeprom(addr, 0, num_bytes);
        } else {
            x300_impl::claim_status_t status = x300_impl::claim_status(_wb);
            // Claim device before driving the I2C bus
            if (status == x300_impl::CLAIMED_BY_US or x300_impl::try_to_claim(_wb))
            {
                bytes = _i2c->read_i2c(addr, num_bytes);
                if (status != x300_impl::CLAIMED_BY_US)
                {
                    // We didn't originally have the claim, so give it up
                    x300_impl::release(_wb);
                }
            }
        }
        return bytes;
    }

    /*!
     * Write bytes to an eeprom.
     * \param addr the address
     * \param offset byte offset
     * \param buf the vector of bytes
     */
    void write_eeprom(
        uint16_t addr,
        uint16_t offset,
        const byte_vector_t &buf
    )
    {
        UHD_ASSERT_THROW(addr == MBOARD_EEPROM_ADDR);
        if (x300_impl::claim_status(_wb) != x300_impl::CLAIMED_BY_US)
        {
            throw uhd::io_error("Attempted to write MB EEPROM without claim to device.");
        }
        _i2c->write_eeprom(addr, offset, buf);
    }

    /*!
     * Read bytes from an eeprom.
     * \param addr the address
     * \param offset byte offset
     * \param num_bytes number of bytes to read
     * \return a vector of bytes
     */
    byte_vector_t read_eeprom(
        uint16_t addr,
        uint16_t offset,
        size_t num_bytes
    )
    {
        UHD_ASSERT_THROW(addr == MBOARD_EEPROM_ADDR);
        byte_vector_t bytes;
        x300_impl::claim_status_t status = x300_impl::claim_status(_wb);
        if (_compat_num >= X300_FW_SHMEM_IDENT_MIN_VERSION)
        {
            // Get MB EEPROM data from firmware memory
            if (num_bytes == 0) return bytes;

            size_t bytes_read = 0;
            for (size_t word = offset / 4; bytes_read < num_bytes; word++)
            {
                uint32_t value = byteswap(_wb->peek32(X300_FW_SHMEM_ADDR(X300_FW_SHMEM_IDENT + word)));
                for (size_t byte = offset % 4; byte < 4 and bytes_read < num_bytes; byte++)
                {
                    bytes.push_back(uint8_t((value >> (byte * 8)) & 0xff));
                    bytes_read++;
                }
            }
        } else {
            // Claim device before driving the I2C bus
            if (status == x300_impl::CLAIMED_BY_US or x300_impl::try_to_claim(_wb))
            {
                bytes = _i2c->read_eeprom(addr, offset, num_bytes);
                if (status != x300_impl::CLAIMED_BY_US)
                {
                    // We didn't originally have the claim, so give it up
                    x300_impl::release(_wb);
                }
            }
        }
        return bytes;
    }


private:
    wb_iface::sptr _wb;
    i2c_iface::sptr _i2c;
    uint32_t _compat_num;
};

x300_mb_eeprom_iface::~x300_mb_eeprom_iface(void)
{
    /* NOP */
}

x300_mb_eeprom_iface::sptr x300_mb_eeprom_iface::make(wb_iface::sptr wb, i2c_iface::sptr i2c)
{
    return boost::make_shared<x300_mb_eeprom_iface_impl>(wb, i2c->eeprom16());
}

