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


#include "max2870_regs.hpp"
#include "db_sbx_common.hpp"


using namespace uhd;
using namespace uhd::usrp;
using namespace boost::assign;

/***********************************************************************
 * Structors
 **********************************************************************/
sbx_xcvr::cbx::cbx(sbx_xcvr *_self_sbx_xcvr) {
    //register the handle to our base CBX class
    self_base = _self_sbx_xcvr;
}


sbx_xcvr::cbx::~cbx(void){
    /* NOP */
}


/***********************************************************************
 * Tuning
 **********************************************************************/
double sbx_xcvr::cbx::set_lo_freq(dboard_iface::unit_t unit, double target_freq) {
    UHD_LOGV(often) << boost::format(
        "CBX tune: target frequency %f Mhz"
    ) % (target_freq/1e6) << std::endl;

    //clip the input
    target_freq = cbx_freq_range.clip(target_freq);

    //map mode setting to valid integer divider (N) values
    static const uhd::range_t int_n_mode_div_range(16,4095,1);
    static const uhd::range_t frac_n_mode_div_range(19,4091,1);

    //map rf divider select output dividers to enums
    static const uhd::dict<int, max2870_regs_t::rf_divider_select_t> rfdivsel_to_enum = map_list_of
        (1,   max2870_regs_t::RF_DIVIDER_SELECT_DIV1)
        (2,   max2870_regs_t::RF_DIVIDER_SELECT_DIV2)
        (4,   max2870_regs_t::RF_DIVIDER_SELECT_DIV4)
        (8,   max2870_regs_t::RF_DIVIDER_SELECT_DIV8)
        (16,  max2870_regs_t::RF_DIVIDER_SELECT_DIV16)
        (32,  max2870_regs_t::RF_DIVIDER_SELECT_DIV32)
        (64,  max2870_regs_t::RF_DIVIDER_SELECT_DIV64)
        (128, max2870_regs_t::RF_DIVIDER_SELECT_DIV128)
    ;
    
    double actual_freq, pfd_freq;
    double ref_freq = self_base->get_iface()->get_clock_rate(unit);
    max2870_regs_t::int_n_mode_t int_n_mode;
    int R=0, BS=0, N=0, FRAC=0, MOD=4095;
    int RFdiv = 1;
    max2870_regs_t::reference_divide_by_2_t T     = max2870_regs_t::REFERENCE_DIVIDE_BY_2_DISABLED;
    max2870_regs_t::reference_doubler_t     D     = max2870_regs_t::REFERENCE_DOUBLER_DISABLED;    

    //Reference doubler for 50% duty cycle
    // if ref_freq < 12.5MHz enable regs.reference_divide_by_2
    //NOTE: MAX2870 goes down to 10MHz ref vs. 12.5MHz on ADF4351
    if(ref_freq <= 10.0e6) D = max2870_regs_t::REFERENCE_DOUBLER_ENABLED;

    //increase RF divider until acceptable VCO frequency
    double vco_freq = target_freq;
    //NOTE: MIN freq for MAX2870 VCO is 3GHz vs. 2.2GHz on ADF4351
    while (vco_freq < 3e9) {
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
     *     N = f_vco/f_pfd - FRAC/MOD = f_vco*((R*(T+1))/(f_ref*(1+D))) - FRAC/MOD
     * f_rf  = f_vco/RFdiv
     */
    for(R = 1; R <= 1023; R+=1){
        //PFD input frequency = f_ref/R ... ignoring Reference doubler/divide-by-2 (D & T)
        pfd_freq = ref_freq*(1+D)/(R*(1+T));

        //keep the PFD frequency at or below 25MHz
        if (pfd_freq > 25e6) continue;

        //ignore fractional part of tuning
        N = int(vco_freq/pfd_freq);

        //Fractional-N calculation
        FRAC = int((vco_freq/pfd_freq - N)*MOD);

        //are we in int-N or frac-N mode?
        int_n_mode = (FRAC == 0) ? max2870_regs_t::INT_N_MODE_INT_N : max2870_regs_t::INT_N_MODE_FRAC_N;

        //keep N within int divider requirements
        if(int_n_mode == max2870_regs_t::INT_N_MODE_INT_N) {
            if(N < int_n_mode_div_range.start()) continue;
            if(N > int_n_mode_div_range.stop()) continue;
        } else {
            if(N < frac_n_mode_div_range.start()) continue;
            if(N > frac_n_mode_div_range.stop()) continue;
        }

        //keep pfd freq low enough to achieve 50kHz BS clock
        BS = std::ceil(pfd_freq / 50e3);
        if(BS <= 1023) break;
    }

    UHD_ASSERT_THROW(R <= 1023);

    //Reference divide-by-2 for 50% duty cycle
    // if R even, move one divide by 2 to to regs.reference_divide_by_2
    if(R % 2 == 0){
        T = max2870_regs_t::REFERENCE_DIVIDE_BY_2_ENABLED;
        R /= 2;
    }

    //actual frequency calculation
    actual_freq = double((N + (double(FRAC)/double(MOD)))*ref_freq*(1+int(D))/(R*(1+int(T)))/RFdiv);

    UHD_LOGV(often)
        << boost::format("CBX Intermediates: ref=%0.2f, outdiv=%f, fbdiv=%f") % (ref_freq*(1+int(D))/(R*(1+int(T)))) % double(RFdiv*2) % double(N + double(FRAC)/double(MOD)) << std::endl
        << boost::format("CBX tune: R=%d, BS=%d, N=%d, FRAC=%d, MOD=%d, T=%d, D=%d, RFdiv=%d"
            ) % R % BS % N % FRAC % MOD % T % D % RFdiv << std::endl
        << boost::format("CBX Frequencies (MHz): REQ=%0.2f, ACT=%0.2f, VCO=%0.2f, PFD=%0.2f, BAND=%0.2f"
            ) % (target_freq/1e6) % (actual_freq/1e6) % (vco_freq/1e6) % (pfd_freq/1e6) % (pfd_freq/BS/1e6) << std::endl;

    //load the register values
    max2870_regs_t regs;

    if ((unit == dboard_iface::UNIT_TX) and (actual_freq == sbx_tx_lo_2dbm.clip(actual_freq))) 
        regs.output_power = max2870_regs_t::OUTPUT_POWER_2DBM;
    else
        regs.output_power = max2870_regs_t::OUTPUT_POWER_5DBM;

    //set frac/int CPL mode
    max2870_regs_t::cpl_t cpl;
    max2870_regs_t::ldf_t ldf;
    max2870_regs_t::cpoc_t cpoc;
    if(int_n_mode == max2870_regs_t::INT_N_MODE_INT_N) {
        cpl = max2870_regs_t::CPL_DISABLED;
        cpoc = max2870_regs_t::CPOC_ENABLED;
        ldf = max2870_regs_t::LDF_INT_N;
    } else {
        cpl = max2870_regs_t::CPL_ENABLED;
        ldf = max2870_regs_t::LDF_FRAC_N;
        cpoc = max2870_regs_t::CPOC_DISABLED;
    }

    regs.frac_12_bit = FRAC;
    regs.int_16_bit = N;
    regs.mod_12_bit = MOD;
    regs.clock_divider_12_bit = std::max(1, int(std::ceil(400e-6*pfd_freq/MOD)));
    regs.feedback_select = (target_freq >= 3.0e9) ? max2870_regs_t::FEEDBACK_SELECT_DIVIDED : max2870_regs_t::FEEDBACK_SELECT_FUNDAMENTAL;
    regs.r_counter_10_bit = R;
    regs.reference_divide_by_2 = T;
    regs.reference_doubler = D;
    regs.band_select_clock_div = BS;
    UHD_ASSERT_THROW(rfdivsel_to_enum.has_key(RFdiv));
    regs.rf_divider_select = rfdivsel_to_enum[RFdiv];
    regs.int_n_mode = int_n_mode;
    regs.cpl = cpl;
    regs.ldf = ldf;
    regs.cpoc = cpoc;    

    //write the registers
    //correct power-up sequence to write registers (5, 4, 3, 2, 1, 0)
    int addr;

    for(addr=5; addr>=0; addr--){
        UHD_LOGV(often) << boost::format(
            "CBX SPI Reg (0x%02x): 0x%08x"
        ) % addr % regs.get_reg(addr) << std::endl;
        self_base->get_iface()->write_spi(
            unit, spi_config_t::EDGE_RISE,
            regs.get_reg(addr), 32
        );
    }

    //return the actual frequency
    UHD_LOGV(often) << boost::format(
        "CBX tune: actual frequency %f Mhz"
    ) % (actual_freq/1e6) << std::endl;
    return actual_freq;
}

