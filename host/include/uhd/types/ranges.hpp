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

#ifndef INCLUDED_UHD_TYPES_RANGES_HPP
#define INCLUDED_UHD_TYPES_RANGES_HPP

#include <uhd/config.hpp>

namespace uhd{

    /*!
     * The gain range struct describes possible gain settings.
     * The mimumum gain, maximum gain, and step size are in dB.
     */
    struct UHD_API gain_range_t{
        float min, max, step;
        gain_range_t(float min = 0.0, float max = 0.0, float step = 0.0);
    };

    /*!
     * The frequency range struct describes possible frequency settings.
     * Because tuning is very granular (sub-Hz), step size is not listed.
     * The mimumum frequency and maximum frequency are in Hz.
     */
    struct UHD_API freq_range_t{
        double min, max;
        freq_range_t(double min = 0.0, double max = 0.0);
    };

} //namespace uhd

#endif /* INCLUDED_UHD_TYPES_RANGES_HPP */
