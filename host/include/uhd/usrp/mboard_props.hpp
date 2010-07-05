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
        MBOARD_PROP_NAME            = 'n', //ro, std::string
        MBOARD_PROP_OTHERS          = 'o', //ro, prop_names_t
        MBOARD_PROP_RX_DSP          = 'd', //ro, wax::obj
        MBOARD_PROP_RX_DSP_NAMES    = 'D', //ro, prop_names_t
        MBOARD_PROP_TX_DSP          = 'u', //ro, wax::obj
        MBOARD_PROP_TX_DSP_NAMES    = 'U', //ro, prop_names_t
        MBOARD_PROP_RX_DBOARD       = 'e', //ro, wax::obj
        MBOARD_PROP_RX_DBOARD_NAMES = 'E', //ro, prop_names_t
        MBOARD_PROP_TX_DBOARD       = 'v', //ro, wax::obj
        MBOARD_PROP_TX_DBOARD_NAMES = 'V', //ro, prop_names_t
        MBOARD_PROP_CLOCK_CONFIG    = 'C', //rw, clock_config_t
        MBOARD_PROP_TIME_NOW        = 't', //rw, time_spec_t
        MBOARD_PROP_TIME_NEXT_PPS   = 'T', //wo, time_spec_t
        MBOARD_PROP_STREAM_CMD      = 's'  //wo, stream_cmd_t
    };

}} //namespace

#endif /* INCLUDED_UHD_USRP_MBOARD_PROPS_HPP */
