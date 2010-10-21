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

#ifndef INCLUDED_LIBUHD_USRP_WRAPPER_UTILS_HPP
#define INCLUDED_LIBUHD_USRP_WRAPPER_UTILS_HPP

#include <uhd/wax.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/usrp/dsp_props.hpp>
#include <uhd/utils/warning.hpp>
#include <boost/format.hpp>
#include <cmath>

static inline uhd::freq_range_t add_dsp_shift(
    const uhd::freq_range_t &range,
    wax::obj dsp
){
    double codec_rate = dsp[uhd::usrp::DSP_PROP_CODEC_RATE].as<double>();
    return uhd::freq_range_t(range.min - codec_rate/2.0, range.max + codec_rate/2.0);
}

static inline void do_samp_rate_warning_message(
    double target_rate,
    double actual_rate,
    const std::string &xx
){
    static const double max_allowed_error = 1.0; //Sps
    if (std::abs(target_rate - actual_rate) > max_allowed_error){
        uhd::print_warning(str(boost::format(
            "The hardware does not support the requested %s sample rate:\n"
            "Target sample rate: %f MSps\n"
            "Actual sample rate: %f MSps\n"
        ) % xx % (target_rate/1e6) % (actual_rate/1e6)));
    }
}

static inline void do_tune_freq_warning_message(
    double target_freq,
    double actual_freq,
    const std::string &xx
){
    static const double max_allowed_error = 1.0; //Hz
    if (std::abs(target_freq - actual_freq) > max_allowed_error){
        uhd::print_warning(str(boost::format(
            "The hardware does not support the requested %s frequency:\n"
            "Target frequency: %f MHz\n"
            "Actual frequency: %f MHz\n"
        ) % xx % (target_freq/1e6) % (actual_freq/1e6)));
    }
}

#endif /* INCLUDED_LIBUHD_USRP_WRAPPER_UTILS_HPP */
