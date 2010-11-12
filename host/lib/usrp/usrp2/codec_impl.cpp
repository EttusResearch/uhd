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
#include <uhd/types/dict.hpp>
#include <uhd/types/ranges.hpp>
#include <boost/bind.hpp>
#include <boost/assign/list_of.hpp>
#include <uhd/utils/assert.hpp>
#include <uhd/utils/exception.hpp>

using namespace uhd;
using namespace uhd::usrp;
using namespace boost::assign;

//this only applies to USRP2P
static const uhd::dict<std::string, gain_range_t> codec_rx_gain_ranges = map_list_of
                                  ("analog", gain_range_t(0, 3.5, 3.5))
                                  ("digital", gain_range_t(0, 6.0, 0.5))
                                  ("digital-fine", gain_range_t(0, 0.5, 0.05));


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
    named_prop_t key = named_prop_t::extract(key_);

    //handle the get request conditioned on the key
    switch(key.as<codec_prop_t>()){
    case CODEC_PROP_NAME:
        switch(_iface->get_rev()){
        case usrp2_iface::USRP_N200:
        case usrp2_iface::USRP_N210:
            val = std::string(_iface->get_cname() + " adc - ads62p44");
            break;

        case usrp2_iface::USRP2_REV3:
        case usrp2_iface::USRP2_REV4:
            val = std::string(_iface->get_cname() + " adc - ltc2284");
            break;

        case usrp2_iface::USRP_NXXX:
            val = std::string(_iface->get_cname() + " adc - ??????");
            break;
        }
        return;

    case CODEC_PROP_OTHERS:
        val = prop_names_t();
        return;

    case CODEC_PROP_GAIN_NAMES:
        switch(_iface->get_rev()){
        case usrp2_iface::USRP_N200:
        case usrp2_iface::USRP_N210:
            val = prop_names_t(codec_rx_gain_ranges.keys());
            return;

        default: val = prop_names_t();
        }
        return;

    case CODEC_PROP_GAIN_I:
    case CODEC_PROP_GAIN_Q:
        assert_has(_codec_rx_gains.keys(), key.name, "codec rx gain name");
        val = _codec_rx_gains[key.name];
        return;

    case CODEC_PROP_GAIN_RANGE:
      assert_has(codec_rx_gain_ranges.keys(), key.name, "codec rx gain range name");
      val = codec_rx_gain_ranges[key.name];
      return;

    default: UHD_THROW_PROP_GET_ERROR();
    }
}

void usrp2_mboard_impl::rx_codec_set(const wax::obj &key_, const wax::obj &val){
    named_prop_t key = named_prop_t::extract(key_);

    switch(key.as<codec_prop_t>()) {
    case CODEC_PROP_GAIN_I:
    case CODEC_PROP_GAIN_Q:
        this->rx_codec_set_gain(val.as<float>(), key.name);
        return;

    default: UHD_THROW_PROP_SET_ERROR();
  }
}

/***********************************************************************
 * Helper function to set RX codec gain
 ***********************************************************************/

void usrp2_mboard_impl::rx_codec_set_gain(float gain, const std::string &name){
  assert_has(codec_rx_gain_ranges.keys(), name, "codec rx gain name");

  _codec_rx_gains[name] = gain;

  if(name == "analog") {
    _codec_ctrl->set_rx_analog_gain(gain > 0); //just turn it on or off
    return;
  }
  if(name == "digital") {
    _codec_ctrl->set_rx_digital_gain(gain);
    return;
  }
  if(name == "digital-fine") {
    _codec_ctrl->set_rx_digital_fine_gain(gain);
    return;
  }
  UHD_THROW_PROP_SET_ERROR();
}


/***********************************************************************
 * TX Codec Properties
 **********************************************************************/
void usrp2_mboard_impl::tx_codec_get(const wax::obj &key_, wax::obj &val){
    named_prop_t key = named_prop_t::extract(key_);

    //handle the get request conditioned on the key
    switch(key.as<codec_prop_t>()){
    case CODEC_PROP_NAME:
        val = std::string(_iface->get_cname() + " dac - ad9777");
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
