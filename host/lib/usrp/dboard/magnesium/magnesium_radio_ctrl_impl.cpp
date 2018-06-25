//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "magnesium_radio_ctrl_impl.hpp"
#include "magnesium_constants.hpp"
#include "magnesium_gain_table.hpp"
#include <uhd/utils/log.hpp>
#include <uhd/rfnoc/node_ctrl_base.hpp>
#include <uhd/transport/chdr.hpp>
#include <uhd/utils/algorithm.hpp>
#include <uhd/utils/math.hpp>
#include <uhd/types/direction.hpp>
#include <uhd/types/eeprom.hpp>
#include <uhd/exception.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/make_shared.hpp>
#include <boost/format.hpp>
#include <sstream>
#include <cmath>
#include <cstdlib>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::rfnoc;
using namespace uhd::math::fp_compare;

namespace {
    /**************************************************************************
     * ADF4351 Controls
     *************************************************************************/
    /*!
     * \param lo_iface Reference to the LO object
     * \param freq Frequency (in Hz) of the tone to be generated from the LO
     * \param ref_clock_freq Frequency (in Hz) of the reference clock at the
     *                       PLL input of the LO
     * \param int_n_mode Integer-N mode on or off
     */
    double _lo_set_frequency(
        adf435x_iface::sptr lo_iface,
        const double freq,
        const double ref_clock_freq,
        const bool int_n_mode
    ) {
        UHD_LOG_TRACE("MG/ADF4351",
            "Attempting to tune low band LO to " << freq <<
            " Hz with ref clock freq " << ref_clock_freq);
        lo_iface->set_feedback_select(adf435x_iface::FB_SEL_DIVIDED);
        lo_iface->set_reference_freq(ref_clock_freq);
        lo_iface->set_prescaler(adf435x_iface::PRESCALER_4_5);
        const double actual_freq = lo_iface->set_frequency(freq, int_n_mode);
        lo_iface->set_output_power(
            adf435x_iface::RF_OUTPUT_A,
            adf435x_iface::OUTPUT_POWER_2DBM
        );
        lo_iface->set_output_power(
            adf435x_iface::RF_OUTPUT_B,
            adf435x_iface::OUTPUT_POWER_2DBM
        );
        lo_iface->set_charge_pump_current(
                adf435x_iface::CHARGE_PUMP_CURRENT_0_31MA);
        return actual_freq;
    }

    /*! Configure and enable LO
     *
     * Will tune it to requested frequency and enable outputs.
     *
     * \param lo_iface Reference to the LO object
     * \param lo_freq Frequency (in Hz) of the tone to be generated from the LO
     * \param ref_clock_freq Frequency (in Hz) of the reference clock at the
     *                       PLL input of the LO
     * \param int_n_mode Integer-N mode on or off
     * \returns the actual frequency the LO is running at
     */
    double _lo_enable(
        adf435x_iface::sptr lo_iface,
        const double lo_freq,
        const double ref_clock_freq,
        const bool int_n_mode
    ) {
        const double actual_lo_freq =
            _lo_set_frequency(lo_iface, lo_freq, ref_clock_freq, int_n_mode);
        lo_iface->set_output_enable(adf435x_iface::RF_OUTPUT_A, true);
        lo_iface->set_output_enable(adf435x_iface::RF_OUTPUT_B, true);
        lo_iface->commit();
        return actual_lo_freq;
    }

    /*! Disable LO
     */
    void _lo_disable(adf435x_iface::sptr lo_iface)
    {
        lo_iface->set_output_enable(adf435x_iface::RF_OUTPUT_A, false);
        lo_iface->set_output_enable(adf435x_iface::RF_OUTPUT_B, false);
        lo_iface->commit();
    }
}


/******************************************************************************
 * Structors
 *****************************************************************************/
UHD_RFNOC_RADIO_BLOCK_CONSTRUCTOR(magnesium_radio_ctrl)
{
    UHD_LOG_TRACE(unique_id(), "Entering magnesium_radio_ctrl_impl ctor...");
    const char radio_slot_name[2] = {'A', 'B'};
    _radio_slot = radio_slot_name[get_block_id().get_block_count()];
    UHD_LOG_TRACE(unique_id(), "Radio slot: " << _radio_slot);
    _rpc_prefix =
        (_radio_slot == "A") ? "db_0_" : "db_1_";

    _init_defaults();
    _init_peripherals();
    _init_prop_tree();
}

