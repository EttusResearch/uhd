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
#include <uhd/props.hpp>
#include <boost/assign/list_of.hpp>
#include "usrp2_impl.hpp"

using namespace uhd;

/***********************************************************************
 * DDC Helper Methods
 **********************************************************************/
void usrp2_impl::init_ddc_config(size_t which){
    //load the allowed decim/interp rates
    //_USRP2_RATES = range(4, 128+1, 1) + range(130, 256+1, 2) + range(260, 512+1, 4)
    for (size_t i = 4; i <= 128; i+=1){
        _allowed_decim_and_interp_rates.push_back(i);
    }
    for (size_t i = 130; i <= 256; i+=2){
        _allowed_decim_and_interp_rates.push_back(i);
    }
    for (size_t i = 260; i <= 512; i+=4){
        _allowed_decim_and_interp_rates.push_back(i);
    }

    //create the ddc in the rx dsp dict
    _rx_dsps[str(boost::format("ddc%d") % which)] = wax_obj_proxy(
        boost::bind(&usrp2_impl::ddc_get, this, _1, _2, which),
        boost::bind(&usrp2_impl::ddc_set, this, _1, _2, which)
    );

    //initial config and update
    _ddc_decim[which] = 16;
    _ddc_freq[which] = 0;
    update_ddc_config(which);
}

void usrp2_impl::update_ddc_config(size_t){
    //TODO send it!
}

/***********************************************************************
 * DDC Properties
 **********************************************************************/
void usrp2_impl::ddc_get(const wax::obj &key, wax::obj &val, size_t which){
    //handle the case where the key is an expected dsp property
    if (key.type() == typeid(dsp_prop_t)){
        switch(wax::cast<dsp_prop_t>(key)){
        case DSP_PROP_NAME:
            val = str(boost::format("usrp2 ddc%d") % which);
            return;

        case DSP_PROP_OTHERS:{
                prop_names_t others = boost::assign::list_of
                    ("rate")
                    ("decim")
                    ("decim_rates")
                    ("freq")
                ;
                val = others;
            }
            return;
        }
    }

    //handle string-based properties specific to this dsp
    std::string key_name = wax::cast<std::string>(key);
    if (key_name == "rate"){
        val = get_master_clock_freq();
        return;
    }
    else if (key_name == "decim"){
        val = _ddc_decim[which];
        return;
    }
    else if (key_name == "decim_rates"){
        val = _allowed_decim_and_interp_rates;
        return;
    }
    else if (key_name == "freq"){
        val = _ddc_freq[which];
        return;
    }

    throw std::invalid_argument(str(
        boost::format("error getting: unknown key with name %s") % key_name
    ));
}

void usrp2_impl::ddc_set(const wax::obj &key, const wax::obj &val, size_t which){
    //handle string-based properties specific to this dsp
    std::string key_name = wax::cast<std::string>(key);
    if (key_name == "decim"){
        size_t new_decim = wax::cast<size_t>(val);
        ASSERT_THROW(std::has(
            _allowed_decim_and_interp_rates.begin(),
            _allowed_decim_and_interp_rates.end(),
            new_decim
        ));
        _ddc_decim[which] = new_decim; //shadow
        update_ddc_config(which);
        return;
    }
    else if (key_name == "freq"){
        freq_t new_freq = wax::cast<freq_t>(val);
        ASSERT_THROW(new_freq <= get_master_clock_freq()/2.0);
        ASSERT_THROW(new_freq >= -get_master_clock_freq()/2.0);
        _ddc_freq[which] = new_freq; //shadow
        update_ddc_config(which);
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
    _tx_dsps[str(boost::format("duc%d") % 0)] = wax_obj_proxy(
        boost::bind(&usrp2_impl::duc_get, this, _1, _2),
        boost::bind(&usrp2_impl::duc_set, this, _1, _2)
    );

    //initial config and update
    _duc_interp = 16;
    _duc_freq = 0;
    update_duc_config();
}

void usrp2_impl::update_duc_config(void){
    // Calculate CIC interpolation (i.e., without halfband interpolators)
    size_t interp = 0; //TODO
    while(interp > 128) interp /= 2;

    // Calculate closest multiplier constant to reverse gain absent scale multipliers
    size_t interp_cubed = pow(interp, 3);
    size_t scale = rint((4096*pow(2, ceil(log2(interp_cubed))))/(1.65*interp_cubed));

    //TODO send it!
}

/***********************************************************************
 * DUC Properties
 **********************************************************************/
void usrp2_impl::duc_get(const wax::obj &key, wax::obj &val){
    //handle the case where the key is an expected dsp property
    if (key.type() == typeid(dsp_prop_t)){
        switch(wax::cast<dsp_prop_t>(key)){
        case DSP_PROP_NAME:
            val = str(boost::format("usrp2 duc%d") % 0);
            return;

        case DSP_PROP_OTHERS:{
                prop_names_t others = boost::assign::list_of
                    ("rate")
                    ("interp")
                    ("interp_rates")
                    ("freq")
                ;
                val = others;
            }
            return;
        }
    }

    //handle string-based properties specific to this dsp
    std::string key_name = wax::cast<std::string>(key);
    if (key_name == "rate"){
        val = get_master_clock_freq();
        return;
    }
    else if (key_name == "interp"){
        val = _duc_interp;
        return;
    }
    else if (key_name == "interp_rates"){
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
    std::string key_name = wax::cast<std::string>(key);
    if (key_name == "interp"){
        size_t new_interp = wax::cast<size_t>(val);
        ASSERT_THROW(std::has(
            _allowed_decim_and_interp_rates.begin(),
            _allowed_decim_and_interp_rates.end(),
            new_interp
        ));
        _duc_interp = new_interp; //shadow
        update_duc_config();
        return;
    }
    else if (key_name == "freq"){
        freq_t new_freq = wax::cast<freq_t>(val);
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
