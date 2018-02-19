//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "magnesium_radio_ctrl_impl.hpp"
#include "magnesium_gain_table.hpp"
#include "magnesium_constants.hpp"
#include <uhd/utils/log.hpp>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::rfnoc;
using namespace magnesium;

double magnesium_radio_ctrl_impl::_set_all_gain(
    const double gain,
    const double freq,
    const size_t chan,
    const direction_t dir
) {
    UHD_LOG_TRACE(unique_id(),
        __func__ << "(gain=" << gain << "dB, "
        "freq=" << freq << " Hz, "
        "chan=" << chan << ", "
        "dir=" << dir);
    const size_t ad9371_chan = _master ? 0 : 1; ;// FIXME: use chan when 2 radios
    const magnesium_cpld_ctrl::chan_sel_t chan_sel  =
        _master ? magnesium_cpld_ctrl::CHAN1 : magnesium_cpld_ctrl::CHAN2;
    gain_tuple_t gain_tuple = (dir == RX_DIRECTION) ?
            get_rx_gain_tuple(gain, _map_freq_to_rx_band(freq)):
            get_tx_gain_tuple(gain, _map_freq_to_tx_band(freq));

    if (_gain_profile[dir] == "manual"){
        UHD_LOG_TRACE(unique_id(), "Manual gain mode. Getting gain from property tree.");
        gain_tuple = {
            DSA_MAX_GAIN - _dsa_att[dir],
            ((dir == RX_DIRECTION) ? AD9371_MAX_RX_GAIN : AD9371_MAX_TX_GAIN) - _ad9371_att[dir],
            _amp_bypass[dir]};
    }else if (_gain_profile[dir] == "default"){
        UHD_LOG_TRACE(unique_id(), "Getting gain from gain table.");
    }else {
     UHD_LOG_ERROR(unique_id(), "Unsupported gain mode: " << _gain_profile[dir])
    }
    const double ad9371_gain =
        ((dir == RX_DIRECTION) ?  AD9371_MAX_RX_GAIN : AD9371_MAX_TX_GAIN)
        - gain_tuple.ad9371_att;
    UHD_LOG_TRACE(unique_id(),
        "AD9371 attenuation==" << gain_tuple.ad9371_att << " dB, "
        "AD9371 gain==" << ad9371_gain << " dB, "
        "DSA attenuation == " << gain_tuple.dsa_att << " dB."
    );
    _ad9371->set_gain(ad9371_gain, ad9371_chan, dir);
    _dsa_set_att(gain_tuple.dsa_att, chan, dir);
    if (dir == RX_DIRECTION or dir == DX_DIRECTION) {
        _all_rx_gain = gain;
        _rx_bypass_lnas = gain_tuple.bypass;
        _update_rx_freq_switches(
            this->get_rx_frequency(chan),
            _rx_bypass_lnas,
            chan_sel
        );
    }
    if (dir == TX_DIRECTION or dir == DX_DIRECTION) {
        _all_tx_gain = gain;
        _tx_bypass_amp = gain_tuple.bypass;
        _update_tx_freq_switches(
             this->get_tx_frequency(chan),
            _tx_bypass_amp,
            chan_sel
        );
    }

    return gain;
}

double magnesium_radio_ctrl_impl::_get_all_gain(
    const size_t /* chan */,
    const direction_t dir
) {
    UHD_LOG_TRACE(unique_id(), "Getting all gain ");
    if (dir == RX_DIRECTION) {
       return _all_rx_gain;
    }
    return _all_tx_gain;
}

/******************************************************************************
 * DSA Controls
 *****************************************************************************/
double magnesium_radio_ctrl_impl::_dsa_set_att(
    const double att,
    const size_t chan,
    const direction_t dir
) {
    UHD_LOG_TRACE(unique_id(),
        __func__ <<
        "(att=" << "att dB, chan=" << chan << ", dir=" << dir << ")")
    const uint32_t dsa_val = 2*att;

    _set_dsa_val(chan, dir, dsa_val);
    if (dir == RX_DIRECTION or dir == DX_DIRECTION) {
        _dsa_rx_att = att;
    }
    if (dir == TX_DIRECTION or dir == DX_DIRECTION) {
        _dsa_tx_att = att;
    }
    return att;
}

double magnesium_radio_ctrl_impl::_dsa_get_att(
    const size_t /*chan*/,
    const direction_t dir
) {
    if (dir == RX_DIRECTION) {
       return _dsa_rx_att;
    }
    return _dsa_tx_att;
}

void magnesium_radio_ctrl_impl::_set_dsa_val(
    const size_t chan,
    const direction_t dir,
    const uint32_t dsa_val
) {
    // The DSA register holds 12 bits. The lower 6 bits are for RX, the upper
    // 6 bits are for TX.
    if (dir == RX_DIRECTION or dir == DX_DIRECTION){
        UHD_LOG_TRACE(unique_id(),
            __func__ << "(chan=" << chan << ", dir=RX"
            << ", dsa_val=" << dsa_val << ")")
        _gpio[chan]->set_gpio_out(dsa_val, 0x003F);
    }
    if (dir == TX_DIRECTION or dir == DX_DIRECTION){
        UHD_LOG_TRACE(unique_id(),
            __func__ << "(chan=" << chan << ", dir=TX"
            << ", dsa_val=" << dsa_val << ")")
        _gpio[chan]->set_gpio_out(dsa_val<<6, 0x0FC0);
    }
}

