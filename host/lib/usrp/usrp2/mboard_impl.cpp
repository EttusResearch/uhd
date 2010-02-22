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
#include "usrp2_impl.hpp"

using namespace uhd;

/***********************************************************************
 * Helper Methods
 **********************************************************************/
void usrp2_impl::mboard_init(void){
    _mboards[""] = wax_obj_proxy(
        boost::bind(&usrp2_impl::mboard_get, this, _1, _2),
        boost::bind(&usrp2_impl::mboard_set, this, _1, _2)
    );
}

void usrp2_impl::init_clock_config(void){
    //init the pps source clock config
    _pps_source_dict["sma"]  = USRP2_PPS_SOURCE_SMA;
    _pps_source_dict["mimo"] = USRP2_PPS_SOURCE_MIMO;
    _pps_source = "sma";

    //init the pps polarity clock config
    _pps_polarity_dict["pos"] = USRP2_PPS_POLARITY_POS;
    _pps_polarity_dict["neg"] = USRP2_PPS_POLARITY_NEG;
    _pps_polarity = "neg";

    //init the ref source clock config
    _ref_source_dict["int"]  = USRP2_REF_SOURCE_INT;
    _ref_source_dict["sma"]  = USRP2_REF_SOURCE_SMA;
    _ref_source_dict["mimo"] = USRP2_REF_SOURCE_MIMO;
    _ref_source = "int";

    //update the clock config (sends a control packet)
    update_clock_config();
}

void usrp2_impl::update_clock_config(void){
    //setup the out data
    usrp2_ctrl_data_t out_data;
    out_data.id = htonl(USRP2_CTRL_ID_HERES_A_NEW_CLOCK_CONFIG_BRO);
    out_data.data.clock_config.pps_source   = _pps_source_dict  [_pps_source];
    out_data.data.clock_config.pps_polarity = _pps_polarity_dict[_pps_polarity];
    out_data.data.clock_config.ref_source   = _ref_source_dict  [_ref_source];

    //send and recv
    usrp2_ctrl_data_t in_data = ctrl_send_and_recv(out_data);
    ASSERT_THROW(htonl(in_data.id) == USRP2_CTRL_ID_GOT_THE_NEW_CLOCK_CONFIG_DUDE);
}

/***********************************************************************
 * MBoard Get Properties
 **********************************************************************/
void usrp2_impl::mboard_get(const wax::obj &key_, wax::obj &val){
    wax::obj key; std::string name;
    boost::tie(key, name) = extract_named_prop(key_);

    //handle the get request conditioned on the key
    switch(wax::cast<mboard_prop_t>(key)){
    case MBOARD_PROP_NAME:
        val = std::string("usrp2 mboard");
        return;

    case MBOARD_PROP_OTHERS:
        val = prop_names_t(); //empty other props
        return;

    case MBOARD_PROP_RX_DBOARD:
        val = _rx_dboards[name].get_link();
        return;

    case MBOARD_PROP_RX_DBOARD_NAMES:
        val = prop_names_t(_rx_dboards.get_keys());
        return;

    case MBOARD_PROP_TX_DBOARD:
        val = _tx_dboards[name].get_link();
        return;

    case MBOARD_PROP_TX_DBOARD_NAMES:
        val = prop_names_t(_tx_dboards.get_keys());
        return;

    case MBOARD_PROP_MTU:
        // FIXME we dont know the real MTU...
        // give them something to fragment about
        val = size_t(1500);
        return;

    case MBOARD_PROP_CLOCK_RATE:
        val = freq_t(get_master_clock_freq());
        return;

    case MBOARD_PROP_RX_DSP:
        val = _rx_dsps[name].get_link();
        return;

    case MBOARD_PROP_RX_DSP_NAMES:
        val = prop_names_t(_rx_dsps.get_keys());
        return;

    case MBOARD_PROP_TX_DSP:
        val = _tx_dsps[name].get_link();
        return;

    case MBOARD_PROP_TX_DSP_NAMES:
        val = prop_names_t(_tx_dsps.get_keys());
        return;

    case MBOARD_PROP_PPS_SOURCE:
        val = _pps_source;
        return;

    case MBOARD_PROP_PPS_SOURCE_NAMES:
        val = prop_names_t(_pps_source_dict.get_keys());
        return;

    case MBOARD_PROP_PPS_POLARITY:
        val = _pps_polarity;
        return;

    case MBOARD_PROP_REF_SOURCE:
        val = _ref_source;
        return;

    case MBOARD_PROP_REF_SOURCE_NAMES:
        val = prop_names_t(_ref_source_dict.get_keys());
        return;

    case MBOARD_PROP_TIME_NOW:
    case MBOARD_PROP_TIME_NEXT_PPS:
        throw std::runtime_error("Error: trying to get write-only property on usrp2 mboard");

    }
}

/***********************************************************************
 * MBoard Set Properties
 **********************************************************************/
void usrp2_impl::mboard_set(const wax::obj &key, const wax::obj &val){
    //handle the get request conditioned on the key
    switch(wax::cast<mboard_prop_t>(key)){

    case MBOARD_PROP_PPS_SOURCE:{
            std::string name = wax::cast<std::string>(val);
            ASSERT_THROW(_pps_source_dict.has_key(name));
            _pps_source = name; //shadow
            update_clock_config();
        }
        return;

    case MBOARD_PROP_PPS_POLARITY:{
            std::string name = wax::cast<std::string>(val);
            ASSERT_THROW(_pps_polarity_dict.has_key(name));
            _pps_polarity = name; //shadow
            update_clock_config();
        }
        return;

    case MBOARD_PROP_REF_SOURCE:{
            std::string name = wax::cast<std::string>(val);
            ASSERT_THROW(_ref_source_dict.has_key(name));
            _ref_source = name; //shadow
            update_clock_config();
        }
        return;

    case MBOARD_PROP_NAME:
    case MBOARD_PROP_OTHERS:
    case MBOARD_PROP_MTU:
    case MBOARD_PROP_CLOCK_RATE:
    case MBOARD_PROP_RX_DSP:
    case MBOARD_PROP_RX_DSP_NAMES:
    case MBOARD_PROP_TX_DSP:
    case MBOARD_PROP_TX_DSP_NAMES:
    case MBOARD_PROP_RX_DBOARD:
    case MBOARD_PROP_RX_DBOARD_NAMES:
    case MBOARD_PROP_TX_DBOARD:
    case MBOARD_PROP_TX_DBOARD_NAMES:
    case MBOARD_PROP_PPS_SOURCE_NAMES:
    case MBOARD_PROP_REF_SOURCE_NAMES:
    case MBOARD_PROP_TIME_NOW:
    case MBOARD_PROP_TIME_NEXT_PPS:
        throw std::runtime_error("Error: trying to set read-only property on usrp2 mboard");

    }
}
