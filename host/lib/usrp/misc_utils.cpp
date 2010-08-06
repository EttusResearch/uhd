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

#include "misc_utils.hpp"
#include <uhd/utils/gain_group.hpp>
#include <uhd/usrp/subdev_props.hpp>
#include <uhd/usrp/codec_props.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>

using namespace uhd;
using namespace uhd::usrp;

static const size_t subdev_gain_priority = 1; //higher, closer to the antenna
static const size_t codec_gain_priority = 0;

/***********************************************************************
 * codec gain group helper functions:
 *    do this so we dont have to bind a templated function
 **********************************************************************/
static gain_range_t get_codec_gain_range(wax::obj codec, const std::string &name){
    return codec[named_prop_t(CODEC_PROP_GAIN_RANGE, name)].as<gain_range_t>();
}

static float get_codec_gain_i(wax::obj codec, const std::string &name){
    return codec[named_prop_t(CODEC_PROP_GAIN_I, name)].as<float>();
}

static float get_codec_gain_q(wax::obj codec, const std::string &name){
    return codec[named_prop_t(CODEC_PROP_GAIN_Q, name)].as<float>();
}

static void set_codec_gain_both(wax::obj codec, const std::string &name, float gain){
    codec[named_prop_t(CODEC_PROP_GAIN_I, name)] = gain;
    codec[named_prop_t(CODEC_PROP_GAIN_Q, name)] = gain;
}

static void set_codec_gain_i(wax::obj codec, const std::string &name, float gain){
    codec[named_prop_t(CODEC_PROP_GAIN_I, name)] = gain;
}

static void set_codec_gain_q(wax::obj codec, const std::string &name, float gain){
    codec[named_prop_t(CODEC_PROP_GAIN_Q, name)] = gain;
}

/***********************************************************************
 * subdev gain group helper functions:
 *    do this so we dont have to bind a templated function
 **********************************************************************/
static float get_subdev_gain(wax::obj subdev, const std::string &name){
    return subdev[named_prop_t(SUBDEV_PROP_GAIN, name)].as<float>();
}

static gain_range_t get_subdev_gain_range(wax::obj subdev, const std::string &name){
    return subdev[named_prop_t(SUBDEV_PROP_GAIN_RANGE, name)].as<gain_range_t>();
}

static void set_subdev_gain(wax::obj subdev, const std::string &name, float gain){
    subdev[named_prop_t(SUBDEV_PROP_GAIN, name)] = gain;
}

/***********************************************************************
 * gain group factory function for usrp
 **********************************************************************/
gain_group::sptr usrp::make_gain_group(wax::obj subdev, wax::obj codec){
    gain_group::sptr gg = gain_group::make();
    gain_fcns_t fcns;
    //add all the subdev gains first (antenna to dsp order)
    BOOST_FOREACH(const std::string &name, subdev[SUBDEV_PROP_GAIN_NAMES].as<prop_names_t>()){
        fcns.get_range = boost::bind(&get_subdev_gain_range, subdev, name);
        fcns.get_value = boost::bind(&get_subdev_gain, subdev, name);
        fcns.set_value = boost::bind(&set_subdev_gain, subdev, name, _1);
        gg->register_fcns(fcns, subdev_gain_priority);
    }
    //add all the codec gains last (antenna to dsp order)
    BOOST_FOREACH(const std::string &name, codec[CODEC_PROP_GAIN_NAMES].as<prop_names_t>()){
        fcns.get_range = boost::bind(&get_codec_gain_range, codec, name);

        //register the value functions depending upon the connection type
        switch(subdev[SUBDEV_PROP_CONNECTION].as<subdev_conn_t>()){
        case SUBDEV_CONN_COMPLEX_IQ:
        case SUBDEV_CONN_COMPLEX_QI:
            fcns.get_value = boost::bind(&get_codec_gain_i, codec, name); //same as Q
            fcns.set_value = boost::bind(&set_codec_gain_both, codec, name, _1); //sets both
            break;

        case SUBDEV_CONN_REAL_I:
            fcns.get_value = boost::bind(&get_codec_gain_i, codec, name);
            fcns.set_value = boost::bind(&set_codec_gain_i, codec, name, _1);
            break;

        case SUBDEV_CONN_REAL_Q:
            fcns.get_value = boost::bind(&get_codec_gain_q, codec, name);
            fcns.set_value = boost::bind(&set_codec_gain_q, codec, name, _1);
            break;
        }
        gg->register_fcns(fcns, codec_gain_priority);
    }
    return gg;
}
