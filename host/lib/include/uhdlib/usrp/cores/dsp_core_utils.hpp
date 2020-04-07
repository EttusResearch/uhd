//
// Copyright 2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <tuple>

/*! For a requested frequency and sampling rate, return the
 *  correct frequency word (to set the CORDIC) and the actual frequency.
 */
void get_freq_and_freq_word(const double requested_freq,
    const double tick_rate,
    double& actual_freq,
    int32_t& freq_word);

/*! For a requested frequency and sampling rate, return the
 *  correct frequency word (to set the CORDIC) and the actual frequency.
 */
std::tuple<double, int> get_freq_and_freq_word(
    const double requested_freq, const double tick_rate);
