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

#ifndef INCLUDED_UHD_USRP_SUBDEV_PROPS_HPP
#define INCLUDED_UHD_USRP_SUBDEV_PROPS_HPP

#include <uhd/utils/props.hpp>

namespace uhd{ namespace usrp{

    /*!
     * Possible device subdev properties
     */
    enum subdev_prop_t{
        SUBDEV_PROP_NAME,              //ro, std::string
        SUBDEV_PROP_OTHERS,            //ro, prop_names_t
        SUBDEV_PROP_GAIN,              //rw, float
        SUBDEV_PROP_GAIN_RANGE,        //ro, gain_range_t
        SUBDEV_PROP_GAIN_NAMES,        //ro, prop_names_t
        SUBDEV_PROP_FREQ,              //rw, double
        SUBDEV_PROP_FREQ_RANGE,        //ro, freq_range_t
        SUBDEV_PROP_ANTENNA,           //rw, std::string
        SUBDEV_PROP_ANTENNA_NAMES,     //ro, prop_names_t
        SUBDEV_PROP_ENABLED,           //rw, bool
        SUBDEV_PROP_QUADRATURE,        //ro, bool
        SUBDEV_PROP_IQ_SWAPPED,        //ro, bool
        SUBDEV_PROP_SPECTRUM_INVERTED, //ro, bool
        SUBDEV_PROP_LO_INTERFERES      //ro, bool
        //SUBDEV_PROP_RSSI,              //ro, float //----> not on all boards, use named prop
        //SUBDEV_PROP_BANDWIDTH          //rw, double //----> not on all boards, use named prop
    };

}} //namespace

#endif /* INCLUDED_UHD_USRP_SUBDEV_PROPS_HPP */
