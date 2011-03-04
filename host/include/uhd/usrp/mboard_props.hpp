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

#ifndef INCLUDED_UHD_USRP_MBOARD_PROPS_HPP
#define INCLUDED_UHD_USRP_MBOARD_PROPS_HPP

#include <uhd/utils/props.hpp>

namespace uhd{ namespace usrp{

    /*!
     * Possible device mboard properties:
     *   The general mboard properties are listed below.
     *   Custom properties can be identified with a string
     *   and discovered though the others property.
     */
    enum mboard_prop_t{
        MBOARD_PROP_NAME,                    //ro, std::string
        MBOARD_PROP_OTHERS,                  //ro, prop_names_t
        MBOARD_PROP_SENSOR,                  //ro, sensor_value_t
        MBOARD_PROP_SENSOR_NAMES,            //ro, prop_names_t
        MBOARD_PROP_CLOCK_RATE,              //rw, double
        MBOARD_PROP_RX_DSP,                  //ro, wax::obj
        MBOARD_PROP_RX_DSP_NAMES,            //ro, prop_names_t
        MBOARD_PROP_TX_DSP,                  //ro, wax::obj
        MBOARD_PROP_TX_DSP_NAMES,            //ro, prop_names_t
        MBOARD_PROP_RX_DBOARD,               //ro, wax::obj
        MBOARD_PROP_RX_DBOARD_NAMES,         //ro, prop_names_t
        MBOARD_PROP_TX_DBOARD,               //ro, wax::obj
        MBOARD_PROP_TX_DBOARD_NAMES,         //ro, prop_names_t
        MBOARD_PROP_RX_SUBDEV_SPEC,          //rw, subdev_spec_t
        MBOARD_PROP_TX_SUBDEV_SPEC,          //rw, subdev_spec_t
        MBOARD_PROP_CLOCK_CONFIG,            //rw, clock_config_t
        MBOARD_PROP_TIME_NOW,                //rw, time_spec_t
        MBOARD_PROP_TIME_PPS,                //wo, time_spec_t
        MBOARD_PROP_EEPROM_MAP,              //wr, mboard_eeprom_t
        MBOARD_PROP_IFACE,                   //ro, mboard_iface::sptr
    };

}} //namespace

#endif /* INCLUDED_UHD_USRP_MBOARD_PROPS_HPP */
