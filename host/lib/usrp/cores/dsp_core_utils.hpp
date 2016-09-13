//
// Copyright 2016 Ettus Research LLC
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

#ifndef INCLUDED_LIBUHD_DSP_CORE_UTILS_HPP
#define INCLUDED_LIBUHD_DSP_CORE_UTILS_HPP

#include <stdint.h>

/*! For a requested frequency and sampling rate, return the
 *  correct frequency word (to set the CORDIC) and the actual frequency.
 */
void get_freq_and_freq_word(
        const double requested_freq,
        const double tick_rate,
        double &actual_freq,
        int32_t &freq_word
);

#endif /* INCLUDED_LIBUHD_DSP_CORE_UTILS_HPP */
