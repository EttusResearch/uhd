//
// Copyright 2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/utils/math.hpp>
#include <uhdlib/usrp/cores/dsp_core_utils.hpp>
#include <cmath>
#include <limits>

static const int32_t MAX_FREQ_WORD = std::numeric_limits<int32_t>::max();
static const int32_t MIN_FREQ_WORD = std::numeric_limits<int32_t>::min();

void get_freq_and_freq_word(const double requested_freq,
    const double tick_rate,
    double& actual_freq,
    int32_t& freq_word,
    int word_width)
{
    const double freq = uhd::math::wrap_frequency(requested_freq, tick_rate);

    // confirm that the target frequency is within range of the CORDIC
    UHD_ASSERT_THROW(std::abs(freq) <= tick_rate / 2.0);

    /* Now calculate the frequency word. It is possible for this calculation
     * to cause an overflow. As the requested DSP frequency approaches the
     * master clock rate, that ratio multiplied by the scaling factor (2^32)
     * will generally overflow within the last few kHz of tunable range.
     * Thus, we check to see if the operation will overflow before doing it,
     * and if it will, we set it to the integer min or max of this system.
     */
    freq_word = 0;

    static const double scale_factor = std::pow(2.0, word_width);
    if ((freq / tick_rate) >= (MAX_FREQ_WORD / scale_factor)) {
        /* Operation would have caused a positive overflow of int32. */
        freq_word = MAX_FREQ_WORD;

    } else if ((freq / tick_rate) <= (MIN_FREQ_WORD / scale_factor)) {
        /* Operation would have caused a negative overflow of int32. */
        freq_word = MIN_FREQ_WORD;

    } else {
        /* The operation is safe. Perform normally. */
        freq_word = int32_t(std::lround((freq / tick_rate) * scale_factor));
    }

    actual_freq = (double(freq_word) / scale_factor) * tick_rate;
}

std::tuple<double, int> get_freq_and_freq_word(
    const double requested_freq, const double tick_rate, int word_width)
{
    double actual_freq;
    int32_t freq_word;
    get_freq_and_freq_word(requested_freq, tick_rate, actual_freq, freq_word, word_width);
    return std::make_tuple(actual_freq, freq_word);
}
