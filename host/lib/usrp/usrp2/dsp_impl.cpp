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

#include <uhd/utils.hpp>
#include <boost/format.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/math/special_functions/round.hpp>
#include "usrp2_impl.hpp"

using namespace uhd;

static const size_t default_decim = 16;
static const size_t default_interp = 16;

#define rint boost::math::iround

template <class T> T log2(T num){
    return std::log(num)/std::log(T(2));
}

/***********************************************************************
 * DDC Helper Methods
 **********************************************************************/
static boost::uint32_t calculate_freq_word_and_update_actual_freq(freq_t &freq, freq_t clock_freq){
    double scale_factor = std::pow(2.0, 32);

    //calculate the freq register word
    boost::uint32_t freq_word = rint((freq / clock_freq) * scale_factor);

    //update the actual frequency
    freq = (double(freq_word) / scale_factor) * clock_freq;

    return freq_word;
}

static boost::uint32_t calculate_iq_scale_word(boost::int16_t i, boost::int16_t q){
    return (boost::uint16_t(i) << 16) | (boost::uint16_t(q) << 0);
}

void usrp2_impl::init_ddc_config(void){
    //create the ddc in the rx dsp dict
    _rx_dsps["ddc0"] = wax_obj_proxy::make(
        boost::bind(&usrp2_impl::ddc_get, this, _1, _2),
        boost::bind(&usrp2_impl::ddc_set, this, _1, _2)
    );

    //initial config and update
    _ddc_decim = default_decim;
    _ddc_freq = 0;
    update_ddc_config();

    _ddc_stream_at = time_spec_t();
    _ddc_enabled = false;
    update_ddc_enabled();
}

void usrp2_impl::update_ddc_config(void){
    //setup the out data
    usrp2_ctrl_data_t out_data;
    out_data.id = htonl(USRP2_CTRL_ID_SETUP_THIS_DDC_FOR_ME_BRO);
    out_data.data.ddc_args.freq_word = htonl(
        calculate_freq_word_and_update_actual_freq(_ddc_freq, get_master_clock_freq())
    );
    out_data.data.ddc_args.decim = htonl(_ddc_decim);
    static const boost::int16_t default_rx_scale_iq = 1024;
    out_data.data.ddc_args.scale_iq = htonl(
        calculate_iq_scale_word(default_rx_scale_iq, default_rx_scale_iq)
    );

    //send and recv
    usrp2_ctrl_data_t in_data = ctrl_send_and_recv(out_data);
    ASSERT_THROW(htonl(in_data.id) == USRP2_CTRL_ID_TOTALLY_SETUP_THE_DDC_DUDE);
}

void usrp2_impl::update_ddc_enabled(void){
    //setup the out data
    usrp2_ctrl_data_t out_data;
    out_data.id = htonl(USRP2_CTRL_ID_CONFIGURE_STREAMING_FOR_ME_BRO);
    out_data.data.streaming.enabled = (_ddc_enabled)? 1 : 0;
    out_data.data.streaming.secs =  htonl(_ddc_stream_at.secs);
    out_data.data.streaming.ticks = htonl(_ddc_stream_at.ticks);
    out_data.data.streaming.samples = htonl(_max_rx_samples_per_packet);

    //send and recv
    usrp2_ctrl_data_t in_data = ctrl_send_and_recv(out_data);
    ASSERT_THROW(htonl(in_data.id) == USRP2_CTRL_ID_CONFIGURED_THAT_STREAMING_DUDE);

    //clear the stream at time spec (it must be set for the next round of enable/disable)
    _ddc_stream_at = time_spec_t();
}

/***********************************************************************
 * DDC Properties
 **********************************************************************/
void usrp2_impl::ddc_get(const wax::obj &key, wax::obj &val){
    //handle the case where the key is an expected dsp property
    if (key.type() == typeid(dsp_prop_t)){
        switch(key.as<dsp_prop_t>()){
        case DSP_PROP_NAME:
            val = std::string("usrp2 ddc0");
            return;

        case DSP_PROP_OTHERS:{
                prop_names_t others = boost::assign::list_of
                    ("rate")
                    ("decim")
                    ("decims")
                    ("freq")
                    ("enabled")
                    ("stream_at")
                ;
                val = others;
            }
            return;
        }
    }

    //handle string-based properties specific to this dsp
    std::string key_name = key.as<std::string>();
    if (key_name == "rate"){
        val = get_master_clock_freq();
        return;
    }
    else if (key_name == "decim"){
        val = _ddc_decim;
        return;
    }
    else if (key_name == "decims"){
        val = _allowed_decim_and_interp_rates;
        return;
    }
    else if (key_name == "freq"){
        val = _ddc_freq;
        return;
    }
    else if (key_name == "enabled"){
        val = _ddc_enabled;
        return;
    }

    throw std::invalid_argument(str(
        boost::format("error getting: unknown key with name %s") % key_name
    ));
}

