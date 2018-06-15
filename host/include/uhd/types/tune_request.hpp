//
// Copyright 2010-2012 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_TYPES_TUNE_REQUEST_HPP
#define INCLUDED_UHD_TYPES_TUNE_REQUEST_HPP

#include <uhd/config.hpp>
#include <uhd/types/device_addr.hpp>

namespace uhd{

    /*!
     * A tune request instructs the implementation how to tune the RF chain.
     * The policies can be used to select automatic tuning or
     * fined control over the daughterboard IF and DSP tuning.
     * Not all combinations of policies are applicable.
     * Convenience constructors are supplied for most use cases.
     *
     * See also \ref general_tuning
     */
    struct UHD_API tune_request_t{
        /*!
         * Make a new tune request for a particular center frequency.
         * Use an automatic policy for the RF and DSP frequency
         * to tune the chain as close as possible to the target frequency.
         * \param target_freq the target frequency in Hz
         */
        tune_request_t(double target_freq = 0);

        /*!
         * Make a new tune request for a particular center frequency.
         * Use a manual policy for the RF frequency,
         * and an automatic policy for the DSP frequency,
         * to tune the chain as close as possible to the target frequency.
         * \param target_freq the target frequency in Hz
         * \param lo_off the LO offset frequency in Hz
         */
        tune_request_t(double target_freq, double lo_off);

        //! Policy options for tunable elements in the RF chain.
        enum policy_t {
            //! Do not set this argument, use current setting.
            POLICY_NONE   = int('N'),
            //! Automatically determine the argument's value.
            POLICY_AUTO   = int('A'),
            //! Use the argument's value for the setting.
            POLICY_MANUAL = int('M')
        };

        /*!
         * The target frequency of the overall chain in Hz.
         * Set this even if all policies are set to manual.
         */
        double target_freq;

        /*!
         * The policy for the RF frequency.
         * Automatic behavior: the target frequency + default LO offset.
         */
        policy_t rf_freq_policy;

        /*!
         * The RF frequency in Hz.
         * Set when the policy is set to manual.
         */
        double rf_freq;

        /*!
         * The policy for the DSP frequency.
         * Automatic behavior: the difference between the target and IF.
         */
        policy_t dsp_freq_policy;

        /*!
         * The DSP frequency in Hz.
         * Set when the policy is set to manual.
         *
         * Note that the meaning of the DSP frequency's sign differs between
         * TX and RX operations. The target frequency is the result of
         * `target_freq = rf_freq + sign * dsp_freq`. For TX, `sign` is
         * negative, and for RX, `sign` is positive.
         * Example: If both RF and DSP tuning policies are set to manual, and
         * `rf_freq` is set to 1 GHz, and `dsp_freq` is set to 10 MHz, the
         * actual target frequency is 990 MHz for a TX tune request, and
         * 1010 MHz for an RX tune request.
         */
        double dsp_freq;

        /*!
         * The args parameter is used to pass arbitrary key/value pairs.
         * Possible keys used by args (depends on implementation):
         *
         * - mode_n: Allows the user to tell the daughterboard tune code
         * to choose between an integer N divider or fractional N divider.
         * Default is fractional N on boards that support fractional N tuning.
         * Fractional N provides greater tuning accuracy at the expense of spurs.
         * Possible options for this key: "integer" or "fractional".
         */
        device_addr_t args;

    };

} //namespace uhd

#endif /* INCLUDED_UHD_TYPES_TUNE_REQUEST_HPP */
