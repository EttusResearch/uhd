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

#ifndef INCLUDED_UHD_USRP_CODEC_PROPS_HPP
#define INCLUDED_UHD_USRP_CODEC_PROPS_HPP

#include <uhd/utils/props.hpp>

namespace uhd{ namespace usrp{

    /*!
    * Possible device codec properties:
    *   A codec is expected to have a rate and gain elements.
    *   Other properties can be discovered through the others prop.
    */
    enum codec_prop_t{
        CODEC_PROP_NAME           = 'n', //ro, std::string
        CODEC_PROP_OTHERS         = 'o', //ro, prop_names_t
        CODEC_PROP_GAIN_I         = 'i', //rw, double
        CODEC_PROP_GAIN_Q         = 'q', //rw, double
        CODEC_PROP_GAIN_RANGE     = 'r', //ro, gain_range_t
        CODEC_PROP_GAIN_NAMES     = 'G'  //ro, prop_names_t
    };


}} //namespace

#endif /* INCLUDED_UHD_USRP_CODEC_PROPS_HPP */
