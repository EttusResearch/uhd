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

#ifndef INCLUDED_UHD_USRP_MBOARD_EEPROM_HPP
#define INCLUDED_UHD_USRP_MBOARD_EEPROM_HPP

#include <uhd/config.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/types/serial.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <string>

namespace uhd{ namespace usrp{

    /*!
     * The motherboard EEPROM class:
     * Knows how to read and write the EEPROM for various USRPs.
     * The class inherits from a string, string dictionary.
     * Use the dictionary interface to get and set values.
     * Commit to the EEPROM to save changed settings.
     */
    class UHD_API mboard_eeprom_t: boost::noncopyable,
        public uhd::dict<std::string, std::string>
    {
    public:
        typedef boost::shared_ptr<mboard_eeprom_t> sptr;

        //! Possible EEPROM maps types
        enum map_type{
            MAP_NXXX,
            MAP_BXXX
        };

        /*!
         * Make a new mboard EEPROM handler.
         * \param map the map type enum
         * \param iface the interface to i2c
         * \return a new mboard EEPROM object
         */
        static sptr make(map_type map, i2c_iface &iface);

        //! Write the contents of this object to the EEPROM.
        virtual void commit(void) = 0;

    };

}} //namespace

#endif /* INCLUDED_UHD_USRP_MBOARD_EEPROM_HPP */
