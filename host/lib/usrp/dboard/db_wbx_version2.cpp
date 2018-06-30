//
// Copyright 2011-2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "db_wbx_common.hpp"
#include <uhd/types/tune_request.hpp>
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
 * WBX Version 2 Constants
 **********************************************************************/
static const uhd::dict<std::string, gain_range_t> wbx_v2_tx_gain_ranges = map_list_of
    ("PGA0", gain_range_t(0, 25, 0.05))
;

static const freq_range_t wbx_v2_freq_range(68.75e6, 2.2e9);

/***********************************************************************
 * Gain-related functions
 **********************************************************************/
static double tx_pga0_gain_to_dac_volts(double &gain){
    //clip the input
    gain = wbx_v2_tx_gain_ranges["PGA0"].clip(gain);

    //voltage level constants
    static const double max_volts = 0.5, min_volts = 1.4;
    static const double slope = (max_volts-min_volts)/wbx_v2_tx_gain_ranges["PGA0"].stop();

    //calculate the voltage for the aux dac
    double dac_volts = gain*slope + min_volts;

    UHD_LOGGER_TRACE("WBX") << boost::format(
        "WBX TX Gain: %f dB, dac_volts: %f V"
    ) % gain % dac_volts ;

    //the actual gain setting
    gain = (dac_volts - min_volts)/slope;

    return dac_volts;
}


/***********************************************************************
 * WBX Version 2 Implementation
 **********************************************************************/
wbx_base::wbx_version2::wbx_version2(wbx_base *_self_wbx_base) {
    //register our handle on the primary wbx_base instance
    self_base = _self_wbx_base;
    _txlo = adf435x_iface::make_adf4350(boost::bind(&wbx_base::wbx_versionx::write_lo_regs, this, dboard_iface::UNIT_TX, _1));
    _rxlo = adf435x_iface::make_adf4350(boost::bind(&wbx_base::wbx_versionx::write_lo_regs, this, dboard_iface::UNIT_RX, _1));

    ////////////////////////////////////////////////////////////////////
    // Register RX properties
    ////////////////////////////////////////////////////////////////////
    this->get_rx_subtree()->create<std::string>("name").set("WBXv2 RX");
    this->get_rx_subtree()->create<double>("freq/value")
         .set_coercer(boost::bind(&wbx_base::wbx_version2::set_lo_freq, this, dboard_iface::UNIT_RX, _1))
         .set((wbx_v2_freq_range.start() + wbx_v2_freq_range.stop())/2.0);
    this->get_rx_subtree()->create<meta_range_t>("freq/range").set(wbx_v2_freq_range);

    ////////////////////////////////////////////////////////////////////
    // Register TX properties
    ////////////////////////////////////////////////////////////////////
    this->get_tx_subtree()->create<std::string>("name").set("WBXv2 TX");
    for(const std::string &name:  wbx_v2_tx_gain_ranges.keys()){
        self_base->get_tx_subtree()->create<double>("gains/"+name+"/value")
            .set_coercer(boost::bind(&wbx_base::wbx_version2::set_tx_gain, this, _1, name))
            .set(wbx_v2_tx_gain_ranges[name].start());
        self_base->get_tx_subtree()->create<meta_range_t>("gains/"+name+"/range")
            .set(wbx_v2_tx_gain_ranges[name]);
    }
    this->get_tx_subtree()->create<double>("freq/value")
         .set_coercer(boost::bind(&wbx_base::wbx_version2::set_lo_freq, this, dboard_iface::UNIT_TX, _1))
         .set((wbx_v2_freq_range.start() + wbx_v2_freq_range.stop())/2.0);
    this->get_tx_subtree()->create<meta_range_t>("freq/range").set(wbx_v2_freq_range);
    this->get_tx_subtree()->create<bool>("enabled")
        .add_coerced_subscriber(boost::bind(&wbx_base::wbx_version2::set_tx_enabled, this, _1))
        .set(true); //start enabled

    //set attenuator control bits
    int v2_iobits = ADF435X_CE;
    int v2_tx_mod = TXMOD_EN|ADF435X_PDBRF;

    //set the gpio directions and atr controls
    self_base->get_iface()->set_pin_ctrl(dboard_iface::UNIT_TX, v2_tx_mod);
    self_base->get_iface()->set_pin_ctrl(dboard_iface::UNIT_RX, RXBB_PDB|ADF435X_PDBRF);
    self_base->get_iface()->set_gpio_ddr(dboard_iface::UNIT_TX, TX_PUP_5V|TX_PUP_3V|v2_tx_mod|v2_iobits);
    self_base->get_iface()->set_gpio_ddr(dboard_iface::UNIT_RX, RX_PUP_5V|RX_PUP_3V|ADF435X_CE|RXBB_PDB|ADF435X_PDBRF|RX_ATTN_MASK);

    //setup ATR for the mixer enables (always enabled to prevent phase slip between bursts)
    self_base->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, gpio_atr::ATR_REG_IDLE,        v2_tx_mod, TX_MIXER_DIS | v2_tx_mod);
    self_base->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, gpio_atr::ATR_REG_RX_ONLY,     v2_tx_mod, TX_MIXER_DIS | v2_tx_mod);
    self_base->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, gpio_atr::ATR_REG_TX_ONLY,     v2_tx_mod, TX_MIXER_DIS | v2_tx_mod);
    self_base->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, gpio_atr::ATR_REG_FULL_DUPLEX, v2_tx_mod, TX_MIXER_DIS | v2_tx_mod);

    self_base->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, gpio_atr::ATR_REG_IDLE,        RX_MIXER_ENB, RX_MIXER_DIS | RX_MIXER_ENB);
    self_base->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, gpio_atr::ATR_REG_TX_ONLY,     RX_MIXER_ENB, RX_MIXER_DIS | RX_MIXER_ENB);
    self_base->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, gpio_atr::ATR_REG_RX_ONLY,     RX_MIXER_ENB, RX_MIXER_DIS | RX_MIXER_ENB);
    self_base->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, gpio_atr::ATR_REG_FULL_DUPLEX, RX_MIXER_ENB, RX_MIXER_DIS | RX_MIXER_ENB);
}

wbx_base::wbx_version2::~wbx_version2(void){
    /* NOP */
}

/***********************************************************************
 * Enables
 **********************************************************************/
void wbx_base::wbx_version2::set_tx_enabled(bool enb){
    self_base->get_iface()->set_gpio_out(dboard_iface::UNIT_TX,
        (enb)? TX_POWER_UP | ADF435X_CE : TX_POWER_DOWN, TX_POWER_UP | TX_POWER_DOWN | ADF435X_CE);
}


/***********************************************************************
 * Gain Handling
 **********************************************************************/
double wbx_base::wbx_version2::set_tx_gain(double gain, const std::string &name){
    assert_has(wbx_v2_tx_gain_ranges.keys(), name, "wbx tx gain name");
    if(name == "PGA0"){
        double dac_volts = tx_pga0_gain_to_dac_volts(gain);
        self_base->_tx_gains[name] = gain;

        //write the new voltage to the aux dac
        self_base->get_iface()->write_aux_dac(dboard_iface::UNIT_TX, dboard_iface::AUX_DAC_A, dac_volts);
    }
    else UHD_THROW_INVALID_CODE_PATH();
    return self_base->_tx_gains[name]; //shadowed
}


/***********************************************************************
 * Tuning
 **********************************************************************/
double wbx_base::wbx_version2::set_lo_freq(dboard_iface::unit_t unit, double target_freq) {
    //clip to tuning range
    target_freq = wbx_v2_freq_range.clip(target_freq);

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
