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

#ifndef INCLUDED_USRP_CTRL_HPP
#define INCLUDED_USRP_CTRL_HPP

#include <uhd/transport/usb_control.hpp>
#include <uhd/types/serial.hpp> //i2c iface
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

namespace uhd{ namespace usrp{

class fx2_ctrl : boost::noncopyable, public uhd::i2c_iface{
public:
    typedef boost::shared_ptr<fx2_ctrl> sptr;

    /*!
     * Make a usrp control object from a control transport
     * \param ctrl_transport a USB control transport
     * \return a new usrp control object
     */
    static sptr make(uhd::transport::usb_control::sptr ctrl_transport);

    //! Call init after the fpga is loaded
    virtual void usrp_init(void) = 0;

    /*!
     * Load firmware in Intel HEX Format onto device 
     * \param filename name of firmware file
     * \param force reload firmware if already loaded
     */
    virtual void usrp_load_firmware(std::string filename,
                                   bool force = false) = 0;

    /*!
     * Load fpga file onto usrp 
     * \param filename name of fpga image 
     */
    virtual void usrp_load_fpga(std::string filename) = 0;

    /*!
     * Load USB descriptor file in Intel HEX format into EEPROM
     * \param filename name of EEPROM image
     */
    virtual void usrp_load_eeprom(std::string filestring) = 0;
    
    /*!
     * Submit an IN transfer 
     * \param request device specific request 
     * \param value device specific field
     * \param index device specific field
     * \param buff buffer to place data
     * \return number of bytes read or error 
     */
    virtual int usrp_control_read(boost::uint8_t request,
                                  boost::uint16_t value,
                                  boost::uint16_t index,
                                  unsigned char *buff,
                                  boost::uint16_t length) = 0;

    /*!
     * Submit an OUT transfer 
     * \param request device specific request 
     * \param value device specific field
     * \param index device specific field
     * \param buff buffer of data to be sent 
     * \return number of bytes written or error 
     */
    virtual int usrp_control_write(boost::uint8_t request,
                                   boost::uint16_t value,
                                   boost::uint16_t index,
                                   unsigned char *buff,
                                   boost::uint16_t length) = 0;

    /*!
     * Perform an I2C write
     * \param i2c_addr I2C device address
     * \param buf data to be written 
     * \param len length of data in bytes
     * \return number of bytes written or error 
     */

    virtual int usrp_i2c_write(boost::uint16_t i2c_addr,
                               unsigned char *buf, 
                               boost::uint16_t len) = 0;

    /*!
     * Perform an I2C read
     * \param i2c_addr I2C device address
     * \param buf data to be read 
     * \param len length of data in bytes
     * \return number of bytes read or error 
     */

    virtual int usrp_i2c_read(boost::uint16_t i2c_addr,
                               unsigned char *buf, 
                               boost::uint16_t len) = 0;

    //! enable/disable the rx path
    virtual void usrp_rx_enable(bool on) = 0;

    //! enable/disable the tx path
    virtual void usrp_tx_enable(bool on) = 0;
};

}} //namespace uhd::usrp

#endif /* INCLUDED_USRP_CTRL_HPP */
