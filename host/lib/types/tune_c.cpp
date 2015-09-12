//
// Copyright 2015 Ettus Research LLC
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

#include <uhd/types/tune_request.h>
#include <uhd/types/tune_result.h>

#include <boost/format.hpp>

#include <cstdlib>
#include <cstring>
#include <iostream>

/*
 * Tune request
 */

uhd::tune_request_t uhd_tune_request_c_to_cpp(uhd_tune_request_t *tune_request_c){
    uhd::tune_request_t tune_request_cpp;

    tune_request_cpp.target_freq = tune_request_c->target_freq;
    tune_request_cpp.rf_freq_policy = uhd::tune_request_t::policy_t(tune_request_c->rf_freq_policy);
    tune_request_cpp.rf_freq = tune_request_c->rf_freq;
    tune_request_cpp.dsp_freq_policy = uhd::tune_request_t::policy_t(tune_request_c->dsp_freq_policy);
    tune_request_cpp.dsp_freq = tune_request_c->dsp_freq;

    std::string args_cpp = (tune_request_c->args) ? tune_request_c->args : std::string("");
    tune_request_cpp.args = uhd::device_addr_t(args_cpp);

    return tune_request_cpp;
}

/*
 * Tune result
 */

void uhd_tune_result_to_pp_string(uhd_tune_result_t *tune_result_c,
                                  char* pp_string_out, size_t strbuffer_len){
    std::string pp_string_cpp = uhd_tune_result_c_to_cpp(tune_result_c).to_pp_string();
    memset(pp_string_out, '\0', strbuffer_len);
    strncpy(pp_string_out, pp_string_cpp.c_str(), strbuffer_len);
}

uhd::tune_result_t uhd_tune_result_c_to_cpp(uhd_tune_result_t *tune_result_c){
    uhd::tune_result_t tune_result_cpp;

    tune_result_cpp.clipped_rf_freq = tune_result_c->clipped_rf_freq;
    tune_result_cpp.target_rf_freq = tune_result_c->target_rf_freq;
    tune_result_cpp.actual_rf_freq = tune_result_c->actual_rf_freq;
    tune_result_cpp.target_dsp_freq = tune_result_c->target_dsp_freq;
    tune_result_cpp.actual_dsp_freq = tune_result_c->actual_dsp_freq;

    return tune_result_cpp;
}

void uhd_tune_result_cpp_to_c(const uhd::tune_result_t &tune_result_cpp,
                              uhd_tune_result_t *tune_result_c){
    tune_result_c->clipped_rf_freq = tune_result_cpp.clipped_rf_freq;
    tune_result_c->target_rf_freq = tune_result_cpp.target_rf_freq;
    tune_result_c->actual_rf_freq = tune_result_cpp.actual_rf_freq;
    tune_result_c->target_dsp_freq = tune_result_cpp.target_dsp_freq;
    tune_result_c->actual_dsp_freq = tune_result_cpp.actual_dsp_freq;
}
