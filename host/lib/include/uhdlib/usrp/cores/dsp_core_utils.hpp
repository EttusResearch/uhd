//
// Copyright 2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/exception.hpp>
#include <uhd/utils/math.hpp>
#include <uhdlib/usrp/cores/dsp_core_utils.hpp>
#include <cmath>
#include <limits>
#include <tuple>

/*! For a requested frequency, sampling rate, and frequency word width (in
 *  number of bits), return the correct frequency word (to set the CORDIC or
 *  DDS) and the actual frequency.
 */
template <int word_width = 32>
void get_freq_and_freq_word(const double requested_freq,
    const double tick_rate,
    double& actual_freq,
    int32_t& freq_word)
{
    constexpr int32_t MAX_FREQ_WORD = std::numeric_limits<int32_t>::max();
    constexpr int32_t MIN_FREQ_WORD = std::numeric_limits<int32_t>::min();
    constexpr double scale_factor   = static_cast<double>(uint64_t(1) << word_width);
    // Frequency normalized by sampling rate, and wrapped to [-0.5, 0.5).
    const double freq_norm_scaled =
        uhd::math::wrap_frequency(requested_freq / tick_rate, 1.0) * scale_factor;

    // confirm that the target frequency is within range of the CORDIC
    UHD_ASSERT_THROW(std::abs(freq_norm_scaled) - 1 <= MAX_FREQ_WORD);

    /* Now calculate the frequency word. It is possible for this calculation
     * to cause an overflow. As the requested DSP frequency approaches the
     * master clock rate, that ratio multiplied by the scaling factor (2^32)
     * will generally overflow within the last few kHz of tunable range.
     * Thus, we check to see if the operation will overflow before doing it,
     * and if it will, we set it to the integer min or max of this system.
     */
    if (freq_norm_scaled >= MAX_FREQ_WORD) {
        /* Operation would have caused a positive overflow of int32. */
        freq_word = MAX_FREQ_WORD;

    } else if (freq_norm_scaled <= MIN_FREQ_WORD) {
        /* Operation would have caused a negative overflow of int32. */
        freq_word = MIN_FREQ_WORD;

    } else {
        /* The operation is safe. Perform normally. */
        freq_word = int32_t(std::lround(freq_norm_scaled));
    }

    actual_freq = (double(freq_word) / scale_factor) * tick_rate;
}

/*! For a requested frequency, sampling rate, and frequency word width (in
 *  number of bits), return the correct frequency word (to set the CORDIC or
 *  DDS) and the actual frequency.
 */
template <int word_width = 32>
std::tuple<double, int> get_freq_and_freq_word(
    const double requested_freq, const double tick_rate)
{
    double actual_freq;
    int32_t freq_word;
    get_freq_and_freq_word<word_width>(requested_freq, tick_rate, actual_freq, freq_word);
    return {actual_freq, freq_word};
}
