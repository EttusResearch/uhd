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
#include <uhd/utils/assert.hpp>
#include <uhd/usrp/mboard_props.hpp>
#include <boost/bind.hpp>

using namespace uhd;
using namespace uhd::usrp;

/***********************************************************************
 * Mboard Initialization
 **********************************************************************/
void usrp_e_impl::mboard_init(void){
    _mboard_proxy = wax_obj_proxy::make(
        boost::bind(&usrp_e_impl::mboard_get, this, _1, _2),
        boost::bind(&usrp_e_impl::mboard_set, this, _1, _2)
    );

    //init the clock config
    _clock_config.ref_source = clock_config_t::REF_AUTO;
    _clock_config.pps_source = clock_config_t::PPS_SMA;

    //TODO poke the clock config regs
}

/***********************************************************************
 * Mboard Get
 **********************************************************************/
void usrp_e_impl::mboard_get(const wax::obj &key_, wax::obj &val){
    wax::obj key; std::string name;
    boost::tie(key, name) = extract_named_prop(key_);


    //handle the get request conditioned on the key
    switch(key.as<mboard_prop_t>()){
    case MBOARD_PROP_NAME:
        val = std::string("usrp-e mboard");
        return;

    case MBOARD_PROP_OTHERS:
        val = prop_names_t();
        return;

    case MBOARD_PROP_RX_DBOARD:
        ASSERT_THROW(name == "");
        val = _rx_dboard_proxy->get_link();
        return;

    case MBOARD_PROP_RX_DBOARD_NAMES:
        val = prop_names_t(1, ""); //vector of size 1 with empty string
        return;

    case MBOARD_PROP_TX_DBOARD:
        ASSERT_THROW(name == "");
        val = _tx_dboard_proxy->get_link();
        return;

    case MBOARD_PROP_TX_DBOARD_NAMES:
        val = prop_names_t(1, ""); //vector of size 1 with empty string
        return;

    case MBOARD_PROP_STREAM_CMD:
        //val = TODO
        return;

    case MBOARD_PROP_RX_DSP:
        ASSERT_THROW(name == "ddc0");
        val = _rx_ddc_proxy->get_link();
        return;

    case MBOARD_PROP_RX_DSP_NAMES:
        val = prop_names_t(1, "ddc0");
        return;

    case MBOARD_PROP_TX_DSP:
        ASSERT_THROW(name == "duc0");
        val = _tx_duc_proxy->get_link();
        return;

    case MBOARD_PROP_TX_DSP_NAMES:
        val = prop_names_t(1, "duc0");
        return;

    case MBOARD_PROP_CLOCK_CONFIG:
        val = _clock_config;
        return;

    case MBOARD_PROP_TIME_NOW:
    case MBOARD_PROP_TIME_NEXT_PPS:
        throw std::runtime_error("Error: trying to get write-only property on usrp-e mboard");

    }
}

/***********************************************************************
 * Mboard Set
 **********************************************************************/
void usrp_e_impl::mboard_set(const wax::obj &, const wax::obj &){
    
}
