//
// Copyright 2010-2013 Ettus Research LLC
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
#include <uhd/usrp/mboard_eeprom.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <boost/function.hpp>
#include "usrp2_regs.hpp"
#include <uhd/types/wb_iface.hpp>
#include <string>

/*!
 * The usrp2 interface class:
 * Provides a set of functions to implementation layer.
 * Including spi, peek, poke, control...
 */
class usrp2_iface : public uhd::timed_wb_iface, public uhd::spi_iface, public uhd::i2c_iface
{
public:
    typedef boost::shared_ptr<usrp2_iface> sptr;
    /*!
     * Make a new usrp2 interface with the control transport.
     * \param ctrl_transport the udp transport object
     * \return a new usrp2 interface object
     */
    static sptr make(uhd::transport::udp_simple::sptr ctrl_transport);

    //! poke a register in the virtual fw table
    virtual void pokefw(wb_addr_type addr, boost::uint32_t data) = 0;

    //! peek a register in the virtual fw table
    virtual boost::uint32_t peekfw(wb_addr_type addr) = 0;

    //! The list of possible revision types
    enum rev_type {
        USRP2_REV3 = 3,
        USRP2_REV4 = 4,
        USRP_N200 = 200,
        USRP_N200_R4 = 201,
        USRP_N210 = 210,
        USRP_N210_R4 = 211,
        USRP_NXXX = 0
    };

    //! Get the revision type for this device
    virtual rev_type get_rev(void) = 0;

    //! Get the canonical name for this device
    virtual const std::string get_cname(void) = 0;

    //! Lock the device to this iface
    virtual void lock_device(bool lock) = 0;

    //! Is this device locked?
    virtual bool is_device_locked(void) = 0;

    //! A version string for firmware
    virtual const std::string get_fw_version_string(void) = 0;

    //! Construct a helpful warning message for images
    virtual std::string images_warn_help_message(void) = 0;

    //motherboard eeprom map structure
    uhd::usrp::mboard_eeprom_t mb_eeprom;
};

#endif /* INCLUDED_USRP2_IFACE_HPP */