magnesium_radio_ctrl_impl::~magnesium_radio_ctrl_impl()
{
    UHD_LOG_TRACE(unique_id(), "magnesium_radio_ctrl_impl::dtor() ");
}


/******************************************************************************
 * API Calls
 *****************************************************************************/
double magnesium_radio_ctrl_impl::set_rate(double rate)
{
    std::lock_guard<std::mutex> l(_set_lock);
    // TODO: implement
    if (rate != get_rate()) {
        UHD_LOG_WARNING(unique_id(),
                "Attempting to set sampling rate to invalid value " << rate);
    }
    return get_rate();
}

void magnesium_radio_ctrl_impl::set_tx_antenna(
        const std::string &ant,
        const size_t chan
) {
    if (ant != get_tx_antenna(chan)) {
        throw uhd::value_error(str(
            boost::format("[%s] Requesting invalid TX antenna value: %s")
            % unique_id()
            % ant
        ));
    }
    // We can't actually set the TX antenna, so let's stop here.
}

void magnesium_radio_ctrl_impl::set_rx_antenna(
        const std::string &ant,
        const size_t chan
) {
    UHD_ASSERT_THROW(chan <= MAGNESIUM_NUM_CHANS);
    if (std::find(MAGNESIUM_RX_ANTENNAS.begin(),
                  MAGNESIUM_RX_ANTENNAS.end(),
                  ant) == MAGNESIUM_RX_ANTENNAS.end()) {
        throw uhd::value_error(str(
            boost::format("[%s] Requesting invalid RX antenna value: %s")
            % unique_id()
            % ant
        ));
    }
    UHD_LOG_TRACE(unique_id(),
        "Setting RX antenna to " << ant << " for chan " << chan);
    magnesium_cpld_ctrl::chan_sel_t chan_sel  =
        chan ? magnesium_cpld_ctrl::CHAN1 : magnesium_cpld_ctrl::CHAN2;
    _update_atr_switches(chan_sel, RX_DIRECTION, ant);

    radio_ctrl_impl::set_rx_antenna(ant, chan); // we don't use _master here since each radio has one antenna.
}

double magnesium_radio_ctrl_impl::set_tx_frequency(
        const double freq,
        const size_t chan
) {
    // FIXME bounds checking + clipping!!!
    UHD_LOG_TRACE(unique_id(),
        "set_tx_frequency(f=" << freq << ", chan=" << chan << ")");
    _desired_rf_freq[TX_DIRECTION]=freq;
    std::lock_guard<std::mutex> l(_set_lock);
    // We need to set the switches on both channels, because they share an LO.
    // This way, if we tune channel 0 it will not put channel 1 into a bad
    // state.
    _update_tx_freq_switches(freq, _tx_bypass_amp, magnesium_cpld_ctrl::BOTH);
    const std::string ad9371_source = this->get_tx_lo_source(MAGNESIUM_LO1, chan);
    const std::string adf4351_source = this->get_tx_lo_source(MAGNESIUM_LO2, chan);
    UHD_ASSERT_THROW(adf4351_source == "internal");
    double coerced_if_freq = freq;

    if (_map_freq_to_tx_band(freq) == tx_band::LOWBAND) {
        _is_low_band[TX_DIRECTION] = true;
        const double desired_low_freq = MAGNESIUM_TX_IF_FREQ - freq;
        coerced_if_freq =
            this->_set_tx_lo_freq(adf4351_source, MAGNESIUM_LO2, desired_low_freq, chan) + freq;
        UHD_LOG_TRACE(unique_id(), "coerced_if_freq = " << coerced_if_freq);
    } else {
        _is_low_band[TX_DIRECTION] = false;
        _lo_disable(_tx_lo);
    }
    // external LO required to tune at 2xdesired_frequency.
    const double desired_if_freq = (ad9371_source == "internal") ?
        coerced_if_freq :
        2*coerced_if_freq;

    this->_set_tx_lo_freq(ad9371_source, MAGNESIUM_LO1, desired_if_freq, chan);
    this->_update_freq(chan, TX_DIRECTION);
    this->_update_gain(chan, TX_DIRECTION);
    return radio_ctrl_impl::get_tx_frequency(chan);
}

