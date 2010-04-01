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

#ifndef INCLUDED_UHD_USRP_DSP_PROPS_HPP
#define INCLUDED_UHD_USRP_DSP_PROPS_HPP

#include <uhd/utils/props.hpp>

namespace uhd{ namespace usrp{

    /*!
     * Possible device dsp properties:
     *   A dsp can have a wide range of possible properties.
     *   A ddc would have a properties "decim", "freq", "taps"...
     *   Other properties could be gains, complex scalars, enables...
     *   For this reason the only required properties of a dsp is a name
     *   and a property to get list of other possible properties.
     */
    enum dsp_prop_t{
        DSP_PROP_NAME,                 //ro, std::string
        DSP_PROP_OTHERS                //ro, prop_names_t
    };

}} //namespace

#endif /* INCLUDED_UHD_USRP_DSP_PROPS_HPP */
