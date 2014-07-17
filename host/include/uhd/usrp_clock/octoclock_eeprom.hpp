//
// Copyright 2014 Ettus Research LLC
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

#ifndef INCLUDED_UHD_USRP_CLOCK_OCTOCLOCK_EEPROM_HPP
#define INCLUDED_UHD_USRP_CLOCK_OCTOCLOCK_EEPROM_HPP

#include <uhd/config.hpp>
#include <uhd/transport/udp_simple.hpp>
#include <uhd/types/dict.hpp>
#include <string>

namespace uhd{ namespace usrp_clock{

/*!
 * The OctoClock EEPROM object:
 * Knows how to read and write the OctoClock EEPROM.
 * The class inherits from a string, string dictionary.
 * Use the dictionary interface to get and set values.
 * Commit to the EEPROM to save changed settings.
 */
class UHD_API octoclock_eeprom_t : public uhd::dict<std::string, std::string>{
public:
    //! Make a new empty OctoClock EEPROM handler
    octoclock_eeprom_t(void);

    /*!
     * Make a new OctoClock EEPROM handler.
     * \param transport the UDP transport to the OctoClock
     */
    octoclock_eeprom_t(transport::udp_simple::sptr transport);

    /*!
     * Write the contents of this object to the EEPROM.
     */
    void commit() const;

private:
    transport::udp_simple::sptr xport;
    void _load();
    void _store() const;

};

} //namespace
} //namespace

#endif /* INCLUDED_UHD_USRP_CLOCK_OCTOCLOCK_EEPROM_HPP */
