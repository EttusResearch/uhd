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

#ifndef INCLUDED_USRP_CTRL_HPP
#define INCLUDED_USRP_CTRL_HPP

#include <uhd/transport/usb_control.hpp> 
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

class usrp_ctrl : boost::noncopyable{
public:
    typedef boost::shared_ptr<usrp_ctrl> sptr;

    /*!
     * Make a usrp control object from a control transport
     * \param ctrl_transport a USB control transport
     * \return a new usrp control object
     */
    static sptr make(uhd::transport::usb_control::sptr ctrl_transport);

    /*!
     * Load firmware in Intel HEX Format onto device 
     * \param filename name of firmware file
     * \param force reload firmware if already loaded
     * \return 0 on success, error code otherwise
     */
    virtual int usrp_load_firmware(std::string filename,
                                   bool force = false) = 0;

    /*!
     * Load fpga file onto usrp 
     * \param filename name of fpga image 
     * \return 0 on success, error code otherwise
     */
    virtual int usrp_load_fpga(std::string filename) = 0;

    /*!
     * Set led usrp 
     * \param led_num which LED to control (0 or 1)
     * \param on turn LED on or off
     * \return 0 on success, error code otherwise
     */
    virtual int usrp_set_led(int led_num, bool on) = 0;

    /*!
     * Get firmware hash 
     * \param hash a size_t hash value
     * \return 0 on success, error code otherwise
     */
    virtual int usrp_get_firmware_hash(size_t &hash) = 0;

    /*!
     * Set firmware hash 
     * \param hash a size_t hash value
     * \return 0 on success, error code otherwise
     */
    virtual int usrp_set_firmware_hash(size_t hash) = 0;
                              
    /*!
     * Get fpga hash 
     * \param hash a size_t hash value
     * \return 0 on success, error code otherwise
     */
    virtual int usrp_get_fpga_hash(size_t &hash) = 0;

    /*!
     * Set fpga hash 
     * \param hash a size_t hash value
     * \return 0 on success, error code otherwise
     */
    virtual int usrp_set_fpga_hash(size_t hash) = 0;

    /*!
     * Set rx enable or disable 
     * \param on enable or disable value
     * \return 0 on success, error code otherwise
     */
    virtual int usrp_rx_enable(bool on) = 0;

    /*!
     * Set rx enable or disable 
     * \param on enable or disable value
     * \return 0 on success, error code otherwise
     */
    virtual int usrp_tx_enable(bool on) = 0;

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

};

#endif /* INCLUDED_USRP_CTRL_HPP */
