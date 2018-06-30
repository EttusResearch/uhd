//
// Copyright 2011-2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "db_wbx_common.hpp"
#include <uhd/utils/log.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/types/sensors.hpp>
#include <uhd/utils/assert_has.hpp>
#include <uhd/utils/algorithm.hpp>

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

    UHD_LOGGER_TRACE("WBX") << boost::format(
        "WBX TX Attenuation: %f dB, Code: %d, IO Bits %x, Mask: %x"
    ) % attn % attn_code % (iobits & TX_ATTN_MASK) % TX_ATTN_MASK ;

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
    _txlo = adf435x_iface::make_adf4350(boost::bind(&wbx_base::wbx_versionx::write_lo_regs, this, dboard_iface::UNIT_TX, _1));
    _rxlo = adf435x_iface::make_adf4350(boost::bind(&wbx_base::wbx_versionx::write_lo_regs, this, dboard_iface::UNIT_RX, _1));

    ////////////////////////////////////////////////////////////////////
    // Register RX properties
    ////////////////////////////////////////////////////////////////////
    this->get_rx_subtree()->create<std::string>("name").set("WBXv3 RX");
    this->get_rx_subtree()->create<double>("freq/value")
         .set_coercer(boost::bind(&wbx_base::wbx_version3::set_lo_freq, this, dboard_iface::UNIT_RX, _1))
         .set((wbx_v3_freq_range.start() + wbx_v3_freq_range.stop())/2.0);
    this->get_rx_subtree()->create<meta_range_t>("freq/range").set(wbx_v3_freq_range);

    ////////////////////////////////////////////////////////////////////
    // Register TX properties
    ////////////////////////////////////////////////////////////////////
    this->get_tx_subtree()->create<std::string>("name").set("WBXv3 TX");
    for(const std::string &name:  wbx_v3_tx_gain_ranges.keys()){
        self_base->get_tx_subtree()->create<double>("gains/"+name+"/value")
            .set_coercer(boost::bind(&wbx_base::wbx_version3::set_tx_gain, this, _1, name))
            .set(wbx_v3_tx_gain_ranges[name].start());
        self_base->get_tx_subtree()->create<meta_range_t>("gains/"+name+"/range")
            .set(wbx_v3_tx_gain_ranges[name]);
    }
    this->get_tx_subtree()->create<double>("freq/value")
         .set_coercer(boost::bind(&wbx_base::wbx_version3::set_lo_freq, this, dboard_iface::UNIT_TX, _1))
         .set((wbx_v3_freq_range.start() + wbx_v3_freq_range.stop())/2.0);
    this->get_tx_subtree()->create<meta_range_t>("freq/range").set(wbx_v3_freq_range);
    this->get_tx_subtree()->create<bool>("enabled")
        .add_coerced_subscriber(boost::bind(&wbx_base::wbx_version3::set_tx_enabled, this, _1))
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
            gpio_atr::ATR_REG_IDLE, v3_tx_mod, \
            TX_ATTN_MASK | TX_MIXER_DIS | v3_tx_mod);
    self_base->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, \
            gpio_atr::ATR_REG_RX_ONLY, v3_tx_mod, \
            TX_ATTN_MASK | TX_MIXER_DIS | v3_tx_mod);
    self_base->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, \
            gpio_atr::ATR_REG_TX_ONLY, v3_tx_mod, \
            TX_ATTN_MASK | TX_MIXER_DIS | v3_tx_mod);
    self_base->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, \
            gpio_atr::ATR_REG_FULL_DUPLEX, v3_tx_mod, \
            TX_ATTN_MASK | TX_MIXER_DIS | v3_tx_mod);

    self_base->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, \
            gpio_atr::ATR_REG_IDLE, \
            RX_MIXER_ENB, RX_MIXER_DIS | RX_MIXER_ENB);
    self_base->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, \
            gpio_atr::ATR_REG_TX_ONLY, \
            RX_MIXER_ENB, RX_MIXER_DIS | RX_MIXER_ENB);
    self_base->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, \
            gpio_atr::ATR_REG_RX_ONLY, \
            RX_MIXER_ENB, RX_MIXER_DIS | RX_MIXER_ENB);
    self_base->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, \
            gpio_atr::ATR_REG_FULL_DUPLEX, \
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
        uint16_t io_bits = tx_pga0_gain_to_iobits(gain);

        self_base->_tx_gains[name] = gain;

        //write the new gain to tx gpio outputs
        //Update ATR with gain io_bits, only update for TX_ONLY and FULL_DUPLEX ATR states
        self_base->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, gpio_atr::ATR_REG_TX_ONLY,     io_bits, TX_ATTN_MASK);
        self_base->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, gpio_atr::ATR_REG_FULL_DUPLEX, io_bits, TX_ATTN_MASK);
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

    UHD_LOGGER_TRACE("WBX") << boost::format(
        "WBX tune: target frequency %f MHz"
    ) % (target_freq/1e6) ;

    /*
     * If the user sets 'mode_n=integer' in the tuning args, the user wishes to
     * tune in Integer-N mode, which can result in better spur
     * performance on some mixers. The default is fractional tuning.
     */
    property_tree::sptr subtree = (unit == dboard_iface::UNIT_RX) ? self_base->get_rx_subtree()
                                                                  : self_base->get_tx_subtree();
    device_addr_t tune_args = subtree->access<device_addr_t>("tune_args").get();
    bool is_int_n = boost::iequals(tune_args.get("mode_n",""), "integer");
    double reference_freq = self_base->get_iface()->get_clock_rate(unit);

    //Select the LO
    adf435x_iface::sptr& lo_iface = unit == dboard_iface::UNIT_RX ? _rxlo : _txlo;
    lo_iface->set_reference_freq(reference_freq);

    //The mixer has a divide-by-2 stage on the LO port so the synthesizer
    //frequency must 2x the target frequency
    double synth_target_freq = target_freq * 2;

    //Use 8/9 prescaler for vco_freq > 3 GHz (pg.18 prescaler)
    lo_iface->set_prescaler(synth_target_freq > 3e9 ?
        adf435x_iface::PRESCALER_8_9 : adf435x_iface::PRESCALER_4_5);

    //The feedback of the divided frequency must be disabled whenever the target frequency
    //divided by the minimum PFD frequency cannot meet the minimum integer divider (N) value.
    //If it is disabled, additional phase ambiguity will be introduced.  With a minimum PFD
    //frequency of 10 MHz, synthesizer frequencies below 230 MHz (LO frequencies below 115 MHz)
    //will have too much ambiguity to synchronize.
    lo_iface->set_feedback_select(
        (int(synth_target_freq / 10e6) >= lo_iface->get_int_range().start() ?
            adf435x_iface::FB_SEL_DIVIDED : adf435x_iface::FB_SEL_FUNDAMENTAL));

    double synth_actual_freq = lo_iface->set_frequency(synth_target_freq, is_int_n);

    //The mixer has a divide-by-2 stage on the LO port so the synthesizer
    //actual_freq must /2 the synth_actual_freq
    double actual_freq = synth_actual_freq / 2;

    if (unit == dboard_iface::UNIT_RX) {
        lo_iface->set_output_power((actual_freq == wbx_rx_lo_5dbm.clip(actual_freq)) ?
            adf435x_iface::OUTPUT_POWER_5DBM : adf435x_iface::OUTPUT_POWER_2DBM);
    } else {
        lo_iface->set_output_power((actual_freq == wbx_tx_lo_5dbm.clip(actual_freq)) ?
            adf435x_iface::OUTPUT_POWER_5DBM : adf435x_iface::OUTPUT_POWER_M1DBM);
    }

    //Write to hardware
    lo_iface->commit();

    return actual_freq;
}
