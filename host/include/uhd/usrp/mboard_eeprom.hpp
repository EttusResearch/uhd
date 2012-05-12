//
// Copyright 2010-2012 Ettus Research LLC
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

#ifndef INCLUDED_UHD_USRP_MBOARD_EEPROM_HPP
#define INCLUDED_UHD_USRP_MBOARD_EEPROM_HPP

#include <uhd/config.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/types/serial.hpp>
#include <string>

namespace uhd{ namespace usrp{

    /*!
     * The motherboard EEPROM object:
     * Knows how to read and write the EEPROM for various USRPs.
     * The class inherits from a string, string dictionary.
     * Use the dictionary interface to get and set values.
     * Commit to the EEPROM to save changed settings.
     */
    struct UHD_API mboard_eeprom_t : uhd::dict<std::string, std::string>{

        //! Make a new empty mboard eeprom
        mboard_eeprom_t(void);

        /*!
         * Make a new mboard EEPROM handler.
         * \param iface the interface to i2c
         * \param which which EEPROM map to use
         */
        mboard_eeprom_t(i2c_iface &iface, const std::string &which);

        /*!
         * Write the contents of this object to the EEPROM.
         * \param iface the interface to i2c
         * \param which which EEPROM map to use
         */
        void commit(i2c_iface &iface, const std::string &which) const;

    };

}} //namespace

#endif /* INCLUDED_UHD_USRP_MBOARD_EEPROM_HPP */
