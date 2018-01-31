//
// Copyright 2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
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
