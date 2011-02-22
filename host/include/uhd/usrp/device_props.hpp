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

#ifndef INCLUDED_UHD_USRP_DEVICE_PROPS_HPP
#define INCLUDED_UHD_USRP_DEVICE_PROPS_HPP

#include <uhd/utils/props.hpp>

namespace uhd{ namespace usrp{

    /*!
     * Possible device properties:
     *   In general, a device will have a single mboard.
     *   In certain mimo applications, multiple boards
     *   will be present in the interface for configuration.
     */
    enum device_prop_t{
        DEVICE_PROP_NAME,            //ro, std::string
        DEVICE_PROP_MBOARD,          //ro, wax::obj
        DEVICE_PROP_MBOARD_NAMES,    //ro, prop_names_t
    };

}} //namespace

#endif /* INCLUDED_UHD_USRP_DEVICE_PROPS_HPP */
