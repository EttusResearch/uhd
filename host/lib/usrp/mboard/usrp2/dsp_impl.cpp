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
 * Helper Methods
 **********************************************************************/
void usrp2_impl::ddc_init(size_t which){
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
    //TODO
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
    if (key_name == "decim"){
        val = _ddc_decim[which];
        return;
    }
    if (key_name == "freq"){
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
        _ddc_decim[which] = new_decim; //shadow
        update_ddc_config(which);
        return;
    }
    if (key_name == "freq"){
        freq_t new_freq = wax::cast<freq_t>(val);
        _ddc_freq[which] = new_freq; //shadow
        update_ddc_config(which);
        return;
    }

    throw std::invalid_argument(str(
        boost::format("error setting: unknown key with name %s") % key_name
    ));
}

/***********************************************************************
 * DUC Properties
 **********************************************************************/