void magnesium_radio_ctrl_impl::_update_gain(
        const size_t chan,
        const uhd::direction_t dir
) {
    const std::string fe =
        (dir == TX_DIRECTION) ? "tx_frontends" : "rx_frontends";
    const double freq =  (dir == TX_DIRECTION) ?
        this->get_tx_frequency(chan) :
        this->get_rx_frequency(chan);
    this->_set_all_gain(this->_get_all_gain(chan, dir), freq, chan, dir);
}

void magnesium_radio_ctrl_impl::_update_freq(
        const size_t chan,
        const uhd::direction_t dir
) {
    const std::string ad9371_source = dir == TX_DIRECTION ?
        this->get_tx_lo_source(MAGNESIUM_LO1, chan) :
        this->get_rx_lo_source(MAGNESIUM_LO1, chan)
    ;

    const double ad9371_freq = ad9371_source == "external" ?
        _ad9371_freq[dir]/2 :
        _ad9371_freq[dir]
    ;
    const double rf_freq = _is_low_band[dir] ?
        ad9371_freq - _adf4351_freq[dir] :
        ad9371_freq
    ;

    UHD_LOG_TRACE(unique_id(),
         "RF freq = " << rf_freq);
        UHD_ASSERT_THROW(fp_compare_epsilon<double>(rf_freq) >= 0);
        UHD_ASSERT_THROW(
            fp_compare_epsilon<double>(std::abs(rf_freq - _desired_rf_freq[dir])) <= _master_clock_rate/2);
    if (dir == RX_DIRECTION){
        radio_ctrl_impl::set_rx_frequency(rf_freq, chan);
    }else if (dir == TX_DIRECTION){
        radio_ctrl_impl::set_tx_frequency(rf_freq, chan);
    }else{
        UHD_THROW_INVALID_CODE_PATH();
    }
}

double magnesium_radio_ctrl_impl::set_rx_frequency(
        const double freq,
        const size_t chan
) {
    // FIXME bounds checking + clipping!!!
    UHD_LOG_TRACE(unique_id(),
        "set_rx_frequency(f=" << freq << ", chan=" << chan << ")");
    _desired_rf_freq[RX_DIRECTION]=freq;
    std::lock_guard<std::mutex> l(_set_lock);
    // We need to set the switches on both channels, because they share an LO.
    // This way, if we tune channel 0 it will not put channel 1 into a bad
    // state.
    _update_rx_freq_switches(freq, _rx_bypass_lnas, magnesium_cpld_ctrl::BOTH);
    const std::string ad9371_source = this->get_rx_lo_source(MAGNESIUM_LO1, chan);
    const std::string adf4351_source = this->get_rx_lo_source(MAGNESIUM_LO2, chan);
    UHD_ASSERT_THROW(adf4351_source == "internal");
    double coerced_if_freq = freq;

    if (_map_freq_to_rx_band(freq) == rx_band::LOWBAND) {
        _is_low_band[RX_DIRECTION] = true;
        const double desired_low_freq = MAGNESIUM_RX_IF_FREQ - freq;
        coerced_if_freq =
             this->_set_rx_lo_freq(adf4351_source, MAGNESIUM_LO2, desired_low_freq, chan) + freq;
        UHD_LOG_TRACE(unique_id(), "coerced_if_freq = " << coerced_if_freq);
    } else {
        _is_low_band[RX_DIRECTION] = false;
        _lo_disable(_rx_lo);
    }
    // external LO required to tune at 2xdesired_frequency.
    const double desired_if_freq = ad9371_source == "internal" ?
        coerced_if_freq :
        2*coerced_if_freq;

    this->_set_rx_lo_freq(ad9371_source, MAGNESIUM_LO1, desired_if_freq, chan);

    this->_update_freq(chan, RX_DIRECTION);
    this->_update_gain(chan, RX_DIRECTION);

    return radio_ctrl_impl::get_rx_frequency(chan);
}

double magnesium_radio_ctrl_impl::get_tx_frequency(
    const size_t chan)
{
    UHD_LOG_TRACE(unique_id(),
                  "get_tx_frequency(chan=" << chan << ")");
    return radio_ctrl_impl::get_tx_frequency(chan);
}

