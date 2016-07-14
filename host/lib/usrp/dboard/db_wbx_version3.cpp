//
// Copyright 2011-2014 Ettus Research LLC
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

#include "db_wbx_common.hpp"
#include "adf4350_regs.hpp"
#include "../common/adf435x_common.hpp"
#include <uhd/utils/log.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/types/sensors.hpp>
#include <uhd/utils/assert_has.hpp>
#include <uhd/utils/algorithm.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/usrp/dboard_base.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/format.hpp>
#include <boost/math/special_functions/round.hpp>
#include <boost/algorithm/string.hpp>

using namespace uhd;
using namespace uhd::usrp;
using namespace boost::assign;


/***********************************************************************
 * WBX Version 3 Constants
 **********************************************************************/
static const uhd::dict<std::string, gain_range_t> wbx_v3_tx_gain_ranges = map_list_of
    ("PGA0", gain_range_t(0, 31, 1.0))
;

static const freq_range_t wbx_v3_freq_range(68.75e6, 2.2e9);

/***********************************************************************
 * Gain-related functions
 **********************************************************************/
static int tx_pga0_gain_to_iobits(double &gain){
    //clip the input
    gain = wbx_v3_tx_gain_ranges["PGA0"].clip(gain);

    //convert to attenuation
    double attn = wbx_v3_tx_gain_ranges["PGA0"].stop() - gain;

    //calculate the attenuation
    int attn_code = boost::math::iround(attn);
    int iobits = (
            (attn_code & 16 ? 0 : TX_ATTN_16) |
            (attn_code &  8 ? 0 : TX_ATTN_8) |
            (attn_code &  4 ? 0 : TX_ATTN_4) |
            (attn_code &  2 ? 0 : TX_ATTN_2) |
            (attn_code &  1 ? 0 : TX_ATTN_1)
        ) & TX_ATTN_MASK;

    UHD_LOGV(often) << boost::format(
        "WBX TX Attenuation: %f dB, Code: %d, IO Bits %x, Mask: %x"
    ) % attn % attn_code % (iobits & TX_ATTN_MASK) % TX_ATTN_MASK << std::endl;

    //the actual gain setting
    gain = wbx_v3_tx_gain_ranges["PGA0"].stop() - double(attn_code);

    return iobits;
}


/***********************************************************************
 * WBX Common Implementation
 **********************************************************************/
