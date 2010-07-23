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
        DBOARD_PROP_NAME         = 'n', //ro, std::string
        DBOARD_PROP_SUBDEV       = 's', //ro, wax::obj
        DBOARD_PROP_SUBDEV_NAMES = 'S', //ro, prop_names_t
        DBOARD_PROP_USED_SUBDEVS = 'u', //ro, prop_names_t
        DBOARD_PROP_DBOARD_ID    = 'i', //rw, dboard_id_t
        DBOARD_PROP_DBOARD_IFACE = 'f', //ro, dboard_iface::sptr
        DBOARD_PROP_CODEC        = 'c'  //ro, wax::obj
    }; 

}} //namespace

#endif /* INCLUDED_UHD_USRP_DBOARD_PROPS_HPP */