double magnesium_radio_ctrl_impl::get_rx_frequency(
    const size_t chan)
{
    UHD_LOG_TRACE(unique_id(),
                  "get_rx_frequency(chan=" << chan << ")");
    return radio_ctrl_impl::get_rx_frequency(chan);
}
double magnesium_radio_ctrl_impl::set_rx_bandwidth(
        const double bandwidth,
        const size_t chan
) {
    std::lock_guard<std::mutex> l(_set_lock);
    _ad9371->set_bandwidth(bandwidth, chan, RX_DIRECTION);
    // FIXME: setting analog bandwidth on AD9371 take no effect.
    // Remove this warning when ADI can confirm that it works.
    UHD_LOG_WARNING(unique_id(),
        "set_rx_bandwidth take no effect on AD9371. "
        "Default analog bandwidth is 100MHz");
    return AD9371_RX_MAX_BANDWIDTH;
}

double magnesium_radio_ctrl_impl::set_tx_bandwidth(
        const double bandwidth,
        const size_t chan
) {
    std::lock_guard<std::mutex> l(_set_lock);
    _ad9371->set_bandwidth(bandwidth, chan, TX_DIRECTION);
    // FIXME: setting analog bandwidth on AD9371 take no effect.
    // Remove this warning when ADI can confirm that it works.
    UHD_LOG_WARNING(unique_id(),
        "set_tx_bandwidth take no effect on AD9371. "
        "Default analog bandwidth is 100MHz");
    return AD9371_TX_MAX_BANDWIDTH ;
}

double magnesium_radio_ctrl_impl::set_tx_gain(
        const double gain,
        const size_t chan
) {
    std::lock_guard<std::mutex> l(_set_lock);
    UHD_LOG_TRACE(unique_id(),
        "set_tx_gain(gain=" << gain << ", chan=" << chan << ")");
    const double coerced_gain = _set_all_gain(
        gain,
        this->get_tx_frequency(chan),
        chan,
        TX_DIRECTION
    );
    radio_ctrl_impl::set_tx_gain(coerced_gain, chan);
    return coerced_gain;
}

double magnesium_radio_ctrl_impl::_set_tx_gain(
        const std::string &name,
        const double gain,
        const size_t chan
) {
    std::lock_guard<std::mutex> l(_set_lock);
    UHD_LOG_TRACE(unique_id(),
        "_set_tx_gain(name=" << name << ", gain=" << gain << ", chan=" << chan << ")");
      UHD_LOG_TRACE(unique_id(),
        "_set_tx_gain(name=" << name << ", gain=" << gain << ", chan=" << chan << ")");
    double clip_gain = 0;
    if (name == MAGNESIUM_GAIN1){
        clip_gain = uhd::clip(gain, AD9371_MIN_TX_GAIN, AD9371_MAX_TX_GAIN);
        _ad9371_att[TX_DIRECTION] = clip_gain;
    }else if (name == MAGNESIUM_GAIN2){
        clip_gain = uhd::clip(gain, DSA_MIN_GAIN, DSA_MAX_GAIN);
        _dsa_att[TX_DIRECTION] = clip_gain;
    }else if (name == MAGNESIUM_AMP){
        clip_gain = gain > 0.0 ? AMP_MAX_GAIN: AMP_MIN_GAIN;
        _amp_bypass[TX_DIRECTION] = clip_gain == 0.0;
    }else {
        throw uhd::value_error("Could not find gain element " + name);
    }
    UHD_LOG_TRACE(unique_id(),
        "_set_tx_gain calling update gain");
    this->_set_all_gain(
        this->_get_all_gain(chan, TX_DIRECTION),
        this->get_tx_frequency(chan),
        chan,
        TX_DIRECTION
    );
    return clip_gain;
}

double magnesium_radio_ctrl_impl::_get_tx_gain(
        const std::string &name,
        const size_t /*chan*/
) {
    std::lock_guard<std::mutex> l(_set_lock);
    if (name == MAGNESIUM_GAIN1){
        return _ad9371_att[TX_DIRECTION];
    }else if (name == MAGNESIUM_GAIN2){
        return _dsa_att[TX_DIRECTION];
    }else if (name == MAGNESIUM_AMP){
        return _amp_bypass[TX_DIRECTION]? AMP_MIN_GAIN : AMP_MAX_GAIN;
    }else {
        throw uhd::value_error("Could not find gain element " + name);
    }
}

