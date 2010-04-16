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

#include "usrp2_impl.hpp"
#include "usrp2_regs.hpp"
#include <uhd/usrp/dsp_props.hpp>
#include <uhd/utils/assert.hpp>
#include <boost/format.hpp>
#include <boost/bind.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/math/special_functions/round.hpp>

using namespace uhd;
using namespace uhd::usrp;

static const size_t default_decim = 16;
static const size_t default_interp = 16;

#define rint boost::math::iround

template <class T> T log2(T num){
    return std::log(num)/std::log(T(2));
}

/***********************************************************************
 * DDC Helper Methods
 **********************************************************************/
static boost::uint32_t calculate_freq_word_and_update_actual_freq(double &freq, double clock_freq){
    ASSERT_THROW(std::abs(freq) < clock_freq/2.0);
    static const double scale_factor = std::pow(2.0, 32);

    //calculate the freq register word
    boost::uint32_t freq_word = rint((freq / clock_freq) * scale_factor);

    //update the actual frequency
    freq = (double(freq_word) / scale_factor) * clock_freq;

    return freq_word;
}

static boost::uint32_t calculate_iq_scale_word(boost::int16_t i, boost::int16_t q){
    return (boost::uint16_t(i) << 16) | (boost::uint16_t(q) << 0);
}

template <class rate_t> static rate_t
pick_closest_rate(double exact_rate, const std::vector<rate_t> &rates){
    rate_t closest_match = rates.at(0);
    BOOST_FOREACH(rate_t possible_rate, rates){
        if(std::abs(exact_rate - possible_rate) < std::abs(exact_rate - closest_match))
            closest_match = possible_rate;
    }
    return closest_match;
}

void usrp2_impl::init_ddc_config(void){
    //create the ddc in the rx dsp dict
    _rx_dsp_proxy = wax_obj_proxy::make(
        boost::bind(&usrp2_impl::ddc_get, this, _1, _2),
        boost::bind(&usrp2_impl::ddc_set, this, _1, _2)
    );

    //initial config and update
    _ddc_decim = default_decim;
    _ddc_freq = 0;
    update_ddc_config();

    //initial command that kills streaming (in case if was left on)
    issue_ddc_stream_cmd(stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
}

void usrp2_impl::update_ddc_config(void){
    //set the decimation
    _iface->poke32(FR_DSP_RX_DECIM_RATE, _ddc_decim);

    //set the scaling
    static const boost::int16_t default_rx_scale_iq = 1024;
    _iface->poke32(FR_DSP_RX_SCALE_IQ,
        calculate_iq_scale_word(default_rx_scale_iq, default_rx_scale_iq)
    );
}

/***********************************************************************
 * DDC Properties
 **********************************************************************/
void usrp2_impl::ddc_get(const wax::obj &key, wax::obj &val){
    switch(key.as<dsp_prop_t>()){
    case DSP_PROP_NAME:
        val = std::string("usrp2 ddc0");
        return;

    case DSP_PROP_OTHERS:
        val = prop_names_t(); //empty
        return;

    case DSP_PROP_FREQ_SHIFT:
        val = _ddc_freq;
        return;

    case DSP_PROP_CODEC_RATE:
        val = get_master_clock_freq();
        return;

    case DSP_PROP_HOST_RATE:
        val = get_master_clock_freq()/_ddc_decim;
        return;
    }
}

void usrp2_impl::ddc_set(const wax::obj &key, const wax::obj &val){
    switch(key.as<dsp_prop_t>()){

    case DSP_PROP_FREQ_SHIFT:{
            double new_freq = val.as<double>();
            _iface->poke32(FR_DSP_RX_FREQ,
                calculate_freq_word_and_update_actual_freq(new_freq, get_master_clock_freq())
            );
            _ddc_freq = new_freq; //shadow
        }
        return;

    case DSP_PROP_HOST_RATE:{
            double extact_rate = get_master_clock_freq()/val.as<double>();
            _ddc_decim = pick_closest_rate(extact_rate, _allowed_decim_and_interp_rates);
            update_ddc_config();
        }
        return;

    default:
        throw std::runtime_error("Error: trying to set read-only property on usrp2 ddc0");
    }
}

/***********************************************************************
 * DUC Helper Methods
 **********************************************************************/
void usrp2_impl::init_duc_config(void){
    //create the duc in the tx dsp dict
    _tx_dsp_proxy = wax_obj_proxy::make(
        boost::bind(&usrp2_impl::duc_get, this, _1, _2),
        boost::bind(&usrp2_impl::duc_set, this, _1, _2)
    );

    //initial config and update
    _duc_interp = default_interp;
    _duc_freq = 0;
    update_duc_config();
}

void usrp2_impl::update_duc_config(void){
    // Calculate CIC interpolation (i.e., without halfband interpolators)
    size_t tmp_interp = _duc_interp;
    while(tmp_interp > 128) tmp_interp /= 2;

    // Calculate closest multiplier constant to reverse gain absent scale multipliers
    double interp_cubed = std::pow(double(tmp_interp), 3);
    boost::int16_t scale = rint((4096*std::pow(2, ceil(log2(interp_cubed))))/(1.65*interp_cubed));

    //set the interpolation
    _iface->poke32(FR_DSP_TX_INTERP_RATE, _ddc_decim);

    //set the scaling
    _iface->poke32(FR_DSP_TX_SCALE_IQ, calculate_iq_scale_word(scale, scale));
}

/***********************************************************************
 * DUC Properties
 **********************************************************************/
void usrp2_impl::duc_get(const wax::obj &key, wax::obj &val){
    switch(key.as<dsp_prop_t>()){
    case DSP_PROP_NAME:
        val = std::string("usrp2 duc0");
        return;

    case DSP_PROP_OTHERS:
        val = prop_names_t(); //empty
        return;

    case DSP_PROP_FREQ_SHIFT:
        val = _duc_freq;
        return;

    case DSP_PROP_CODEC_RATE:
        val = get_master_clock_freq();
        return;

    case DSP_PROP_HOST_RATE:
        val = get_master_clock_freq()/_duc_interp;
        return;
    }
}

void usrp2_impl::duc_set(const wax::obj &key, const wax::obj &val){
    switch(key.as<dsp_prop_t>()){

    case DSP_PROP_FREQ_SHIFT:{
            double new_freq = val.as<double>();
            _iface->poke32(FR_DSP_TX_FREQ,
                calculate_freq_word_and_update_actual_freq(new_freq, get_master_clock_freq())
            );
            _duc_freq = new_freq; //shadow
        }
        return;

    case DSP_PROP_HOST_RATE:{
            double extact_rate = get_master_clock_freq()/val.as<double>();
            _duc_interp = pick_closest_rate(extact_rate, _allowed_decim_and_interp_rates);
            update_duc_config();
        }
        return;

    default:
        throw std::runtime_error("Error: trying to set read-only property on usrp2 duc0");
    }
}
