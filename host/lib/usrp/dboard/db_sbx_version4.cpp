//
// Copyright 2011-2012 Ettus Research LLC
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


#include "adf4351_regs.hpp"
#include "db_sbx_common.hpp"


using namespace uhd;
using namespace uhd::usrp;
using namespace boost::assign;

/***********************************************************************
 * Structors
 **********************************************************************/
sbx_xcvr::sbx_version4::sbx_version4(sbx_xcvr *_self_sbx_xcvr) {
    //register the handle to our base SBX class
    self_base = _self_sbx_xcvr;
}


sbx_xcvr::sbx_version4::~sbx_version4(void){
    /* NOP */
}


/***********************************************************************
 * Tuning
 **********************************************************************/
double sbx_xcvr::sbx_version4::set_lo_freq(dboard_iface::unit_t unit, double target_freq) {
    UHD_LOGV(often) << boost::format(
        "SBX tune: target frequency %f Mhz"
    ) % (target_freq/1e6) << std::endl;

    //clip the input
    target_freq = sbx_freq_range.clip(target_freq);

    //map prescaler setting to mininmum integer divider (N) values (pg.18 prescaler)
    static const uhd::dict<int, int> prescaler_to_min_int_div = map_list_of
        (0,23) //adf4351_regs_t::PRESCALER_4_5
        (1,75) //adf4351_regs_t::PRESCALER_8_9
    ;

    //map rf divider select output dividers to enums
    static const uhd::dict<int, adf4351_regs_t::rf_divider_select_t> rfdivsel_to_enum = map_list_of
        (1,  adf4351_regs_t::RF_DIVIDER_SELECT_DIV1)
        (2,  adf4351_regs_t::RF_DIVIDER_SELECT_DIV2)
        (4,  adf4351_regs_t::RF_DIVIDER_SELECT_DIV4)
        (8,  adf4351_regs_t::RF_DIVIDER_SELECT_DIV8)
        (16, adf4351_regs_t::RF_DIVIDER_SELECT_DIV16)
        (32, adf4351_regs_t::RF_DIVIDER_SELECT_DIV32)
        (64, adf4351_regs_t::RF_DIVIDER_SELECT_DIV64)
    ;

    double actual_freq, pfd_freq;
    double ref_freq = self_base->get_iface()->get_clock_rate(unit);
    int R=0, BS=0, N=0, FRAC=0, MOD=0;
    int RFdiv = 1;
    adf4351_regs_t::reference_divide_by_2_t T     = adf4351_regs_t::REFERENCE_DIVIDE_BY_2_DISABLED;
    adf4351_regs_t::reference_doubler_t     D     = adf4351_regs_t::REFERENCE_DOUBLER_DISABLED;    

    //Reference doubler for 50% duty cycle
    // if ref_freq < 12.5MHz enable regs.reference_divide_by_2
    if(ref_freq <= 12.5e6) D = adf4351_regs_t::REFERENCE_DOUBLER_ENABLED;

    //increase RF divider until acceptable VCO frequency
    double vco_freq = target_freq;
    while (vco_freq < 2.2e9) {
        vco_freq *= 2;
        RFdiv *= 2;
    }

    //use 8/9 prescaler for vco_freq > 3 GHz (pg.18 prescaler)
    adf4351_regs_t::prescaler_t prescaler = target_freq > 3e9 ? adf4351_regs_t::PRESCALER_8_9 : adf4351_regs_t::PRESCALER_4_5;

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
     * f_rf = f_vco/RFdiv)
     * f_actual = f_rf/2
     */
    for(R = 1; R <= 1023; R+=1){
        //PFD input frequency = f_ref/R ... ignoring Reference doubler/divide-by-2 (D & T)
        pfd_freq = ref_freq*(1+D)/(R*(1+T));

        //keep the PFD frequency at or below 25MHz (Loop Filter Bandwidth)
        if (pfd_freq > 25e6) continue;

        //ignore fractional part of tuning
        N = int(std::floor(vco_freq/pfd_freq));

        //keep N > minimum int divider requirement
        if (N < prescaler_to_min_int_div[prescaler]) continue;

        for(BS=1; BS <= 255; BS+=1){
            //keep the band select frequency at or below 100KHz
            //constraint on band select clock
            if (pfd_freq/BS > 100e3) continue;
            goto done_loop;
        }
    } done_loop:

    //Fractional-N calculation
    MOD = 4095; //max fractional accuracy
    FRAC = int((target_freq/pfd_freq - N)*MOD);

    //Reference divide-by-2 for 50% duty cycle
    // if R even, move one divide by 2 to to regs.reference_divide_by_2
    if(R % 2 == 0){
        T = adf4351_regs_t::REFERENCE_DIVIDE_BY_2_ENABLED;
        R /= 2;
    }

    //actual frequency calculation
    actual_freq = double((N + (double(FRAC)/double(MOD)))*ref_freq*(1+int(D))/(R*(1+int(T))));

    UHD_LOGV(often)
        << boost::format("SBX Intermediates: ref=%0.2f, outdiv=%f, fbdiv=%f") % (ref_freq*(1+int(D))/(R*(1+int(T)))) % double(RFdiv*2) % double(N + double(FRAC)/double(MOD)) << std::endl
        << boost::format("SBX tune: R=%d, BS=%d, N=%d, FRAC=%d, MOD=%d, T=%d, D=%d, RFdiv=%d"
            ) % R % BS % N % FRAC % MOD % T % D % RFdiv << std::endl
        << boost::format("SBX Frequencies (MHz): REQ=%0.2f, ACT=%0.2f, VCO=%0.2f, PFD=%0.2f, BAND=%0.2f"
            ) % (target_freq/1e6) % (actual_freq/1e6) % (vco_freq/1e6) % (pfd_freq/1e6) % (pfd_freq/BS/1e6) << std::endl;

    //load the register values
    adf4351_regs_t regs;

    if ((unit == dboard_iface::UNIT_TX) and (actual_freq == sbx_tx_lo_2dbm.clip(actual_freq))) 
        regs.output_power = adf4351_regs_t::OUTPUT_POWER_2DBM;
    else
        regs.output_power = adf4351_regs_t::OUTPUT_POWER_5DBM;

    regs.frac_12_bit = FRAC;
    regs.int_16_bit = N;
    regs.mod_12_bit = MOD;
    regs.clock_divider_12_bit = std::max(1, int(std::ceil(400e-6*pfd_freq/MOD)));
    regs.feedback_select = adf4351_regs_t::FEEDBACK_SELECT_DIVIDED;
    regs.clock_div_mode = adf4351_regs_t::CLOCK_DIV_MODE_RESYNC_ENABLE;
    regs.prescaler = prescaler;
    regs.r_counter_10_bit = R;
    regs.reference_divide_by_2 = T;
    regs.reference_doubler = D;
    regs.band_select_clock_div = BS;
    UHD_ASSERT_THROW(rfdivsel_to_enum.has_key(RFdiv));
    regs.rf_divider_select = rfdivsel_to_enum[RFdiv];

    //reset the N and R counter
    regs.counter_reset = adf4351_regs_t::COUNTER_RESET_ENABLED;
    self_base->get_iface()->write_spi(unit, spi_config_t::EDGE_RISE, regs.get_reg(2), 32);
    regs.counter_reset = adf4351_regs_t::COUNTER_RESET_DISABLED;

    //write the registers
    //correct power-up sequence to write registers (5, 4, 3, 2, 1, 0)
    int addr;

    for(addr=5; addr>=0; addr--){
        UHD_LOGV(often) << boost::format(
            "SBX SPI Reg (0x%02x): 0x%08x"
        ) % addr % regs.get_reg(addr) << std::endl;
        self_base->get_iface()->write_spi(
            unit, spi_config_t::EDGE_RISE,
            regs.get_reg(addr), 32
        );
    }

    //return the actual frequency
    UHD_LOGV(often) << boost::format(
        "SBX tune: actual frequency %f Mhz"
    ) % (actual_freq/1e6) << std::endl;
    return actual_freq;
}