double magnesium_radio_ctrl_impl::set_rx_gain(
        const double gain,
        const size_t chan
) {
    std::lock_guard<std::mutex> l(_set_lock);
    UHD_LOG_TRACE(unique_id(),
        "set_rx_gain(gain=" << gain << ", chan=" << chan << ")");
    const double coerced_gain = _set_all_gain(
        gain,
        this->get_rx_frequency(chan),
        chan,
        RX_DIRECTION
    );
    radio_ctrl_impl::set_rx_gain(coerced_gain, chan);
    return coerced_gain;
}

double magnesium_radio_ctrl_impl::_set_rx_gain(
        const std::string &name,
        const double gain,
        const size_t chan
) {
    std::lock_guard<std::mutex> l(_set_lock);
    UHD_LOG_TRACE(unique_id(),
        "_set_rx_gain(name=" << name << ", gain=" << gain << ", chan=" << chan << ")");
    double clip_gain = 0;
    if (name == MAGNESIUM_GAIN1){
        clip_gain = uhd::clip(gain, AD9371_MIN_RX_GAIN, AD9371_MAX_RX_GAIN);
        _ad9371_att[RX_DIRECTION] = clip_gain;
    }else if (name == MAGNESIUM_GAIN2){
        clip_gain = uhd::clip(gain, DSA_MIN_GAIN, DSA_MAX_GAIN);
        _dsa_att[RX_DIRECTION] = clip_gain;
    }else if (name == MAGNESIUM_AMP){
        clip_gain = gain > 0.0 ? AMP_MAX_GAIN: AMP_MIN_GAIN;
        _amp_bypass[RX_DIRECTION] = clip_gain == 0.0;
    }else {
        throw uhd::value_error("Could not find gain element " + name);
    }
    UHD_LOG_TRACE(unique_id(),
        "_set_rx_gain calling update gain");
    this->_set_all_gain(
        this->_get_all_gain(chan, RX_DIRECTION),
        this->get_rx_frequency(chan),
        chan,
        RX_DIRECTION
    );
    return clip_gain; // not really any coreced here (only clip) for individual gain
}

double magnesium_radio_ctrl_impl::_get_rx_gain(
        const std::string &name,
        const size_t /*chan*/
) {
    std::lock_guard<std::mutex> l(_set_lock);

    if (name == MAGNESIUM_GAIN1){
        return _ad9371_att[RX_DIRECTION];
    }else if (name == MAGNESIUM_GAIN2){
        return _dsa_att[RX_DIRECTION];
    }else if (name == MAGNESIUM_AMP){
        return _amp_bypass[RX_DIRECTION]? AMP_MIN_GAIN : AMP_MAX_GAIN;
    }else{
        throw uhd::value_error("Could not find gain element " + name);
    }
}

std::vector<std::string> magnesium_radio_ctrl_impl::get_rx_lo_names(
        const size_t /*chan*/
) {
    return std::vector<std::string>  {MAGNESIUM_LO1, MAGNESIUM_LO2};
}

std::vector<std::string> magnesium_radio_ctrl_impl::get_rx_lo_sources(
        const std::string &name,
        const size_t /*chan*/
) {
   if (name == MAGNESIUM_LO2){
       return std::vector<std::string> { "internal" };
   }else if (name == MAGNESIUM_LO1){
       return std::vector<std::string> { "internal", "external" };
   }else {
        throw uhd::value_error("Could not find LO stage " + name);
   }
}

freq_range_t magnesium_radio_ctrl_impl::get_rx_lo_freq_range(
        const std::string & name,
        const size_t  /*chan*/
) {
    if (name == MAGNESIUM_LO1){
        return freq_range_t{ADF4351_MIN_FREQ, ADF4351_MAX_FREQ};
    }
    else if(name == MAGNESIUM_LO2){
        return freq_range_t{AD9371_MIN_FREQ, AD9371_MAX_FREQ};
    }
    else {
        throw uhd::value_error("Could not find LO stage " + name);
    }
}

