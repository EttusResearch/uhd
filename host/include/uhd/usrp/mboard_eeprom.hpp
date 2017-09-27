//
// Copyright 2010-2012 Ettus Research LLC
// Copyright 2017 Ettus Research (National Instruments Corp.)
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

#include <uhd/types/dict.hpp>
#include <string>

namespace uhd{ namespace usrp{

    /*!  The motherboard EEPROM object.
     *
     * The specific implementation knows how to read and write the EEPROM for
     * various USRPs. By itself, this class is nothing but a thin wrapper
     * around a string -> string dictionary.
     *
     * Note that writing to an object of type mboard_eeprom_t does not actually
     * write to the EEPROM. Devices have their own APIs to read/write from the
     * EEPROM chips themselves. Most devices will write the EEPROM itself when
     * the according property is updated.
     */
    using mboard_eeprom_t = uhd::dict<std::string, std::string>;

}} // namespace uhd::usrp

#endif /* INCLUDED_UHD_USRP_MBOARD_EEPROM_HPP */
