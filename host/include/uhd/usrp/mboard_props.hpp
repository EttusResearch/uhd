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
        MBOARD_PROP_NAME,              //ro, std::string
        MBOARD_PROP_OTHERS,            //ro, prop_names_t
        MBOARD_PROP_CLOCK_RATE,        //ro, double
        MBOARD_PROP_RX_DSP,            //ro, wax::obj
        MBOARD_PROP_RX_DSP_NAMES,      //ro, prop_names_t
        MBOARD_PROP_TX_DSP,            //ro, wax::obj
        MBOARD_PROP_TX_DSP_NAMES,      //ro, prop_names_t
        MBOARD_PROP_RX_DBOARD,         //ro, wax::obj
        MBOARD_PROP_RX_DBOARD_NAMES,   //ro, prop_names_t
        MBOARD_PROP_TX_DBOARD,         //ro, wax::obj
        MBOARD_PROP_TX_DBOARD_NAMES,   //ro, prop_names_t
        MBOARD_PROP_CLOCK_CONFIG,      //rw, clock_config_t
        MBOARD_PROP_TIME_NOW,          //wo, time_spec_t
        MBOARD_PROP_TIME_NEXT_PPS      //wo, time_spec_t
    };

}} //namespace

#endif /* INCLUDED_UHD_USRP_MBOARD_PROPS_HPP */