void magnesium_radio_ctrl_impl::set_rx_lo_source(
        const std::string &src,
        const std::string &name,
        const size_t /*chan*/
) {
    //TODO: checking what options are there
    std::lock_guard<std::mutex> l(_set_lock);
    UHD_LOG_TRACE(unique_id(), "Setting RX LO " << name << " to " << src);

    if (name == MAGNESIUM_LO1) {
        _ad9371->set_lo_source(src, RX_DIRECTION);
    } else {
        UHD_LOG_ERROR(unique_id(),
           "RX LO " << name << " does not support setting source to " << src);
    }
}

const std::string magnesium_radio_ctrl_impl::get_rx_lo_source(
        const std::string &name,
        const size_t /*chan*/
) {
    if (name == MAGNESIUM_LO1){
        //TODO: should we use this from cache?
        return _ad9371->get_lo_source(RX_DIRECTION);
    }
    return "internal";
}

double magnesium_radio_ctrl_impl::_set_rx_lo_freq(
    const std::string source,
    const std::string name,
    const double freq,
    const size_t chan
){
    double coerced_lo_freq = freq;
    if (source != "internal"){
        UHD_LOG_WARNING(unique_id(), "LO source is not internal. This set frequency will be ignored");
        if(name == MAGNESIUM_LO1){
            // handle ad9371 external LO case
            coerced_lo_freq = freq;
            _ad9371_freq[RX_DIRECTION] = coerced_lo_freq;
        }
    }else {
        if(name == MAGNESIUM_LO1){
            coerced_lo_freq = _ad9371->set_frequency(freq, chan, RX_DIRECTION);
            _ad9371_freq[RX_DIRECTION] = coerced_lo_freq;
        }else if (name == MAGNESIUM_LO2 ){
            // TODO: no hardcode the init_n_mode
             coerced_lo_freq = _lo_enable(_rx_lo, freq, _master_clock_rate, false);
            _adf4351_freq[RX_DIRECTION] = coerced_lo_freq;
        }else {
            UHD_LOG_WARNING(unique_id(), "There's no LO with this name of "<<name << " in the system. This set rx lo freq will be ignored");
        };
    }
    return coerced_lo_freq;
}

double magnesium_radio_ctrl_impl::set_rx_lo_freq(
        double freq,
        const std::string &name,
        const size_t chan
) {
    UHD_LOG_TRACE(unique_id(), "Setting rx lo frequency for " <<name << " with freq = " <<freq);
    std::lock_guard<std::mutex> l(_set_lock);
    std::string source = this->get_rx_lo_source(name, chan);
    const double coerced_lo_freq = this->_set_rx_lo_freq(source, name, freq, chan);
    this->_update_freq(chan,RX_DIRECTION);
    this->_update_gain(chan,RX_DIRECTION);
    return coerced_lo_freq;
}

double magnesium_radio_ctrl_impl::get_rx_lo_freq(
        const std::string & name,
        const size_t chan
) {

    UHD_LOG_TRACE(unique_id(),"Getting rx lo frequency for " <<name);
    std::string source = this->get_rx_lo_source(name,chan);
    if(name == MAGNESIUM_LO1){
        return _ad9371_freq[RX_DIRECTION];
    }else if (name == "adf4531" ){
        return _adf4351_freq[RX_DIRECTION];
    }else {
            UHD_LOG_ERROR(unique_id(), "There's no LO with this name of "<<name << " in the system. This set rx lo freq will be ignored");
    }
    UHD_THROW_INVALID_CODE_PATH();
}

//TX LO
std::vector<std::string> magnesium_radio_ctrl_impl::get_tx_lo_names(
        const size_t /*chan*/
) {
    return std::vector<std::string>  {MAGNESIUM_LO1, MAGNESIUM_LO2};
}

std::vector<std::string> magnesium_radio_ctrl_impl::get_tx_lo_sources(
        const std::string &name,
        const size_t /*chan*/
) {
   if (name == MAGNESIUM_LO2){
       return std::vector<std::string> { "internal" };
   }else if (name == MAGNESIUM_LO1){
       return std::vector<std::string> { "internal", "external" };
   }else {
        throw uhd::value_error("Could not find LO stage " + name);
   }
}

