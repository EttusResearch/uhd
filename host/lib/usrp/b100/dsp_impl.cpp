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

#include "b100_impl.hpp"
#include "b100_regs.hpp"
#include <uhd/usrp/dsp_utils.hpp>
#include <uhd/usrp/dsp_props.hpp>
#include <boost/math/special_functions/round.hpp>
#include <boost/bind.hpp>

#define rint boost::math::iround

using namespace uhd;
using namespace uhd::usrp;

static const double MASTER_CLOCK_RATE = 64e6; //TODO get from clock control

/***********************************************************************
 * RX DDC Initialization
 **********************************************************************/
void b100_impl::rx_ddc_init(void){
    _rx_ddc_proxy = wax_obj_proxy::make(
        boost::bind(&b100_impl::rx_ddc_get, this, _1, _2),
        boost::bind(&b100_impl::rx_ddc_set, this, _1, _2)
    );

    //initial config and update
    rx_ddc_set(DSP_PROP_FREQ_SHIFT, double(0));
    rx_ddc_set(DSP_PROP_HOST_RATE, double(16e6));
}

/***********************************************************************
 * RX DDC Get
 **********************************************************************/
void b100_impl::rx_ddc_get(const wax::obj &key_, wax::obj &val){
    named_prop_t key = named_prop_t::extract(key_);

    switch(key.as<dsp_prop_t>()){
    case DSP_PROP_NAME:
        val = std::string("USRP-B100 RX DSP");
        return;

    case DSP_PROP_OTHERS:
        val = prop_names_t(); //empty
        return;

    case DSP_PROP_FREQ_SHIFT:
        val = _ddc_freq;
        return;

    case DSP_PROP_CODEC_RATE:
        val = _clock_ctrl->get_fpga_clock_rate();
        return;

    case DSP_PROP_HOST_RATE:
        val = _clock_ctrl->get_fpga_clock_rate()/_ddc_decim;
        return;

    default: UHD_THROW_PROP_GET_ERROR();
    }
}

/***********************************************************************
 * RX DDC Set
 **********************************************************************/
void b100_impl::rx_ddc_set(const wax::obj &key_, const wax::obj &val){
    named_prop_t key = named_prop_t::extract(key_);

    switch(key.as<dsp_prop_t>()){
    case DSP_PROP_STREAM_CMD:
        issue_stream_cmd(val.as<stream_cmd_t>());
        return;

    case DSP_PROP_FREQ_SHIFT:{
            double new_freq = val.as<double>();
            _iface->poke32(B100_REG_DSP_RX_FREQ,
                dsp_type1::calc_cordic_word_and_update(new_freq, _clock_ctrl->get_fpga_clock_rate())
            );
            _ddc_freq = new_freq; //shadow
        }
        return;

    case DSP_PROP_HOST_RATE:{
            //set the decimation
            _ddc_decim = rint(_clock_ctrl->get_fpga_clock_rate()/val.as<double>());
            _iface->poke32(B100_REG_DSP_RX_DECIM_RATE, dsp_type1::calc_cic_filter_word(_ddc_decim));

            //set the scaling
            static const boost::int16_t default_rx_scale_iq = 1024;
            _iface->poke32(B100_REG_DSP_RX_SCALE_IQ,
                dsp_type1::calc_iq_scale_word(default_rx_scale_iq, default_rx_scale_iq)
            );
        }
        return;

    default: UHD_THROW_PROP_SET_ERROR();
    }
}

/***********************************************************************
 * TX DUC Initialization
 **********************************************************************/
void b100_impl::tx_duc_init(void){
    _tx_duc_proxy = wax_obj_proxy::make(
        boost::bind(&b100_impl::tx_duc_get, this, _1, _2),
        boost::bind(&b100_impl::tx_duc_set, this, _1, _2)
    );

    //initial config and update
    tx_duc_set(DSP_PROP_FREQ_SHIFT, double(0));
    tx_duc_set(DSP_PROP_HOST_RATE, double(16e6));
}

/***********************************************************************
 * TX DUC Get
 **********************************************************************/
void b100_impl::tx_duc_get(const wax::obj &key_, wax::obj &val){
    named_prop_t key = named_prop_t::extract(key_);

    switch(key.as<dsp_prop_t>()){
    case DSP_PROP_NAME:
        val = std::string("USRP-B100 TX DSP");
        return;

    case DSP_PROP_OTHERS:
        val = prop_names_t(); //empty
        return;

    case DSP_PROP_FREQ_SHIFT:
        val = _duc_freq;
        return;

    case DSP_PROP_CODEC_RATE:
        val = _clock_ctrl->get_fpga_clock_rate();
        return;

    case DSP_PROP_HOST_RATE:
        val = _clock_ctrl->get_fpga_clock_rate()/_duc_interp;
        return;

    default: UHD_THROW_PROP_GET_ERROR();
    }
}

/***********************************************************************
 * TX DUC Set
 **********************************************************************/
void b100_impl::tx_duc_set(const wax::obj &key_, const wax::obj &val){
    named_prop_t key = named_prop_t::extract(key_);

    switch(key.as<dsp_prop_t>()){

    case DSP_PROP_FREQ_SHIFT:{
            double new_freq = val.as<double>();
            _iface->poke32(B100_REG_DSP_TX_FREQ,
                dsp_type1::calc_cordic_word_and_update(new_freq, _clock_ctrl->get_fpga_clock_rate())
            );
            _duc_freq = new_freq; //shadow
        }
        return;

    case DSP_PROP_HOST_RATE:{
            _duc_interp = rint(_clock_ctrl->get_fpga_clock_rate()/val.as<double>());

            //set the interpolation
            _iface->poke32(B100_REG_DSP_TX_INTERP_RATE, dsp_type1::calc_cic_filter_word(_duc_interp));

            //set the scaling
            _iface->poke32(B100_REG_DSP_TX_SCALE_IQ, dsp_type1::calc_iq_scale_word(_duc_interp));
        }
        return;

    default: UHD_THROW_PROP_SET_ERROR();
    }
}
