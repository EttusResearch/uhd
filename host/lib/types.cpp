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

#include <uhd/types.hpp>

using namespace uhd;

/***********************************************************************
 * gain range
 **********************************************************************/
gain_range_t::gain_range_t(float min_, float max_, float step_){
    min = min_;
    max = max_;
    step = step_;
}

/***********************************************************************
 * freq range
 **********************************************************************/
freq_range_t::freq_range_t(double min_, double max_){
    min = min_;
    max = max_;
}

/***********************************************************************
 * tune result
 **********************************************************************/
tune_result_t::tune_result_t(void){
    target_inter_freq = 0.0;
    actual_inter_freq = 0.0;
    target_dxc_freq = 0.0;
    actual_dxc_freq = 0.0;
    spectrum_inverted = false;
}

/***********************************************************************
 * clock config
 **********************************************************************/
clock_config_t::clock_config_t(void){
    ref_source = REF_INT,
    pps_source = PPS_INT,
    pps_polarity = PPS_NEG;
}

/***********************************************************************
 * stream command
 **********************************************************************/
stream_cmd_t::stream_cmd_t(void){
    stream_now = true;
    continuous = false;
    num_samps = 0;
}