freq_range_t magnesium_radio_ctrl_impl::get_tx_lo_freq_range(
        const std::string &name,
        const size_t /*chan*/
) {
    if (name == MAGNESIUM_LO2){
        return freq_range_t{ADF4351_MIN_FREQ, ADF4351_MAX_FREQ};
    }
    else if(name == MAGNESIUM_LO1){
        return freq_range_t{AD9371_MIN_FREQ, AD9371_MAX_FREQ};
    }
    else {
        throw uhd::value_error("Could not find LO stage " + name);
    }
}

void magnesium_radio_ctrl_impl::set_tx_lo_source(
        const std::string &src,
        const std::string &name,
        const size_t /*chan*/
) {
    //TODO: checking what options are there
    std::lock_guard<std::mutex> l(_set_lock);
    UHD_LOG_TRACE(unique_id(), "Setting TX LO " << name << " to " << src);
    if (name == MAGNESIUM_LO1) {
        _ad9371->set_lo_source(src, TX_DIRECTION);
    } else {
        UHD_LOG_ERROR(unique_id(),
           "TX LO " << name << " does not support setting source to " << src);
    }
}

const std::string magnesium_radio_ctrl_impl::get_tx_lo_source(
        const std::string &name,
        const size_t /*chan*/
) {
    if (name == MAGNESIUM_LO1){
        //TODO: should we use this from cache?
        return _ad9371->get_lo_source(TX_DIRECTION);
    }
    return "internal";
}

double magnesium_radio_ctrl_impl::_set_tx_lo_freq(
    const std::string source,
    const std::string name,
    const double freq,
    const size_t chan
){
    double coerced_lo_freq = freq;
    if (source != "internal"){
        UHD_LOG_WARNING(unique_id(), "LO source is not internal. This set frequency will be ignored");
        if(name == MAGNESIUM_LO1){
            // handle ad9371 external LO case
            coerced_lo_freq = freq;
            _ad9371_freq[TX_DIRECTION] = coerced_lo_freq;
        }
    }else {
        if(name == MAGNESIUM_LO1){
            coerced_lo_freq = _ad9371->set_frequency(freq, chan, TX_DIRECTION);
            _ad9371_freq[TX_DIRECTION] = coerced_lo_freq;
        }else if (name == MAGNESIUM_LO2 ){
            // TODO: no hardcode the int_n_mode
            const bool int_n_mode = false;
            coerced_lo_freq = _lo_enable(_tx_lo, freq, _master_clock_rate, int_n_mode);
            _adf4351_freq[TX_DIRECTION] = coerced_lo_freq;
        }else {
            UHD_LOG_WARNING(unique_id(), "There's no LO with this name of "<<name << " in the system. This set tx lo freq will be ignored");
        };
    }
    return coerced_lo_freq;
}

double magnesium_radio_ctrl_impl::set_tx_lo_freq(
        double freq,
        const std::string &name,
        const size_t chan
) {
    UHD_LOG_TRACE(unique_id(), "Setting tx lo frequency for " <<name << " with freq = " <<freq);
    std::string source = this->get_tx_lo_source(name,chan);
    const double return_freq = this->_set_tx_lo_freq(source, name, freq, chan);
    this->_update_freq(chan, TX_DIRECTION);
    this->_update_gain(chan, TX_DIRECTION);
    return return_freq;
}

double magnesium_radio_ctrl_impl::get_tx_lo_freq(
        const std::string & name,
        const size_t chan
) {
    UHD_LOG_TRACE(unique_id(),"Getting tx lo frequency for " <<name);
    std::string source = this->get_tx_lo_source(name,chan);
    if(name == MAGNESIUM_LO1){
        return _ad9371_freq[TX_DIRECTION];
    }else if (name == MAGNESIUM_LO2){
        return _adf4351_freq[TX_DIRECTION];
    }else {
        UHD_LOG_ERROR(unique_id(), "There's no LO with this name of "<<name << " in the system.");
    };

    UHD_THROW_INVALID_CODE_PATH();
}



size_t magnesium_radio_ctrl_impl::get_chan_from_dboard_fe(
    const std::string &fe, const direction_t /* dir */
) {
    return boost::lexical_cast<size_t>(fe);
}

