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

#include "usrp2_impl.hpp"
#include "usrp2_regs.hpp"
#include <uhd/usrp/dsp_utils.hpp>
#include <uhd/usrp/dsp_props.hpp>
#include <boost/bind.hpp>
#include <boost/math/special_functions/round.hpp>
#include <boost/math/special_functions/sign.hpp>
#include <algorithm>
#include <cmath>

using namespace uhd;
using namespace uhd::usrp;

/***********************************************************************
 * DSP impl and methods
 **********************************************************************/
struct usrp2_mboard_impl::dsp_impl{
    uhd::dict<size_t, size_t> ddc_decim;
    uhd::dict<size_t, double> ddc_freq;
    uhd::dict<size_t, size_t> duc_interp;
    uhd::dict<size_t, double> duc_freq;
    std::vector<size_t> decim_and_interp_rates;
    uhd::dict<size_t, bool> continuous_streaming;
};

void usrp2_mboard_impl::dsp_init(void){
    //create new dsp impl
    _dsp_impl = UHD_PIMPL_MAKE(dsp_impl, ());

    //load the allowed decim/interp rates
    //range(4, 128+1, 1) + range(130, 256+1, 2) + range(260, 512+1, 4)
    for (size_t i = 4; i <= 128; i+=1){
        _dsp_impl->decim_and_interp_rates.push_back(i);
    }
    for (size_t i = 130; i <= 256; i+=2){
        _dsp_impl->decim_and_interp_rates.push_back(i);
    }
    for (size_t i = 260; i <= 512; i+=4){
        _dsp_impl->decim_and_interp_rates.push_back(i);
    }

    //bind and initialize the rx dsps
    for (size_t i = 0; i < NUM_RX_DSPS; i++){
        _rx_dsp_proxies[str(boost::format("DSP%d")%i)] = wax_obj_proxy::make(
            boost::bind(&usrp2_mboard_impl::ddc_get, this, _1, _2, i),
            boost::bind(&usrp2_mboard_impl::ddc_set, this, _1, _2, i)
        );

        //initial config and update
        ddc_set(DSP_PROP_FREQ_SHIFT, double(0), i);
        ddc_set(DSP_PROP_HOST_RATE, double(get_master_clock_freq()/16), i);

        //setup the rx control registers
        _iface->poke32(U2_REG_RX_CTRL_CLEAR(i), 1); //reset
        _iface->poke32(U2_REG_RX_CTRL_NSAMPS_PP(i), _device.get_max_recv_samps_per_packet());
        _iface->poke32(U2_REG_RX_CTRL_NCHANNELS(i), 1);
        _iface->poke32(U2_REG_RX_CTRL_VRT_HDR(i), 0
            | (0x1 << 28) //if data with stream id
            | (0x1 << 26) //has trailer
            | (0x3 << 22) //integer time other
            | (0x1 << 20) //fractional time sample count
        );
        _iface->poke32(U2_REG_RX_CTRL_VRT_SID(i), usrp2_impl::RECV_SID);
        _iface->poke32(U2_REG_RX_CTRL_VRT_TLR(i), 0);
        _iface->poke32(U2_REG_TIME64_TPS, size_t(get_master_clock_freq()));
    }

    //bind and initialize the tx dsps
    for (size_t i = 0; i < NUM_TX_DSPS; i++){
        _tx_dsp_proxies[str(boost::format("DSP%d")%i)] = wax_obj_proxy::make(
            boost::bind(&usrp2_mboard_impl::duc_get, this, _1, _2, i),
            boost::bind(&usrp2_mboard_impl::duc_set, this, _1, _2, i)
        );

        //initial config and update
        duc_set(DSP_PROP_FREQ_SHIFT, double(0), i);
        duc_set(DSP_PROP_HOST_RATE, double(get_master_clock_freq()/16), i);

        //init the tx control registers
        _iface->poke32(U2_REG_TX_CTRL_CLEAR_STATE, 1); //reset
        _iface->poke32(U2_REG_TX_CTRL_NUM_CHAN, 0);    //1 channel
        _iface->poke32(U2_REG_TX_CTRL_REPORT_SID, usrp2_impl::ASYNC_SID);
        _iface->poke32(U2_REG_TX_CTRL_POLICY, U2_FLAG_TX_CTRL_POLICY_NEXT_PACKET);
    }
}

template <typename rate_type>
static rate_type pick_closest_rate(double exact_rate, const std::vector<rate_type> &rates){
    unsigned closest_match = rates.front();
    BOOST_FOREACH(rate_type possible_rate, rates){
        if(std::abs(exact_rate - possible_rate) < std::abs(exact_rate - closest_match))
            closest_match = possible_rate;
    }
    return closest_match;
}

