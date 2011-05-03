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

#ifndef INCLUDED_UHD_TYPES_TUNE_REQUEST_HPP
#define INCLUDED_UHD_TYPES_TUNE_REQUEST_HPP

#include <uhd/config.hpp>

namespace uhd{

    /*!
     * A tune request instructs the implementation how to tune the RF chain.
     * The policies can be used to select automatic tuning or
     * fined control over the daughterboard IF and DSP tuning.
     * Not all combinations of policies are applicable.
     * Convenience constructors are supplied for most use cases.
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
         */
        double dsp_freq;

    };

} //namespace uhd

#endif /* INCLUDED_UHD_TYPES_TUNE_REQUEST_HPP */