void usrp2_impl::ddc_set(const wax::obj &key, const wax::obj &val){
    //handle string-based properties specific to this dsp
    std::string key_name = key.as<std::string>();
    if (key_name == "decim"){
        size_t new_decim = val.as<size_t>();
        assert_has(
            _allowed_decim_and_interp_rates,
            new_decim, "usrp2 decimation"
        );
        _ddc_decim = new_decim; //shadow
        update_ddc_config();
        return;
    }
    else if (key_name == "freq"){
        freq_t new_freq = val.as<freq_t>();
        ASSERT_THROW(new_freq <= get_master_clock_freq()/2.0);
        ASSERT_THROW(new_freq >= -get_master_clock_freq()/2.0);
        _ddc_freq = new_freq; //shadow
        update_ddc_config();
        return;
    }
    else if (key_name == "enabled"){
        bool new_enabled = val.as<bool>();
        _ddc_enabled = new_enabled; //shadow
        update_ddc_enabled();
        return;
    }
    else if (key_name == "stream_at"){
        time_spec_t new_stream_at = val.as<time_spec_t>();
        _ddc_stream_at = new_stream_at; //shadow
        //update_ddc_enabled(); //dont update from here
        return;
    }

    throw std::invalid_argument(str(
        boost::format("error setting: unknown key with name %s") % key_name
    ));
}

/***********************************************************************
 * DUC Helper Methods
 **********************************************************************/
void usrp2_impl::init_duc_config(void){
    //create the duc in the tx dsp dict
    _tx_dsps["duc0"] = wax_obj_proxy::make(
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

    //setup the out data
    usrp2_ctrl_data_t out_data;
    out_data.id = htonl(USRP2_CTRL_ID_SETUP_THIS_DUC_FOR_ME_BRO);
    out_data.data.duc_args.freq_word = htonl(
        calculate_freq_word_and_update_actual_freq(_duc_freq, get_master_clock_freq())
    );
    out_data.data.duc_args.interp = htonl(_duc_interp);
    out_data.data.duc_args.scale_iq = htonl(
        calculate_iq_scale_word(scale, scale)
    );

    //send and recv
    usrp2_ctrl_data_t in_data = ctrl_send_and_recv(out_data);
    ASSERT_THROW(htonl(in_data.id) == USRP2_CTRL_ID_TOTALLY_SETUP_THE_DUC_DUDE);
}

/***********************************************************************
 * DUC Properties
 **********************************************************************/
void usrp2_impl::duc_get(const wax::obj &key, wax::obj &val){
    //handle the case where the key is an expected dsp property
    if (key.type() == typeid(dsp_prop_t)){
        switch(key.as<dsp_prop_t>()){
        case DSP_PROP_NAME:
            val = std::string("usrp2 duc0");
            return;

        case DSP_PROP_OTHERS:{
                prop_names_t others = boost::assign::list_of
                    ("rate")
                    ("interp")
                    ("interps")
                    ("freq")
                ;
                val = others;
            }
            return;
        }
    }

    //handle string-based properties specific to this dsp
    std::string key_name = key.as<std::string>();
    if (key_name == "rate"){
        val = get_master_clock_freq();
        return;
    }
    else if (key_name == "interp"){
        val = _duc_interp;
        return;
    }
    else if (key_name == "interps"){
        val = _allowed_decim_and_interp_rates;
        return;
    }
    else if (key_name == "freq"){
        val = _duc_freq;
        return;
    }

    throw std::invalid_argument(str(
        boost::format("error getting: unknown key with name %s") % key_name
    ));
}

void usrp2_impl::duc_set(const wax::obj &key, const wax::obj &val){
    //handle string-based properties specific to this dsp
    std::string key_name = key.as<std::string>();
    if (key_name == "interp"){
        size_t new_interp = val.as<size_t>();
        assert_has(
            _allowed_decim_and_interp_rates,
            new_interp, "usrp2 interpolation"
        );
        _duc_interp = new_interp; //shadow
        update_duc_config();
        return;
    }
    else if (key_name == "freq"){
        freq_t new_freq = val.as<freq_t>();
        ASSERT_THROW(new_freq <= get_master_clock_freq()/2.0);
        ASSERT_THROW(new_freq >= -get_master_clock_freq()/2.0);
        _duc_freq = new_freq; //shadow
        update_duc_config();
        return;
    }

    throw std::invalid_argument(str(
        boost::format("error setting: unknown key with name %s") % key_name
    ));
}
