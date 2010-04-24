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

#ifndef INCLUDED_USRP2_IFACE_HPP
#define INCLUDED_USRP2_IFACE_HPP

#include <uhd/transport/udp_simple.hpp>
#include <uhd/types/serial.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <boost/cstdint.hpp>
#include "fw_common.h"

////////////////////////////////////////////////////////////////////////
// I2C addresses
////////////////////////////////////////////////////////////////////////
#define I2C_DEV_EEPROM  0x50 // 24LC02[45]:  7-bits 1010xxx
#define	I2C_ADDR_MBOARD (I2C_DEV_EEPROM | 0x0)
#define	I2C_ADDR_TX_DB  (I2C_DEV_EEPROM | 0x4)
#define	I2C_ADDR_RX_DB  (I2C_DEV_EEPROM | 0x5)
////////////////////////////////////////////////////////////////////////

/*!
 * The usrp2 interface class:
 * Provides a set of functions to implementation layer.
 * Including spi, peek, poke, control...
 */
class usrp2_iface : boost::noncopyable{
public:
    typedef boost::shared_ptr<usrp2_iface> sptr;

    /*!
     * Make a new usrp2 interface with the control transport.
     * \param ctrl_transport the udp transport object
     * \return a new usrp2 interface object
     */
    static sptr make(uhd::transport::udp_simple::sptr ctrl_transport);

    /*!
     * Perform a control transaction.
     * \param data a control data struct
     * \return the result control data
     */
    virtual usrp2_ctrl_data_t ctrl_send_and_recv(const usrp2_ctrl_data_t &data) = 0;

    /*!
     * Write a register (32 bits)
     * \param addr the address
     * \param data the 32bit data
     */
    virtual void poke32(boost::uint32_t addr, boost::uint32_t data) = 0;

    /*!
     * Read a register (32 bits)
     * \param addr the address
     * \return the 32bit data
     */
    virtual boost::uint32_t peek32(boost::uint32_t addr) = 0;

    /*!
     * Write a register (16 bits)
     * \param addr the address
     * \param data the 16bit data
     */
    virtual void poke16(boost::uint32_t addr, boost::uint16_t data) = 0;

    /*!
     * Read a register (16 bits)
     * \param addr the address
     * \return the 16bit data
     */
    virtual boost::uint16_t peek16(boost::uint32_t addr) = 0;

    /*!
     * Perform an spi transaction.
     * \param which_slave the slave device number
     * \param config spi config args
     * \param data the bits to write
     * \param num_bits how many bits in data
     * \param readback true to readback a value
     * \return spi data if readback set
     */
    virtual boost::uint32_t transact_spi(
        int which_slave,
        const uhd::spi_config_t &config,
        boost::uint32_t data,
        size_t num_bits,
        bool readback
    ) = 0;

    /*!
     * Write bytes over the i2c.
     * \param addr the address
     * \param buf the vector of bytes
     */
    virtual void write_i2c(
        boost::uint8_t addr,
        const uhd::byte_vector_t &buf
    ) = 0;

    /*!
     * Read bytes over the i2c.
     * \param addr the address
     * \param num_bytes number of bytes to read
     * \return a vector of bytes
     */
    virtual uhd::byte_vector_t read_i2c(
        boost::uint8_t addr,
        size_t num_bytes
    ) = 0;

    /*!
     * Write bytes to an eeprom.
     * \param addr the address
     * \param offset byte offset
     * \param buf the vector of bytes
     */
    virtual void write_eeprom(
        boost::uint8_t addr,
        boost::uint8_t offset,
        const uhd::byte_vector_t &buf
    ) = 0;

    /*!
     * Read bytes from an eeprom.
     * \param addr the address
     * \param offset byte offset
     * \param num_bytes number of bytes to read
     * \return a vector of bytes
     */
    virtual uhd::byte_vector_t read_eeprom(
        boost::uint8_t addr,
        boost::uint8_t offset,
        size_t num_bytes
    ) = 0;

    /*!
     * Get the master clock frequency.
     * \return the frequency in Hz
     */
    virtual double get_master_clock_freq(void) = 0;
};

#endif /* INCLUDED_USRP2_IFACE_HPP */