wbx_base::wbx_version3::wbx_version3(wbx_base *_self_wbx_base) {
    //register our handle on the primary wbx_base instance
    self_base = _self_wbx_base;

    ////////////////////////////////////////////////////////////////////
    // Register RX properties
    ////////////////////////////////////////////////////////////////////
    this->get_rx_subtree()->create<std::string>("name").set("WBXv3 RX");
    this->get_rx_subtree()->create<double>("freq/value")
         .coerce(boost::bind(&wbx_base::wbx_version3::set_lo_freq, this, dboard_iface::UNIT_RX, _1))
         .set((wbx_v3_freq_range.start() + wbx_v3_freq_range.stop())/2.0);
    this->get_rx_subtree()->create<meta_range_t>("freq/range").set(wbx_v3_freq_range);

    ////////////////////////////////////////////////////////////////////
    // Register TX properties
    ////////////////////////////////////////////////////////////////////
    this->get_tx_subtree()->create<std::string>("name").set("WBXv3 TX");
    BOOST_FOREACH(const std::string &name, wbx_v3_tx_gain_ranges.keys()){
        self_base->get_tx_subtree()->create<double>("gains/"+name+"/value")
            .coerce(boost::bind(&wbx_base::wbx_version3::set_tx_gain, this, _1, name))
            .set(wbx_v3_tx_gain_ranges[name].start());
        self_base->get_tx_subtree()->create<meta_range_t>("gains/"+name+"/range")
            .set(wbx_v3_tx_gain_ranges[name]);
    }
    this->get_tx_subtree()->create<double>("freq/value")
         .coerce(boost::bind(&wbx_base::wbx_version3::set_lo_freq, this, dboard_iface::UNIT_TX, _1))
         .set((wbx_v3_freq_range.start() + wbx_v3_freq_range.stop())/2.0);
    this->get_tx_subtree()->create<meta_range_t>("freq/range").set(wbx_v3_freq_range);
    this->get_tx_subtree()->create<bool>("enabled")
        .subscribe(boost::bind(&wbx_base::wbx_version3::set_tx_enabled, this, _1))
        .set(true); //start enabled

    //set attenuator control bits
    int v3_iobits = TX_ATTN_MASK;
    int v3_tx_mod = ADF435X_PDBRF;

    //set the gpio directions and atr controls
    self_base->get_iface()->set_pin_ctrl(dboard_iface::UNIT_TX, \
            v3_tx_mod|v3_iobits);
    self_base->get_iface()->set_pin_ctrl(dboard_iface::UNIT_RX, \
            RXBB_PDB|ADF435X_PDBRF);
    self_base->get_iface()->set_gpio_ddr(dboard_iface::UNIT_TX, \
            TX_PUP_5V|TX_PUP_3V|v3_tx_mod|v3_iobits);
    self_base->get_iface()->set_gpio_ddr(dboard_iface::UNIT_RX, \
            RX_PUP_5V|RX_PUP_3V|ADF435X_CE|RXBB_PDB|ADF435X_PDBRF|RX_ATTN_MASK);

    //setup ATR for the mixer enables (always enabled to prevent phase
    //slip between bursts).  set TX gain iobits to min gain (max attenuation)
    //when RX_ONLY or IDLE to suppress LO leakage
    self_base->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, \
            dboard_iface::ATR_REG_IDLE, v3_tx_mod, \
            TX_ATTN_MASK | TX_MIXER_DIS | v3_tx_mod);
    self_base->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, \
            dboard_iface::ATR_REG_RX_ONLY, v3_tx_mod, \
            TX_ATTN_MASK | TX_MIXER_DIS | v3_tx_mod);
    self_base->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, \
            dboard_iface::ATR_REG_TX_ONLY, v3_tx_mod, \
            TX_ATTN_MASK | TX_MIXER_DIS | v3_tx_mod);
    self_base->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, \
            dboard_iface::ATR_REG_FULL_DUPLEX, v3_tx_mod, \
            TX_ATTN_MASK | TX_MIXER_DIS | v3_tx_mod);

    self_base->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, \
            dboard_iface::ATR_REG_IDLE, \
            RX_MIXER_ENB, RX_MIXER_DIS | RX_MIXER_ENB);
    self_base->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, \
            dboard_iface::ATR_REG_TX_ONLY, \
            RX_MIXER_ENB, RX_MIXER_DIS | RX_MIXER_ENB);
    self_base->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, \
            dboard_iface::ATR_REG_RX_ONLY, \
            RX_MIXER_ENB, RX_MIXER_DIS | RX_MIXER_ENB);
    self_base->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, \
            dboard_iface::ATR_REG_FULL_DUPLEX, \
            RX_MIXER_ENB, RX_MIXER_DIS | RX_MIXER_ENB);
}

wbx_base::wbx_version3::~wbx_version3(void){
    /* NOP */
}


/***********************************************************************
 * Enables
 **********************************************************************/
void wbx_base::wbx_version3::set_tx_enabled(bool enb){
    self_base->get_iface()->set_gpio_out(dboard_iface::UNIT_TX,
        (enb)? TX_POWER_UP | ADF435X_CE : TX_POWER_DOWN, TX_POWER_UP | TX_POWER_DOWN | 0);
}


/***********************************************************************
 * Gain Handling
 **********************************************************************/
