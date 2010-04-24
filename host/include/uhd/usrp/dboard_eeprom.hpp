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

#ifndef INCLUDED_UHD_USRP_DBOARD_EEPROM_HPP
#define INCLUDED_UHD_USRP_DBOARD_EEPROM_HPP

#include <uhd/config.hpp>
#include <uhd/usrp/dboard_id.hpp>
#include <uhd/types/serial.hpp>
#include <string>

namespace uhd{ namespace usrp{

struct UHD_API dboard_eeprom_t{
    /*!
     * The dboard id that was read from eeprom or will be set to eeprom.
     */
    dboard_id_t id;

    /*!
     * Create a dboard eeprom struct from the bytes read out of eeprom
     * \param buf the vector of bytes
     */
    dboard_eeprom_t(const uhd::byte_vector_t &buf = uhd::byte_vector_t(0));

    /*!
     * Get the bytes that would be written to dboard eeprom
     * \return a vector of bytes
     */
    uhd::byte_vector_t get_eeprom_bytes(void);

    /*!
     * Get the number of bytes to read out of eeprom.
     * \return the number of bytes we are interested in
     */
    static size_t num_bytes(void);
};

}} //namespace

#endif /* INCLUDED_UHD_USRP_DBOARD_EEPROM_HPP */
