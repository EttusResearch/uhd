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

#ifndef INCLUDED_UHD_USRP_DBOARD_EEPROM_HPP
#define INCLUDED_UHD_USRP_DBOARD_EEPROM_HPP

#include <uhd/config.hpp>
#include <uhd/usrp/dboard_id.hpp>
#include <uhd/types/serial.hpp>
#include <string>

namespace uhd{ namespace usrp{

struct UHD_API dboard_eeprom_t{

    //! The ID for the daughterboard type
    dboard_id_t id;

    //! The unique serial number
    std::string serial;

    //! A hardware revision number
    std::string revision;

    /*!
     * Create an empty dboard eeprom struct.
     */
    dboard_eeprom_t(void);

    /*!
     * Load the object with bytes from the eeprom.
     * \param iface the serial interface with i2c
     * \param addr the i2c address for the eeprom
     */
    void load(i2c_iface &iface, boost::uint8_t addr);

    /*!
     * Store the object to bytes in the eeprom.
     * \param iface the serial interface with i2c
     * \param addr the i2c address for the eeprom
     */
    void store(i2c_iface &iface, boost::uint8_t addr) const;

};

}} //namespace

#endif /* INCLUDED_UHD_USRP_DBOARD_EEPROM_HPP */
