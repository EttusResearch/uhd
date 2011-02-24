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
#include <uhd/exception.hpp>
#include <uhd/usrp/codec_props.hpp>
#include <boost/bind.hpp>

using namespace uhd;
using namespace uhd::usrp;

/***********************************************************************
 * Helper Methods
 **********************************************************************/
void usrp_e100_impl::codec_init(void){
    //make proxies
    _rx_codec_proxy = wax_obj_proxy::make(
        boost::bind(&usrp_e100_impl::rx_codec_get, this, _1, _2),
        boost::bind(&usrp_e100_impl::rx_codec_set, this, _1, _2)
    );
    _tx_codec_proxy = wax_obj_proxy::make(
        boost::bind(&usrp_e100_impl::tx_codec_get, this, _1, _2),
        boost::bind(&usrp_e100_impl::tx_codec_set, this, _1, _2)
    );
}

/***********************************************************************
 * RX Codec Properties
 **********************************************************************/
static const std::string ad9862_pga_gain_name = "ad9862 pga";

void usrp_e100_impl::rx_codec_get(const wax::obj &key_, wax::obj &val){
    named_prop_t key = named_prop_t::extract(key_);

    //handle the get request conditioned on the key
    switch(key.as<codec_prop_t>()){
    case CODEC_PROP_NAME:
        val = std::string("usrp-e adc - ad9522");
        return;

    case CODEC_PROP_OTHERS:
        val = prop_names_t();
        return;

    case CODEC_PROP_GAIN_NAMES:
        val = prop_names_t(1, ad9862_pga_gain_name);
        return;

    case CODEC_PROP_GAIN_RANGE:
        UHD_ASSERT_THROW(key.name == ad9862_pga_gain_name);
        val = usrp_e100_codec_ctrl::rx_pga_gain_range;
        return;

    case CODEC_PROP_GAIN_I:
        UHD_ASSERT_THROW(key.name == ad9862_pga_gain_name);
        val = _codec_ctrl->get_rx_pga_gain('A');
        return;

    case CODEC_PROP_GAIN_Q:
        UHD_ASSERT_THROW(key.name == ad9862_pga_gain_name);
        val = _codec_ctrl->get_rx_pga_gain('B');
        return;

    default: UHD_THROW_PROP_GET_ERROR();
    }
}

void usrp_e100_impl::rx_codec_set(const wax::obj &key_, const wax::obj &val){
    named_prop_t key = named_prop_t::extract(key_);

    //handle the set request conditioned on the key
    switch(key.as<codec_prop_t>()){
    case CODEC_PROP_GAIN_I:
        UHD_ASSERT_THROW(key.name == ad9862_pga_gain_name);
        _codec_ctrl->set_rx_pga_gain(val.as<double>(), 'A');
        return;

    case CODEC_PROP_GAIN_Q:
        UHD_ASSERT_THROW(key.name == ad9862_pga_gain_name);
        _codec_ctrl->set_rx_pga_gain(val.as<double>(), 'B');
        return;

    default: UHD_THROW_PROP_SET_ERROR();
    }
}

/***********************************************************************
 * TX Codec Properties
 **********************************************************************/
void usrp_e100_impl::tx_codec_get(const wax::obj &key_, wax::obj &val){
    named_prop_t key = named_prop_t::extract(key_);

    //handle the get request conditioned on the key
    switch(key.as<codec_prop_t>()){
    case CODEC_PROP_NAME:
        val = std::string("usrp-e dac - ad9522");
        return;

    case CODEC_PROP_OTHERS:
        val = prop_names_t();
        return;

    case CODEC_PROP_GAIN_NAMES:
        val = prop_names_t(1, ad9862_pga_gain_name);
        return;

    case CODEC_PROP_GAIN_RANGE:
        UHD_ASSERT_THROW(key.name == ad9862_pga_gain_name);
        val = usrp_e100_codec_ctrl::tx_pga_gain_range;
        return;

    case CODEC_PROP_GAIN_I: //only one gain for I and Q
    case CODEC_PROP_GAIN_Q:
        UHD_ASSERT_THROW(key.name == ad9862_pga_gain_name);
        val = _codec_ctrl->get_tx_pga_gain();
        return;

    default: UHD_THROW_PROP_GET_ERROR();
    }
}

void usrp_e100_impl::tx_codec_set(const wax::obj &key_, const wax::obj &val){
    named_prop_t key = named_prop_t::extract(key_);

    //handle the set request conditioned on the key
    switch(key.as<codec_prop_t>()){
    case CODEC_PROP_GAIN_I: //only one gain for I and Q
    case CODEC_PROP_GAIN_Q:
        UHD_ASSERT_THROW(key.name == ad9862_pga_gain_name);
        _codec_ctrl->set_tx_pga_gain(val.as<double>());
        return;

    default: UHD_THROW_PROP_SET_ERROR();
    }
}
