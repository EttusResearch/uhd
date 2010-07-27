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

#include "usrp2_impl.hpp"
#include <uhd/usrp/codec_props.hpp>
#include <boost/bind.hpp>

using namespace uhd;
using namespace uhd::usrp;

/***********************************************************************
 * Helper Methods
 **********************************************************************/
void usrp2_mboard_impl::codec_init(void){
    //make proxies
    _rx_codec_proxy = wax_obj_proxy::make(
        boost::bind(&usrp2_mboard_impl::rx_codec_get, this, _1, _2),
        boost::bind(&usrp2_mboard_impl::rx_codec_set, this, _1, _2)
    );
    _tx_codec_proxy = wax_obj_proxy::make(
        boost::bind(&usrp2_mboard_impl::tx_codec_get, this, _1, _2),
        boost::bind(&usrp2_mboard_impl::tx_codec_set, this, _1, _2)
    );
}

/***********************************************************************
 * RX Codec Properties
 **********************************************************************/
void usrp2_mboard_impl::rx_codec_get(const wax::obj &key_, wax::obj &val){
    wax::obj key; std::string name;
    boost::tie(key, name) = extract_named_prop(key_);

    //handle the get request conditioned on the key
    switch(key.as<codec_prop_t>()){
    case CODEC_PROP_NAME:
        val = std::string("usrp2 adc");
        return;

    case CODEC_PROP_OTHERS:
        val = prop_names_t();
        return;

    case CODEC_PROP_GAIN_NAMES:
        val = prop_names_t(); //no gain elements to be controlled
        return;

    default: UHD_THROW_PROP_GET_ERROR();
    }
}

void usrp2_mboard_impl::rx_codec_set(const wax::obj &, const wax::obj &){
    UHD_THROW_PROP_SET_ERROR();
}

/***********************************************************************
 * TX Codec Properties
 **********************************************************************/
void usrp2_mboard_impl::tx_codec_get(const wax::obj &key_, wax::obj &val){
    wax::obj key; std::string name;
    boost::tie(key, name) = extract_named_prop(key_);

    //handle the get request conditioned on the key
    switch(key.as<codec_prop_t>()){
    case CODEC_PROP_NAME:
        val = std::string("usrp2 dac - ad9777");
        return;

    case CODEC_PROP_OTHERS:
        val = prop_names_t();
        return;

    case CODEC_PROP_GAIN_NAMES:
        val = prop_names_t(); //no gain elements to be controlled
        return;

    default: UHD_THROW_PROP_GET_ERROR();
    }
}

void usrp2_mboard_impl::tx_codec_set(const wax::obj &, const wax::obj &){
    UHD_THROW_PROP_SET_ERROR();
}
