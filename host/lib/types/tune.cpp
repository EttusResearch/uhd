//
// Copyright 2011 Ettus Research LLC
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

#include <uhd/types/tune_request.hpp>
#include <uhd/types/tune_result.hpp>
#include <boost/format.hpp>

using namespace uhd;

tune_request_t::tune_request_t(double target_freq):
    target_freq(target_freq),
    rf_freq_policy(POLICY_AUTO),
    rf_freq(0.0),
    dsp_freq_policy(POLICY_AUTO),
    dsp_freq(0.0)
{
    /* NOP */
}

tune_request_t::tune_request_t(double target_freq, double lo_off):
    target_freq(target_freq),
    rf_freq_policy(POLICY_MANUAL),
    rf_freq(target_freq + lo_off),
    dsp_freq_policy(POLICY_AUTO),
    dsp_freq(0.0)
{
    /* NOP */
}

std::string tune_result_t::to_pp_string(void) const{
    return str(boost::format(
        "Tune Result:\n"
        "    Target RF  Freq: %f (MHz)\n"
        "    Actual RF  Freq: %f (MHz)\n"
        "    Target DSP Freq: %f (MHz)\n"
        "    Actual DSP Freq: %f (MHz)\n"
    )
        % (target_rf_freq/1e6)  % (actual_rf_freq/1e6)
        % (target_dsp_freq/1e6) % (actual_dsp_freq/1e6)
    );
}
