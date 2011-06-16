//
// Copyright 2010-2011 Ettus Research LLC
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

#include "usrp_e100_impl.hpp"
#include "usrp_e100_regs.hpp"
#include <uhd/usrp/dsp_utils.hpp>
#include <uhd/usrp/dsp_props.hpp>
#include <boost/math/special_functions/round.hpp>
#include <boost/bind.hpp>

using namespace uhd;
using namespace uhd::usrp;

/***********************************************************************
 * DSP impl and methods
 **********************************************************************/
struct usrp_e100_impl::dsp_impl{
    uhd::dict<size_t, size_t> ddc_decim;
    uhd::dict<size_t, double> ddc_freq;
    uhd::dict<size_t, size_t> duc_interp;
    uhd::dict<size_t, double> duc_freq;
};

/***********************************************************************
 * RX DDC Initialization
 **********************************************************************/
void usrp_e100_impl::dsp_init(void){
    //create new dsp impl
    _dsp_impl = UHD_PIMPL_MAKE(dsp_impl, ());

    //bind and initialize the rx dsps
    for (size_t i = 0; i < E100_NUM_RX_DSPS; i++){
        _rx_dsp_proxies[str(boost::format("DSP%d")%i)] = wax_obj_proxy::make(
            boost::bind(&usrp_e100_impl::ddc_get, this, _1, _2, i),
            boost::bind(&usrp_e100_impl::ddc_set, this, _1, _2, i)
        );

        //initial config and update
        ddc_set(DSP_PROP_FREQ_SHIFT, double(0), i);
        ddc_set(DSP_PROP_HOST_RATE, double(_clock_ctrl->get_fpga_clock_rate()/16), i);

        //setup the rx control registers
        _iface->poke32(UE_REG_RX_CTRL_CLEAR(i), 1); //reset
        _iface->poke32(UE_REG_RX_CTRL_NSAMPS_PP(i), this->get_max_recv_samps_per_packet());
        _iface->poke32(UE_REG_RX_CTRL_NCHANNELS(i), 1);
        _iface->poke32(UE_REG_RX_CTRL_VRT_HDR(i), 0
            | (0x1 << 28) //if data with stream id
            | (0x1 << 26) //has trailer
            | (0x3 << 22) //integer time other
            | (0x1 << 20) //fractional time sample count
        );
        _iface->poke32(UE_REG_RX_CTRL_VRT_SID(i), E100_DSP_SID_BASE + i);
        _iface->poke32(UE_REG_RX_CTRL_VRT_TLR(i), 0);
        _iface->poke32(UE_REG_TIME64_TPS, size_t(_clock_ctrl->get_fpga_clock_rate()));
    }

    //bind and initialize the tx dsps
    for (size_t i = 0; i < E100_NUM_TX_DSPS; i++){
        _tx_dsp_proxies[str(boost::format("DSP%d")%i)] = wax_obj_proxy::make(
            boost::bind(&usrp_e100_impl::duc_get, this, _1, _2, i),
            boost::bind(&usrp_e100_impl::duc_set, this, _1, _2, i)
        );

        //initial config and update
        duc_set(DSP_PROP_FREQ_SHIFT, double(0), i);
        duc_set(DSP_PROP_HOST_RATE, double(_clock_ctrl->get_fpga_clock_rate()/16), i);

        //init the tx control registers
        _iface->poke32(UE_REG_TX_CTRL_CLEAR_STATE, 1); //reset
        _iface->poke32(UE_REG_TX_CTRL_NUM_CHAN, 0);    //1 channel
        _iface->poke32(UE_REG_TX_CTRL_REPORT_SID, E100_ASYNC_SID);
        _iface->poke32(UE_REG_TX_CTRL_POLICY, UE_FLAG_TX_CTRL_POLICY_NEXT_PACKET);
    }
}

/***********************************************************************
 * RX DDC Get
 **********************************************************************/
void usrp_e100_impl::ddc_get(const wax::obj &key_, wax::obj &val, size_t which_dsp){
    named_prop_t key = named_prop_t::extract(key_);

    switch(key.as<dsp_prop_t>()){
    case DSP_PROP_NAME:
        val = str(boost::format("%s ddc%d") % _iface->get_cname() % which_dsp);
        return;

    case DSP_PROP_OTHERS:
        val = prop_names_t(); //empty
        return;

    case DSP_PROP_FREQ_SHIFT:
        val = _dsp_impl->ddc_freq[which_dsp];
        return;

    case DSP_PROP_CODEC_RATE:
        val = _clock_ctrl->get_fpga_clock_rate();
        return;

    case DSP_PROP_HOST_RATE:
        val = _clock_ctrl->get_fpga_clock_rate()/_dsp_impl->ddc_decim[which_dsp];
        return;

    default: UHD_THROW_PROP_GET_ERROR();
    }
}

