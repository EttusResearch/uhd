//
// Copyright 2013-2014 Ettus Research LLC
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

#include "adf435x_common.hpp"

#include <boost/math/special_functions/round.hpp>
#include <uhd/types/tune_request.hpp>
#include <uhd/utils/log.hpp>
#include <cmath>


using namespace uhd;

/***********************************************************************
 * ADF 4350/4351 Tuning Utility
 **********************************************************************/
adf435x_tuning_settings tune_adf435x_synth(
    const double target_freq,
    const double ref_freq,
    const adf435x_tuning_constraints& constraints,
    double& actual_freq)
{
    //Default invalid value for actual_freq
    actual_freq = 0;

    double pfd_freq = 0;
    boost::uint16_t R = 0, BS = 0, N = 0, FRAC = 0, MOD = 0;
    boost::uint16_t RFdiv = static_cast<boost::uint16_t>(constraints.rf_divider_range.start());
    bool D = false, T = false;

    //Reference doubler for 50% duty cycle
    //If ref_freq < 12.5MHz enable the reference doubler
    D = (ref_freq <= constraints.ref_doubler_threshold);

    static const double MIN_VCO_FREQ = 2.2e9;
    static const double MAX_VCO_FREQ = 4.4e9;

    //increase RF divider until acceptable VCO frequency
    double vco_freq = target_freq;
    while (vco_freq < MIN_VCO_FREQ && RFdiv < static_cast<boost::uint16_t>(constraints.rf_divider_range.stop())) {
        vco_freq *= 2;
        RFdiv *= 2;
    }

    /*
     * The goal here is to loop though possible R dividers,
     * band select clock dividers, N (int) dividers, and FRAC
     * (frac) dividers.
     *
     * Calculate the N and F dividers for each set of values.
     * The loop exits when it meets all of the constraints.
     * The resulting loop values are loaded into the registers.
     *
     * from pg.21
     *
     * f_pfd = f_ref*(1+D)/(R*(1+T))
     * f_vco = (N + (FRAC/MOD))*f_pfd
     *    N = f_vco/f_pfd - FRAC/MOD = f_vco*((R*(T+1))/(f_ref*(1+D))) - FRAC/MOD
     * f_actual = f_vco/RFdiv)
     */
    double feedback_freq = constraints.feedback_after_divider ? target_freq : vco_freq;

    for(R = 1; R <= 1023; R+=1){
        //PFD input frequency = f_ref/R ... ignoring Reference doubler/divide-by-2 (D & T)
        pfd_freq = ref_freq*(D?2:1)/(R*(T?2:1));

        //keep the PFD frequency at or below 25MHz (Loop Filter Bandwidth)
        if (pfd_freq > constraints.pfd_freq_max) continue;

        //First, ignore fractional part of tuning
        N = boost::uint16_t(std::floor(feedback_freq/pfd_freq));

        //keep N > minimum int divider requirement
        if (N < static_cast<boost::uint16_t>(constraints.int_range.start())) continue;

        for(BS=1; BS <= 255; BS+=1){
            //keep the band select frequency at or below band_sel_freq_max
            //constraint on band select clock
            if (pfd_freq/BS > constraints.band_sel_freq_max) continue;
            goto done_loop;
        }
    } done_loop:

    //Fractional-N calculation
    MOD = 4095; //max fractional accuracy
    FRAC = static_cast<boost::uint16_t>(boost::math::round((feedback_freq/pfd_freq - N)*MOD));
    if (constraints.force_frac0) {
        if (FRAC > (MOD / 2)) { //Round integer such that actual freq is closest to target
            N++;
        }
        FRAC = 0;
    }

    //Reference divide-by-2 for 50% duty cycle
    // if R even, move one divide by 2 to to regs.reference_divide_by_2
    if(R % 2 == 0) {
        T = true;
        R /= 2;
    }

    //Typical phase resync time documented in data sheet pg.24
    static const double PHASE_RESYNC_TIME = 400e-6;

    //If feedback after divider, then compensation for the divider is pulled into the INT value
    int rf_div_compensation = constraints.feedback_after_divider ? 1 : RFdiv;

    //Compute the actual frequency in terms of ref_freq, N, FRAC, MOD, D, R and T.
    actual_freq = (
        double((N + (double(FRAC)/double(MOD))) *
        (ref_freq*(D?2:1)/(R*(T?2:1))))
    ) / rf_div_compensation;

    //load the settings
    adf435x_tuning_settings settings;
    settings.frac_12_bit = FRAC;
    settings.int_16_bit = N;
    settings.mod_12_bit = MOD;
    settings.clock_divider_12_bit = std::max<boost::uint16_t>(1, boost::uint16_t(std::ceil(PHASE_RESYNC_TIME*pfd_freq/MOD)));
    settings.r_counter_10_bit = R;
    settings.r_divide_by_2_en = T;
    settings.r_doubler_en = D;
    settings.band_select_clock_div = boost::uint8_t(BS);
    settings.rf_divider = RFdiv;

    std::string tuning_str = (constraints.force_frac0) ? "Integer-N" : "Fractional";
    UHD_LOGV(often)
        << boost::format("ADF 435X Frequencies (MHz): REQUESTED=%0.9f, ACTUAL=%0.9f"
        ) % (target_freq/1e6) % (actual_freq/1e6) << std::endl
        << boost::format("ADF 435X Intermediates (MHz): Feedback=%0.2f, VCO=%0.2f, PFD=%0.2f, BAND=%0.2f, REF=%0.2f"
        ) % (feedback_freq/1e6) % (vco_freq/1e6) % (pfd_freq/1e6) % (pfd_freq/BS/1e6) % (ref_freq/1e6) << std::endl
        << boost::format("ADF 435X Tuning: %s") % tuning_str.c_str() << std::endl
        << boost::format("ADF 435X Settings: R=%d, BS=%d, N=%d, FRAC=%d, MOD=%d, T=%d, D=%d, RFdiv=%d"
        ) % R % BS % N % FRAC % MOD % T % D % RFdiv << std::endl;

    UHD_ASSERT_THROW((settings.frac_12_bit          & ((boost::uint16_t)~0xFFF)) == 0);
    UHD_ASSERT_THROW((settings.mod_12_bit           & ((boost::uint16_t)~0xFFF)) == 0);
    UHD_ASSERT_THROW((settings.clock_divider_12_bit & ((boost::uint16_t)~0xFFF)) == 0);
    UHD_ASSERT_THROW((settings.r_counter_10_bit     & ((boost::uint16_t)~0x3FF)) == 0);

    UHD_ASSERT_THROW(vco_freq >= MIN_VCO_FREQ and vco_freq <= MAX_VCO_FREQ);
    UHD_ASSERT_THROW(settings.rf_divider >= static_cast<boost::uint16_t>(constraints.rf_divider_range.start()));
    UHD_ASSERT_THROW(settings.rf_divider <= static_cast<boost::uint16_t>(constraints.rf_divider_range.stop()));
    UHD_ASSERT_THROW(settings.int_16_bit >= static_cast<boost::uint16_t>(constraints.int_range.start()));
    UHD_ASSERT_THROW(settings.int_16_bit <= static_cast<boost::uint16_t>(constraints.int_range.stop()));

    return settings;
}
