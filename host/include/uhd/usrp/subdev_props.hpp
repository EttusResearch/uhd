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

#ifndef INCLUDED_UHD_USRP_SUBDEV_PROPS_HPP
#define INCLUDED_UHD_USRP_SUBDEV_PROPS_HPP

#include <uhd/utils/props.hpp>

namespace uhd{ namespace usrp{

    /*!
     * Possible subdev connection types:
     *
     * A complex subdevice is physically connected to both channels,
     * which may be connected in one of two ways: IQ or QI (swapped).
     *
     * A real subdevice is only physically connected one channel,
     * either only the I channel or only the Q channel.
     */
    enum subdev_conn_t{
        SUBDEV_CONN_COMPLEX_IQ = 'C',
        SUBDEV_CONN_COMPLEX_QI = 'c',
        SUBDEV_CONN_REAL_I     = 'R',
        SUBDEV_CONN_REAL_Q     = 'r'
    };

    /*!
     * Possible device subdev properties
     */
    enum subdev_prop_t{
        SUBDEV_PROP_NAME,               //ro, std::string
        SUBDEV_PROP_OTHERS,             //ro, prop_names_t
        SUBDEV_PROP_SENSOR,             //ro, sensor_value_t
        SUBDEV_PROP_SENSOR_NAMES,       //ro, prop_names_t
        SUBDEV_PROP_GAIN,               //rw, double
        SUBDEV_PROP_GAIN_RANGE,         //ro, gain_range_t
        SUBDEV_PROP_GAIN_NAMES,         //ro, prop_names_t
        SUBDEV_PROP_FREQ,               //rw, double
        SUBDEV_PROP_FREQ_RANGE,         //ro, freq_range_t
        SUBDEV_PROP_ANTENNA,            //rw, std::string
        SUBDEV_PROP_ANTENNA_NAMES,      //ro, prop_names_t
        SUBDEV_PROP_CONNECTION,         //ro, subdev_conn_t
        SUBDEV_PROP_ENABLED,            //rw, bool
        SUBDEV_PROP_USE_LO_OFFSET,      //ro, bool
        SUBDEV_PROP_BANDWIDTH           //rw, double
    };

}} //namespace

#endif /* INCLUDED_UHD_USRP_SUBDEV_PROPS_HPP */
