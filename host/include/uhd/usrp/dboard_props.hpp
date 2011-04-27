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

#ifndef INCLUDED_UHD_USRP_DBOARD_PROPS_HPP
#define INCLUDED_UHD_USRP_DBOARD_PROPS_HPP

#include <uhd/utils/props.hpp>

namespace uhd{ namespace usrp{

    /*!
     * Possible device dboard properties:
     *    A dboard has an id, one or more subdevices, and a codec.
     *    A dboard is considered to be unidirectional (RX or TX).
     */
    enum dboard_prop_t{
        DBOARD_PROP_NAME,           //ro, std::string
        DBOARD_PROP_SUBDEV,         //ro, wax::obj
        DBOARD_PROP_SUBDEV_NAMES,   //ro, prop_names_t
        DBOARD_PROP_DBOARD_EEPROM,  //rw, dboard_eeprom_t
        DBOARD_PROP_GBOARD_EEPROM,  //rw, dboard_eeprom_t
        DBOARD_PROP_DBOARD_IFACE,   //ro, dboard_iface::sptr
        DBOARD_PROP_CODEC,          //ro, wax::obj
        DBOARD_PROP_GAIN_GROUP      //ro, gain_group
    }; 

}} //namespace

#endif /* INCLUDED_UHD_USRP_DBOARD_PROPS_HPP */
