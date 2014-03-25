//
// Copyright 2014 Ettus Research LLC
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

#ifndef INCLUDED_ADF435X_COMMON_HPP
#define INCLUDED_ADF435X_COMMON_HPP

#include <boost/cstdint.hpp>
#include <uhd/property_tree.hpp>
#include <uhd/types/ranges.hpp>

//Common IO Pins
#define ADF435X_CE (1 << 3)
#define ADF435X_PDBRF (1 << 2)
#define ADF435X_MUXOUT (1 << 1) // INPUT!!!
#define LOCKDET_MASK (1 << 0) // INPUT!!!

#define RX_ATTN_SHIFT 8 //lsb of RX Attenuator Control
#define RX_ATTN_MASK (63 << RX_ATTN_SHIFT) //valid bits of RX Attenuator Control

struct adf435x_tuning_constraints {
    bool            force_frac0;
    bool            feedback_after_divider;
    double          ref_doubler_threshold;
    double          pfd_freq_max;
    double          band_sel_freq_max;
    uhd::range_t    rf_divider_range;
    uhd::range_t    int_range;
};

struct adf435x_tuning_settings {
    boost::uint16_t frac_12_bit;
    boost::uint16_t int_16_bit;
    boost::uint16_t mod_12_bit;
    boost::uint16_t r_counter_10_bit;
    bool            r_doubler_en;
    bool            r_divide_by_2_en;
    boost::uint16_t clock_divider_12_bit;
    boost::uint8_t  band_select_clock_div;
    boost::uint16_t rf_divider;
};

adf435x_tuning_settings tune_adf435x_synth(
    const double target_freq,
    const double ref_freq,
    const adf435x_tuning_constraints& constraints,
    double& actual_freq
);

#endif /* INCLUDED_ADF435X_COMMON_HPP */