std::string magnesium_radio_ctrl_impl::get_dboard_fe_from_chan(
    const size_t chan,
    const direction_t /* dir */
) {
    return std::to_string(chan);
}


void magnesium_radio_ctrl_impl::set_rpc_client(
    uhd::rpc_client::sptr rpcc,
    const uhd::device_addr_t &block_args
) {
    _rpcc = rpcc;
    _block_args = block_args;
    UHD_LOG_TRACE(unique_id(), "Instantiating AD9371 control object...");
    _ad9371 = magnesium_ad9371_iface::uptr(
        new magnesium_ad9371_iface(
            _rpcc,
            (_radio_slot == "A") ? 0 : 1
        )
    );

    if (block_args.has_key("identify")) {
        const std::string identify_val = block_args.get("identify");
        int identify_duration = std::atoi(identify_val.c_str());
        if (identify_duration == 0) {
            identify_duration = 5;
        }
        UHD_LOG_INFO(unique_id(),
            "Running LED identification process for " << identify_duration
            << " seconds.");
        _identify_with_leds(identify_duration);
    }

    // Note: MCR gets set during the init() call (prior to this), which takes
    // in arguments from the device args. So if block_args contains a
    // master_clock_rate key, then it should better be whatever the device is
    // configured to do.
    _master_clock_rate = _rpcc->request_with_token<double>(
            _rpc_prefix + "get_master_clock_rate");
    if (block_args.cast<double>("master_clock_rate", _master_clock_rate)
            != _master_clock_rate) {
        throw uhd::runtime_error(str(
            boost::format("Master clock rate mismatch. Device returns %f MHz, "
                          "but should have been %f MHz.")
            % (_master_clock_rate / 1e6)
            % (block_args.cast<double>(
                    "master_clock_rate", _master_clock_rate) / 1e6)
        ));
    }
    UHD_LOG_DEBUG(unique_id(),
        "Master Clock Rate is: " << (_master_clock_rate / 1e6) << " MHz.");
    radio_ctrl_impl::set_rate(_master_clock_rate);

    // EEPROM paths subject to change FIXME
    const size_t db_idx = get_block_id().get_block_count();
    _tree->access<eeprom_map_t>(_root_path / "eeprom")
        .add_coerced_subscriber([this, db_idx](const eeprom_map_t& db_eeprom){
            this->_rpcc->notify_with_token("set_db_eeprom", db_idx, db_eeprom);
        })
        .set_publisher([this, db_idx](){
            return this->_rpcc->request_with_token<eeprom_map_t>(
                "get_db_eeprom", db_idx
            );
        })
    ;

    // Init sensors
    for (const auto &dir : std::vector<direction_t>{RX_DIRECTION, TX_DIRECTION}) {
        for (size_t chan_idx = 0; chan_idx < MAGNESIUM_NUM_CHANS; chan_idx++) {
            _init_mpm_sensors(dir, chan_idx);
        }
    }
}

bool magnesium_radio_ctrl_impl::get_lo_lock_status(
    const direction_t dir
) {
    if (not (bool(_rpcc))) {
        UHD_LOG_DEBUG(unique_id(),
            "Reported no LO lock due to lack of RPC connection.");
        return false;
    }

    const std::string trx = (dir == RX_DIRECTION) ? "rx" : "tx";
    const size_t chan = 0; // They're the same after all
    const double freq = (dir == RX_DIRECTION) ?
        get_rx_frequency(chan) :
        get_tx_frequency(chan);

    bool lo_lock = _rpcc->request_with_token<bool>(
        _rpc_prefix + "get_ad9371_lo_lock", trx);
    UHD_LOG_TRACE(unique_id(),
        "AD9371 " << trx << " LO reports lock: " << (lo_lock ? "Yes" : "No"));
    if (lo_lock and _map_freq_to_rx_band(freq) == rx_band::LOWBAND) {
        lo_lock = lo_lock && _rpcc->request_with_token<bool>(
            _rpc_prefix + "get_lowband_lo_lock", trx);
        UHD_LOG_TRACE(unique_id(),
            "ADF4351 " << trx << " LO reports lock: "
            << (lo_lock ? "Yes" : "No"));
    }

    return lo_lock;
}

UHD_RFNOC_BLOCK_REGISTER(magnesium_radio_ctrl, "MagnesiumRadio");
