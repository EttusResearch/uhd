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

#ifndef INCLUDED_USRP1_IFACE_HPP
#define INCLUDED_USRP1_IFACE_HPP

#include <uhd/usrp/mboard_eeprom.hpp>
#include <uhd/types/serial.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include "usrp1_ctrl.hpp"

/*!
 * The usrp1 interface class:
 * Provides a set of functions to implementation layer.
 * Including spi, peek, poke, control...
 */
class usrp1_iface : boost::noncopyable, public uhd::i2c_iface{
public:
    typedef boost::shared_ptr<usrp1_iface> sptr;

    /*!
     * Make a new usrp1 interface with the control transport.
     * \param ctrl_transport the usrp controller object
     * \return a new usrp1 interface object
     */
    static sptr make(usrp_ctrl::sptr ctrl_transport);

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
     * Perform an spi transaction.
     * \param which_slave the slave device number
     * \param config spi config args
     * \param data the bits to write
     * \param num_bits how many bits in data
     * \param readback true to readback a value
     * \return spi data if readback set
     */
    virtual boost::uint32_t transact_spi(int which_slave,
                                         const uhd::spi_config_t &config,
                                         boost::uint32_t data,
                                         size_t num_bits,
                                         bool readback) = 0;

    /*!
     * Perform a general USB firmware OUT operation
     * \param request 
     * \param value
     * \param index 
     * \param data 
     * \return 
     */
    virtual void write_firmware_cmd(boost::uint8_t request,
                                   boost::uint16_t value,
                                   boost::uint16_t index,
                                   unsigned char* buff,
                                   boost::uint16_t length) = 0;

    uhd::usrp::mboard_eeprom_t mb_eeprom;
};

#endif /* INCLUDED_USRP1_IFACE_HPP */
