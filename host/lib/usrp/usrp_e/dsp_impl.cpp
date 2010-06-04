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

#include "usrp_e_impl.hpp"
#include "usrp_e_regs.hpp"
#include <uhd/usrp/dsp_props.hpp>
#include <uhd/utils/assert.hpp>
#include <boost/bind.hpp>
#include <boost/math/special_functions/round.hpp>

#define rint boost::math::iround

using namespace uhd;
using namespace uhd::usrp;

/***********************************************************************
 * Helper Functions
 **********************************************************************/
static boost::uint32_t calculate_freq_word_and_update_actual_freq(double &freq, double clock_freq){
    UHD_ASSERT_THROW(std::abs(freq) < clock_freq/2.0);
    static const double scale_factor = std::pow(2.0, 32);

    //calculate the freq register word
    boost::uint32_t freq_word = rint((freq / clock_freq) * scale_factor);

    //update the actual frequency
    freq = (double(freq_word) / scale_factor) * clock_freq;

    return freq_word;
}

// Check if requested decim/interp rate is:
//      multiple of 4, enable two halfband filters
//      multiple of 2, enable one halfband filter
//      handle remainder in CIC
static boost::uint32_t calculate_cic_word(size_t rate){
    int hb0 = 0, hb1 = 0;
    if (not (rate & 0x1)){
        hb0 = 1;
        rate /= 2;
    }
    if (not (rate & 0x1)){
        hb1 = 1;
        rate /= 2;
    }
    return (hb1 << 9) | (hb0 << 8) | (rate & 0xff);
}

static boost::uint32_t calculate_iq_scale_word(boost::int16_t i, boost::int16_t q){
    return (boost::uint16_t(i) << 16) | (boost::uint16_t(q) << 0);
}

/***********************************************************************
 * RX DDC Initialization
 **********************************************************************/
void usrp_e_impl::rx_ddc_init(void){
    _rx_ddc_proxy = wax_obj_proxy::make(
        boost::bind(&usrp_e_impl::rx_ddc_get, this, _1, _2),
        boost::bind(&usrp_e_impl::rx_ddc_set, this, _1, _2)
    );
}

/***********************************************************************
 * RX DDC Get
 **********************************************************************/
void usrp_e_impl::rx_ddc_get(const wax::obj &key, wax::obj &val){
    switch(key.as<dsp_prop_t>()){
    case DSP_PROP_NAME:
        val = std::string("usrp-e ddc0");
        return;

    case DSP_PROP_OTHERS:
        val = prop_names_t(); //empty
        return;

    case DSP_PROP_FREQ_SHIFT:
        val = _ddc_freq;
        return;

    case DSP_PROP_CODEC_RATE:
        val = MASTER_CLOCK_RATE;
        return;

    case DSP_PROP_HOST_RATE:
        val = MASTER_CLOCK_RATE/_ddc_decim;
        return;

    default: UHD_THROW_PROP_GET_ERROR();
    }
}

/***********************************************************************
 * RX DDC Set
 **********************************************************************/
void usrp_e_impl::rx_ddc_set(const wax::obj &key, const wax::obj &val){
    switch(key.as<dsp_prop_t>()){

    case DSP_PROP_FREQ_SHIFT:{
            double new_freq = val.as<double>();
            _iface->poke32(UE_REG_DSP_RX_FREQ,
                calculate_freq_word_and_update_actual_freq(new_freq, MASTER_CLOCK_RATE)
            );
            _ddc_freq = new_freq; //shadow
        }
        return;

    case DSP_PROP_HOST_RATE:{
            //set the decimation
            _ddc_decim = rint(MASTER_CLOCK_RATE/val.as<double>());
            _iface->poke32(UE_REG_DSP_RX_DECIM_RATE, calculate_cic_word(_ddc_decim));

            //set the scaling
            static const boost::int16_t default_rx_scale_iq = 1024;
            _iface->poke32(UE_REG_DSP_RX_SCALE_IQ,
                calculate_iq_scale_word(default_rx_scale_iq, default_rx_scale_iq)
            );
        }
        return;

    default: UHD_THROW_PROP_SET_ERROR();
    }
}

/***********************************************************************
 * TX DUC Initialization
 **********************************************************************/
void usrp_e_impl::tx_duc_init(void){
    _tx_duc_proxy = wax_obj_proxy::make(
        boost::bind(&usrp_e_impl::tx_duc_get, this, _1, _2),
        boost::bind(&usrp_e_impl::tx_duc_set, this, _1, _2)
    );
}

/***********************************************************************
 * TX DUC Get
 **********************************************************************/
void usrp_e_impl::tx_duc_get(const wax::obj &key, wax::obj &val){
    switch(key.as<dsp_prop_t>()){
    case DSP_PROP_NAME:
        val = std::string("usrp-e duc0");
        return;

    case DSP_PROP_OTHERS:
        val = prop_names_t(); //empty
        return;

    case DSP_PROP_FREQ_SHIFT:
        val = _duc_freq;
        return;

    case DSP_PROP_CODEC_RATE:
        val = MASTER_CLOCK_RATE;
        return;

    case DSP_PROP_HOST_RATE:
        val = MASTER_CLOCK_RATE/_duc_interp;
        return;

    default: UHD_THROW_PROP_GET_ERROR();
    }
}

/***********************************************************************
 * TX DUC Set
 **********************************************************************/
void usrp_e_impl::tx_duc_set(const wax::obj &key, const wax::obj &val){
    switch(key.as<dsp_prop_t>()){

    case DSP_PROP_FREQ_SHIFT:{
            double new_freq = val.as<double>();
            _iface->poke32(UE_REG_DSP_TX_FREQ,
                calculate_freq_word_and_update_actual_freq(new_freq, MASTER_CLOCK_RATE)
            );
            _duc_freq = new_freq; //shadow
        }
        return;

    case DSP_PROP_HOST_RATE:{
            _duc_interp = rint(MASTER_CLOCK_RATE/val.as<double>());

            // Calculate CIC interpolation (i.e., without halfband interpolators)
            size_t tmp_interp = calculate_cic_word(_duc_interp) & 0xff;

            // Calculate closest multiplier constant to reverse gain absent scale multipliers
            double interp_cubed = std::pow(double(tmp_interp), 3);
            boost::int16_t scale = rint((4096*std::pow(2, ceil(log2(interp_cubed))))/(1.65*interp_cubed));

            //set the interpolation
            _iface->poke32(UE_REG_DSP_TX_INTERP_RATE, calculate_cic_word(_duc_interp));

            //set the scaling
            _iface->poke32(UE_REG_DSP_TX_SCALE_IQ, calculate_iq_scale_word(scale, scale));
        }
        return;

    default: UHD_THROW_PROP_SET_ERROR();
    }
}
