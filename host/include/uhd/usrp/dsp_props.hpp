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
     *   A dsp is a black box fpga component found between
     *   the over-the-wire data and the codec pins.
     *
     *   The host rate can be modified to control resampling.
     *   Resampling can take the form of decimation, interpolation,
     *   or more complex fractional resampling techniques.
     *   As usual, read back the host rate after setting it
     *   to get the actual rate that was set (implementation dependent).
     *
     *   A dsp can also shift the digital stream in frequency.
     *   Set the shift property and read it back to get actual shift.
     */
    enum dsp_prop_t{
        DSP_PROP_NAME         = 'n', //ro, std::string
        DSP_PROP_OTHERS       = 'o', //ro, prop_names_t
        DSP_PROP_FREQ_SHIFT   = 'f', //rw, double Hz
        DSP_PROP_CODEC_RATE   = 'c', //ro, double Sps
        DSP_PROP_HOST_RATE    = 'h'  //rw, double Sps
    };

}} //namespace

#endif /* INCLUDED_UHD_USRP_DSP_PROPS_HPP */