double wbx_base::wbx_version3::set_tx_gain(double gain, const std::string &name){
    assert_has(wbx_v3_tx_gain_ranges.keys(), name, "wbx tx gain name");
    if(name == "PGA0"){
        boost::uint16_t io_bits = tx_pga0_gain_to_iobits(gain);

        self_base->_tx_gains[name] = gain;

        //write the new gain to tx gpio outputs
        //Update ATR with gain io_bits, only update for TX_ONLY and FULL_DUPLEX ATR states
        self_base->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, dboard_iface::ATR_REG_TX_ONLY,     io_bits, TX_ATTN_MASK);
        self_base->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, dboard_iface::ATR_REG_FULL_DUPLEX, io_bits, TX_ATTN_MASK);
    }
    else UHD_THROW_INVALID_CODE_PATH();
    return self_base->_tx_gains[name]; //shadow
}


/***********************************************************************
 * Tuning
 **********************************************************************/
double wbx_base::wbx_version3::set_lo_freq(dboard_iface::unit_t unit, double target_freq) {
    //clip to tuning range
    target_freq = wbx_v3_freq_range.clip(target_freq);

    UHD_LOGV(often) << boost::format(
        "WBX tune: target frequency %f MHz"
    ) % (target_freq/1e6) << std::endl;

    /*
     * If the user sets 'mode_n=integer' in the tuning args, the user wishes to
     * tune in Integer-N mode, which can result in better spur
     * performance on some mixers. The default is fractional tuning.
     */
    property_tree::sptr subtree = (unit == dboard_iface::UNIT_RX) ? self_base->get_rx_subtree()
                                                                  : self_base->get_tx_subtree();
    device_addr_t tune_args = subtree->access<device_addr_t>("tune_args").get();
    bool is_int_n = boost::iequals(tune_args.get("mode_n",""), "integer");

    //map prescaler setting to mininmum integer divider (N) values (pg.18 prescaler)
    static const uhd::dict<int, int> prescaler_to_min_int_div = map_list_of
        (0,23) //adf4350_regs_t::PRESCALER_4_5
        (1,75) //adf4350_regs_t::PRESCALER_8_9
    ;

    //map rf divider select output dividers to enums
    static const uhd::dict<int, adf4350_regs_t::rf_divider_select_t> rfdivsel_to_enum = map_list_of
        (1,  adf4350_regs_t::RF_DIVIDER_SELECT_DIV1)
        (2,  adf4350_regs_t::RF_DIVIDER_SELECT_DIV2)
        (4,  adf4350_regs_t::RF_DIVIDER_SELECT_DIV4)
        (8,  adf4350_regs_t::RF_DIVIDER_SELECT_DIV8)
        (16, adf4350_regs_t::RF_DIVIDER_SELECT_DIV16)
    ;

    double reference_freq = self_base->get_iface()->get_clock_rate(unit);
    //The mixer has a divide-by-2 stage on the LO port so the synthesizer
    //frequency must 2x the target frequency.  This introduces a 180 degree
    //phase ambiguity
    double synth_target_freq = target_freq * 2;

    adf4350_regs_t::prescaler_t prescaler =
        synth_target_freq > 3e9 ? adf4350_regs_t::PRESCALER_8_9 : adf4350_regs_t::PRESCALER_4_5;

    adf435x_tuning_constraints tuning_constraints;
    tuning_constraints.force_frac0 = is_int_n;
    tuning_constraints.band_sel_freq_max = 100e3;
    tuning_constraints.ref_doubler_threshold = 12.5e6;
    tuning_constraints.int_range = uhd::range_t(prescaler_to_min_int_div[prescaler], 4095);
    tuning_constraints.pfd_freq_max = 25e6;
    tuning_constraints.rf_divider_range = uhd::range_t(1, 16);
    //The feedback of the divided frequency must be disabled whenever the target frequency
    //divided by the minimum PFD frequency cannot meet the minimum integer divider (N) value.
    //If it is disabled, additional phase ambiguity will be introduced.  With a minimum PFD
    //frequency of 10 MHz, synthesizer frequencies below 230 MHz (LO frequencies below 115 MHz)
    //will have too much ambiguity to synchronize.
    tuning_constraints.feedback_after_divider =
        (int(synth_target_freq / 10e6) >= prescaler_to_min_int_div[prescaler]);

    double synth_actual_freq = 0;
    adf435x_tuning_settings tuning_settings = tune_adf435x_synth(
        synth_target_freq, reference_freq, tuning_constraints, synth_actual_freq);

    //The mixer has a divide-by-2 stage on the LO port so the synthesizer
    //actual_freq must /2 the synth_actual_freq
    double actual_freq = synth_actual_freq / 2;

    //load the register values
    adf4350_regs_t regs;

    if (unit == dboard_iface::UNIT_RX)
        regs.output_power = (actual_freq == wbx_rx_lo_5dbm.clip(actual_freq)) ? adf4350_regs_t::OUTPUT_POWER_5DBM
                                                                              : adf4350_regs_t::OUTPUT_POWER_2DBM;
    else
        regs.output_power = (actual_freq == wbx_tx_lo_5dbm.clip(actual_freq)) ? adf4350_regs_t::OUTPUT_POWER_5DBM
                                                                              : adf4350_regs_t::OUTPUT_POWER_M1DBM;

    regs.frac_12_bit            = tuning_settings.frac_12_bit;
    regs.int_16_bit             = tuning_settings.int_16_bit;
    regs.mod_12_bit             = tuning_settings.mod_12_bit;
    regs.clock_divider_12_bit   = tuning_settings.clock_divider_12_bit;
    regs.feedback_select        = tuning_constraints.feedback_after_divider ?
                                    adf4350_regs_t::FEEDBACK_SELECT_DIVIDED :
                                    adf4350_regs_t::FEEDBACK_SELECT_FUNDAMENTAL;
    regs.clock_div_mode         = tuning_constraints.feedback_after_divider ?
                                    adf4350_regs_t::CLOCK_DIV_MODE_RESYNC_ENABLE :
                                    adf4350_regs_t::CLOCK_DIV_MODE_FAST_LOCK;
    regs.prescaler              = prescaler;
    regs.r_counter_10_bit       = tuning_settings.r_counter_10_bit;
    regs.reference_divide_by_2  = tuning_settings.r_divide_by_2_en ?
                                    adf4350_regs_t::REFERENCE_DIVIDE_BY_2_ENABLED :
                                    adf4350_regs_t::REFERENCE_DIVIDE_BY_2_DISABLED;
    regs.reference_doubler      = tuning_settings.r_doubler_en ?
                                    adf4350_regs_t::REFERENCE_DOUBLER_ENABLED :
                                    adf4350_regs_t::REFERENCE_DOUBLER_DISABLED;
    regs.band_select_clock_div  = tuning_settings.band_select_clock_div;
    UHD_ASSERT_THROW(rfdivsel_to_enum.has_key(tuning_settings.rf_divider));
    regs.rf_divider_select      = rfdivsel_to_enum[tuning_settings.rf_divider];
    regs.ldf                    = is_int_n ?
                                    adf4350_regs_t::LDF_INT_N :
                                    adf4350_regs_t::LDF_FRAC_N;

    //reset the N and R counter
    regs.counter_reset = adf4350_regs_t::COUNTER_RESET_ENABLED;
    self_base->get_iface()->write_spi(unit, spi_config_t::EDGE_RISE, regs.get_reg(2), 32);
    regs.counter_reset = adf4350_regs_t::COUNTER_RESET_DISABLED;

    //write the registers
    //correct power-up sequence to write registers (5, 4, 3, 2, 1, 0)
    int addr;

    for(addr=5; addr>=0; addr--){
        UHD_LOGV(often) << boost::format(
            "WBX SPI Reg (0x%02x): 0x%08x"
        ) % addr % regs.get_reg(addr) << std::endl;
        self_base->get_iface()->write_spi(
            unit, spi_config_t::EDGE_RISE,
            regs.get_reg(addr), 32
        );
    }

    //return the actual frequency
    UHD_LOGV(often) << boost::format(
        "WBX tune: actual frequency %f MHz"
    ) % (actual_freq/1e6) << std::endl;
    return actual_freq;
}
