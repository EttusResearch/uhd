//
// Copyright 2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <tuple>

/*! For a requested frequency, sampling rate, and frequency word width (in
 *  number of bits), return the correct frequency word (to set the CORDIC or
 *  DDS) and the actual frequency.
 */
void get_freq_and_freq_word(const double requested_freq,
    const double tick_rate,
    double& actual_freq,
    int32_t& freq_word,
    int word_width = 32);

/*! For a requested frequency, sampling rate, and frequency word width (in
 *  number of bits), return the correct frequency word (to set the CORDIC or
 *  DDS) and the actual frequency.
 */
std::tuple<double, int> get_freq_and_freq_word(
    const double requested_freq, const double tick_rate, int word_width = 32);