void usrp2_mboard_impl::issue_ddc_stream_cmd(const stream_cmd_t &stream_cmd, size_t which_dsp){
    _dsp_impl->continuous_streaming[which_dsp] = stream_cmd.stream_mode == stream_cmd_t::STREAM_MODE_START_CONTINUOUS;
    _iface->poke32(U2_REG_RX_CTRL_STREAM_CMD(which_dsp), dsp_type1::calc_stream_cmd_word(stream_cmd));
    _iface->poke32(U2_REG_RX_CTRL_TIME_SECS(which_dsp),  boost::uint32_t(stream_cmd.time_spec.get_full_secs()));
    _iface->poke32(U2_REG_RX_CTRL_TIME_TICKS(which_dsp), stream_cmd.time_spec.get_tick_count(get_master_clock_freq()));
}

/***********************************************************************
 * DDC Properties
 **********************************************************************/
void usrp2_mboard_impl::ddc_get(const wax::obj &key_, wax::obj &val, size_t which_dsp){
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
        val = get_master_clock_freq();
        return;

    case DSP_PROP_HOST_RATE:
        val = get_master_clock_freq()/_dsp_impl->ddc_decim[which_dsp];
        return;

    default: UHD_THROW_PROP_GET_ERROR();
    }
}

void usrp2_mboard_impl::ddc_set(const wax::obj &key_, const wax::obj &val, size_t which_dsp){
    named_prop_t key = named_prop_t::extract(key_);

    switch(key.as<dsp_prop_t>()){

    case DSP_PROP_STREAM_CMD:
        issue_ddc_stream_cmd(val.as<stream_cmd_t>(), which_dsp);
        return;

    case DSP_PROP_FREQ_SHIFT:{
            double new_freq = val.as<double>();
            _iface->poke32(U2_REG_DSP_RX_FREQ(which_dsp),
                dsp_type1::calc_cordic_word_and_update(new_freq, get_master_clock_freq())
            );
            _dsp_impl->ddc_freq[which_dsp] = new_freq; //shadow
        }
        return;

    case DSP_PROP_HOST_RATE:{
            double extact_rate = get_master_clock_freq()/val.as<double>();
            _dsp_impl->ddc_decim[which_dsp] = pick_closest_rate(extact_rate, _dsp_impl->decim_and_interp_rates);

            //set the decimation
            _iface->poke32(U2_REG_DSP_RX_DECIM(which_dsp), dsp_type1::calc_cic_filter_word(_dsp_impl->ddc_decim[which_dsp]));
        }
        _device.update_xport_channel_mapping(); //rate changed -> update
        return;

    default: UHD_THROW_PROP_SET_ERROR();
    }
}

/***********************************************************************
 * DUC Properties
 **********************************************************************/
void usrp2_mboard_impl::duc_get(const wax::obj &key_, wax::obj &val, size_t which_dsp){
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
        val = get_master_clock_freq();
        return;

    case DSP_PROP_HOST_RATE:
        val = get_master_clock_freq()/_dsp_impl->duc_interp[which_dsp];
        return;

    default: UHD_THROW_PROP_GET_ERROR();
    }
}

void usrp2_mboard_impl::duc_set(const wax::obj &key_, const wax::obj &val, size_t which_dsp){
    named_prop_t key = named_prop_t::extract(key_);

    switch(key.as<dsp_prop_t>()){

    case DSP_PROP_FREQ_SHIFT:{
            const double codec_rate = get_master_clock_freq();
            double new_freq = val.as<double>();

            //calculate the DAC shift (multiples of rate)
            const int sign = boost::math::sign(new_freq);
            const int zone = std::min(boost::math::iround(new_freq/codec_rate), 2);
            const double dac_shift = sign*zone*codec_rate;
            new_freq -= dac_shift; //update FPGA DSP target freq

            //set the DAC shift (modulation mode)
            if (zone == 0) _codec_ctrl->set_tx_mod_mode(0); //no shift
            else _codec_ctrl->set_tx_mod_mode(sign*4/zone); //DAC interp = 4

            _iface->poke32(U2_REG_DSP_TX_FREQ,
                dsp_type1::calc_cordic_word_and_update(new_freq, codec_rate)
            );
            _dsp_impl->duc_freq[which_dsp] = new_freq + dac_shift; //shadow
        }
        return;

    case DSP_PROP_HOST_RATE:{
            double extact_rate = get_master_clock_freq()/val.as<double>();
            _dsp_impl->duc_interp[which_dsp] = pick_closest_rate(extact_rate, _dsp_impl->decim_and_interp_rates);

            //set the interpolation
            _iface->poke32(U2_REG_DSP_TX_INTERP_RATE, dsp_type1::calc_cic_filter_word(_dsp_impl->duc_interp[which_dsp]));

            //set the scaling
            _iface->poke32(U2_REG_DSP_TX_SCALE_IQ, dsp_type1::calc_iq_scale_word(_dsp_impl->duc_interp[which_dsp]));
        }
        _device.update_xport_channel_mapping(); //rate changed -> update
        return;

    default: UHD_THROW_PROP_SET_ERROR();
    }
}
