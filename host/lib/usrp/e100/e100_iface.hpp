//
// Copyright 2010-2011 Ettus Research LLC
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

#ifndef INCLUDED_E100_IFACE_HPP
#define INCLUDED_E100_IFACE_HPP

#include <uhd/transport/udp_simple.hpp>
#include <uhd/usrp/mboard_eeprom.hpp>
#include <uhd/types/serial.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <boost/cstdint.hpp>
#include <uhd/usrp/mboard_iface.hpp>

////////////////////////////////////////////////////////////////////////
// I2C addresses
////////////////////////////////////////////////////////////////////////
#define I2C_DEV_EEPROM  0x50 // 24LC02[45]:  7-bits 1010xxx
#define	I2C_ADDR_MBOARD (I2C_DEV_EEPROM | 0x0)
#define	I2C_ADDR_TX_DB  (I2C_DEV_EEPROM | 0x4)
#define	I2C_ADDR_RX_DB  (I2C_DEV_EEPROM | 0x5)
////////////////////////////////////////////////////////////////////////

/*!
 * The usrp-e interface class:
 * Provides a set of functions to implementation layer.
 * Including spi, peek, poke, control...
 */
class e100_iface : boost::noncopyable, public uhd::usrp::mboard_iface{
public:
    typedef boost::shared_ptr<e100_iface> sptr;

    /*!
     * Make a new usrp-e interface with the control transport.
     * \return a new usrp-e interface object
     */
    static sptr make(void);

    //! TODO implement this for multiple hardwares revs in the future
    std::string get_cname(void){
        return "USRP-E100";
    }

    /*!
     * Get the underlying file descriptor.
     * \return the file descriptor
     */
    virtual int get_file_descriptor(void) = 0;

    /*!
     * Open a device node into this iface.
     * \param node the device node name
     */
    virtual void open(const std::string &node) = 0;

    /*!
     * Close the open device node in this iface.
     */
    virtual void close(void) = 0;

    /*!
     * Perform an ioctl call on the device node file descriptor.
     * This will throw when the internal ioctl call fails.
     * \param request the control word
     * \param mem pointer to some memory
     */
    virtual void ioctl(int request, void *mem) = 0;

    //! Get the I2C interface for the I2C device node
    virtual uhd::i2c_iface &get_i2c_dev_iface(void) = 0;

    //motherboard eeprom map structure
    uhd::usrp::mboard_eeprom_t mb_eeprom;
};

#endif /* INCLUDED_E100_IFACE_HPP */
