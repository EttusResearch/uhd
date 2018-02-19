//
// Copyright 2010 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_TYPES_TUNE_RESULT_HPP
#define INCLUDED_UHD_TYPES_TUNE_RESULT_HPP

#include <uhd/config.hpp>
#include <string>

namespace uhd{

    /*!
     * The tune result struct holds the RF and DSP tuned frequencies.
     */
    struct UHD_API tune_result_t{
        /*! The target RF frequency, clipped to be within system range
         *
         * If the requested frequency is within the range of the system, then
         * this variable will equal the requested frequency. If the requested
         * frequency is outside of the tunable range, however, this variable
         * will hold the value that it was 'clipped' to in order to keep tuning
         * in-bounds. */
        double clipped_rf_freq;

        /*! Target RF Freq, including RF FE offset
         *
         * AUTO Tuning Policy:
         * This variable holds the requested center frequency, plus any LO
         * offset required by the radio front-end. Note that this is *not* the
         * LO offset requested by the user (if one exists), but rather one
         * required by the hardware (if required).
         *
         * MANUAL Tuning Policy:
         * This variable equals the RF frequency in the tune request. */
        double target_rf_freq;

        /*! The frequency to which the RF LO actually tuned
         *
         * If this does not equal the `target_rf_freq`, then it is because the
         * target was outside of the range of the LO, or the LO was not able to
         * hit it exactly due to tuning accuracy. */
        double actual_rf_freq;

        /*! The frequency the CORDIC must adjust the RF
         *
         * AUTO Tuning Policy:
         * It is fairly common for the RF LO to not be able to exactly hit the
         * requested frequency. This variable holds the required adjustment the
         * CORDIC must make to the signal to bring it to the requested center
         * frequency.
         *
         * MANUAL Tuning Policy
         * This variable equals the DSP frequency in the tune request, clipped
         * to be within range of the DSP if it was outside. */
        double target_dsp_freq;

        /*! The frequency to which the CORDIC in the DSP actually tuned
         *
         * If we failed to hit the target DSP frequency, it is either because
         * the requested resolution wasn't possible or something went wrong in
         * the DSP. In most cases, it should equal the `target_dsp_freq` above.
         */
        double actual_dsp_freq;

        /*!
         * Create a pretty print string for this tune result struct.
         * \return the printable string
         */
        std::string to_pp_string(void) const;
    };

} //namespace uhd

#endif /* INCLUDED_UHD_TYPES_TUNE_RESULT_HPP */