/***********************************************************************
 * RX DDC Set
 **********************************************************************/
void usrp_e100_impl::ddc_set(const wax::obj &key_, const wax::obj &val, size_t which_dsp){
    named_prop_t key = named_prop_t::extract(key_);

    switch(key.as<dsp_prop_t>()){

    case DSP_PROP_STREAM_CMD:
        issue_ddc_stream_cmd(val.as<stream_cmd_t>(), which_dsp);
        return;

    case DSP_PROP_FREQ_SHIFT:{
            double new_freq = val.as<double>();
            _iface->poke32(UE_REG_DSP_RX_FREQ(which_dsp),
                dsp_type1::calc_cordic_word_and_update(new_freq, _clock_ctrl->get_fpga_clock_rate())
            );
            _dsp_impl->ddc_freq[which_dsp] = new_freq; //shadow
        }
        return;

    case DSP_PROP_HOST_RATE:{
            _dsp_impl->ddc_decim[which_dsp] = boost::math::iround(_clock_ctrl->get_fpga_clock_rate()/val.as<double>());

            //set the decimation
            _iface->poke32(UE_REG_DSP_RX_DECIM(which_dsp), dsp_type1::calc_cic_filter_word(_dsp_impl->ddc_decim[which_dsp]));
        }
        this->update_xport_channel_mapping(); //rate changed -> update
        return;

    default: UHD_THROW_PROP_SET_ERROR();
    }
}

/***********************************************************************
 * TX DUC Get
 **********************************************************************/
void usrp_e100_impl::duc_get(const wax::obj &key_, wax::obj &val, size_t which_dsp){
    named_prop_t key = named_prop_t::extract(key_);

    switch(key.as<dsp_prop_t>()){
    case DSP_PROP_NAME:
        val = str(boost::format("%s duc%d") % _iface->get_cname() % which_dsp);
        return;

    case DSP_PROP_OTHERS:
        val = prop_names_t(); //empty
        return;

    case DSP_PROP_FREQ_SHIFT:
        val = _dsp_impl->duc_freq[which_dsp];
        return;

    case DSP_PROP_CODEC_RATE:
        val = _clock_ctrl->get_fpga_clock_rate();
        return;

    case DSP_PROP_HOST_RATE:
        val = _clock_ctrl->get_fpga_clock_rate()/_dsp_impl->duc_interp[which_dsp];
        return;

    default: UHD_THROW_PROP_GET_ERROR();
    }
}

/***********************************************************************
 * TX DUC Set
 **********************************************************************/
void usrp_e100_impl::duc_set(const wax::obj &key_, const wax::obj &val, size_t which_dsp){
    named_prop_t key = named_prop_t::extract(key_);

    switch(key.as<dsp_prop_t>()){

    case DSP_PROP_FREQ_SHIFT:{
            double new_freq = val.as<double>();
            _iface->poke32(UE_REG_DSP_TX_FREQ,
                dsp_type1::calc_cordic_word_and_update(new_freq, _clock_ctrl->get_fpga_clock_rate())
            );
            _dsp_impl->duc_freq[which_dsp] = new_freq; //shadow
        }
        return;

    case DSP_PROP_HOST_RATE:{
            _dsp_impl->duc_interp[which_dsp] = boost::math::iround(_clock_ctrl->get_fpga_clock_rate()/val.as<double>());

            //set the interpolation
            _iface->poke32(UE_REG_DSP_TX_INTERP_RATE, dsp_type1::calc_cic_filter_word(_dsp_impl->duc_interp[which_dsp]));

            //set the scaling
            _iface->poke32(UE_REG_DSP_TX_SCALE_IQ, dsp_type1::calc_iq_scale_word(_dsp_impl->duc_interp[which_dsp]));
        }
        this->update_xport_channel_mapping(); //rate changed -> update
        return;

    default: UHD_THROW_PROP_SET_ERROR();
    }
}
