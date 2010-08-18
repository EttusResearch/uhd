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
#include <uhd/usrp/dsp_utils.hpp>
#include <uhd/usrp/dsp_props.hpp>
#include <boost/bind.hpp>
#include <cmath>

using namespace uhd;
using namespace uhd::usrp;

static const size_t default_decim = 16;
static const size_t default_interp = 16;

/***********************************************************************
 * DDC Helper Methods
 **********************************************************************/
template <typename rate_type>
static rate_type pick_closest_rate(double exact_rate, const std::vector<rate_type> &rates){
    unsigned closest_match = rates.front();
    BOOST_FOREACH(rate_type possible_rate, rates){
        if(std::abs(exact_rate - possible_rate) < std::abs(exact_rate - closest_match))
            closest_match = possible_rate;
    }
    return closest_match;
}

void usrp2_mboard_impl::init_ddc_config(void){
    //create the ddc in the rx dsp dict
    _rx_dsp_proxy = wax_obj_proxy::make(
        boost::bind(&usrp2_mboard_impl::ddc_get, this, _1, _2),
        boost::bind(&usrp2_mboard_impl::ddc_set, this, _1, _2)
    );

    //initial config and update
    ddc_set(DSP_PROP_FREQ_SHIFT, double(0));
    ddc_set(DSP_PROP_HOST_RATE, double(get_master_clock_freq()/default_decim));
}

/***********************************************************************
 * DDC Properties
 **********************************************************************/
void usrp2_mboard_impl::ddc_get(const wax::obj &key, wax::obj &val){
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

    default: UHD_THROW_PROP_GET_ERROR();
    }
}

void usrp2_mboard_impl::ddc_set(const wax::obj &key, const wax::obj &val){
    switch(key.as<dsp_prop_t>()){

    case DSP_PROP_FREQ_SHIFT:{
            double new_freq = val.as<double>();
            _iface->poke32(U2_REG_DSP_RX_FREQ,
                dsp_type1::calc_cordic_word_and_update(new_freq, get_master_clock_freq())
            );
            _ddc_freq = new_freq; //shadow
        }
        return;

    case DSP_PROP_HOST_RATE:{
            double extact_rate = get_master_clock_freq()/val.as<double>();
            _ddc_decim = pick_closest_rate(extact_rate, _allowed_decim_and_interp_rates);

            //set the decimation
            _iface->poke32(U2_REG_DSP_RX_DECIM_RATE, dsp_type1::calc_cic_filter_word(_ddc_decim));

            //set the scaling
            static const boost::int16_t default_rx_scale_iq = 1024;
            _iface->poke32(U2_REG_DSP_RX_SCALE_IQ,
                dsp_type1::calc_iq_scale_word(default_rx_scale_iq, default_rx_scale_iq)
            );
        }
        return;

    default: UHD_THROW_PROP_SET_ERROR();
    }
}

/***********************************************************************
 * DUC Helper Methods
 **********************************************************************/
void usrp2_mboard_impl::init_duc_config(void){
    //create the duc in the tx dsp dict
    _tx_dsp_proxy = wax_obj_proxy::make(
        boost::bind(&usrp2_mboard_impl::duc_get, this, _1, _2),
        boost::bind(&usrp2_mboard_impl::duc_set, this, _1, _2)
    );

    //initial config and update
    duc_set(DSP_PROP_FREQ_SHIFT, double(0));
    duc_set(DSP_PROP_HOST_RATE, double(get_master_clock_freq()/default_interp));
}

/***********************************************************************
 * DUC Properties
 **********************************************************************/
void usrp2_mboard_impl::duc_get(const wax::obj &key, wax::obj &val){
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

    default: UHD_THROW_PROP_GET_ERROR();
    }
}

void usrp2_mboard_impl::duc_set(const wax::obj &key, const wax::obj &val){
    switch(key.as<dsp_prop_t>()){

    case DSP_PROP_FREQ_SHIFT:{
            double new_freq = val.as<double>();
            _iface->poke32(U2_REG_DSP_TX_FREQ,
                dsp_type1::calc_cordic_word_and_update(new_freq, get_master_clock_freq())
            );
            _duc_freq = new_freq; //shadow
        }
        return;

    case DSP_PROP_HOST_RATE:{
            double extact_rate = get_master_clock_freq()/val.as<double>();
            _duc_interp = pick_closest_rate(extact_rate, _allowed_decim_and_interp_rates);

            //set the interpolation
            _iface->poke32(U2_REG_DSP_TX_INTERP_RATE, dsp_type1::calc_cic_filter_word(_duc_interp));

            //set the scaling
            _iface->poke32(U2_REG_DSP_TX_SCALE_IQ, dsp_type1::calc_iq_scale_word(_duc_interp));
        }
        return;

    default: UHD_THROW_PROP_SET_ERROR();
    }
}
