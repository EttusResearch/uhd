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

/***********************************************************************
 * gain group functions
 **********************************************************************/
static void gg_set_iq_value(wax::obj codec, const std::string &name, float gain){
    codec[named_prop_t(CODEC_PROP_GAIN_I, name)] = gain;
    codec[named_prop_t(CODEC_PROP_GAIN_Q, name)] = gain;
}

gain_group::sptr usrp::make_gain_group(wax::obj subdev, wax::obj codec){
    gain_group::sptr gg = gain_group::make();
    gain_fcns_t fcns;
    //add all the subdev gains first (antenna to dsp order)
    BOOST_FOREACH(const std::string &name, subdev[SUBDEV_PROP_GAIN_NAMES].as<prop_names_t>()){
        fcns.get_range = boost::bind(&wax::obj::as<gain_range_t>, subdev[named_prop_t(SUBDEV_PROP_GAIN_RANGE, name)]);
        fcns.get_value = boost::bind(&wax::obj::as<float>, subdev[named_prop_t(SUBDEV_PROP_GAIN, name)]);
        fcns.set_value = boost::bind(&wax::obj::operator[], subdev[named_prop_t(SUBDEV_PROP_GAIN, name)], _1);
        gg->register_fcns(fcns);
    }
    //add all the codec gains last (antenna to dsp order)
    BOOST_FOREACH(const std::string &name, codec[CODEC_PROP_GAIN_NAMES].as<prop_names_t>()){
        fcns.get_range = boost::bind(&wax::obj::as<gain_range_t>, codec[named_prop_t(CODEC_PROP_GAIN_RANGE, name)]);

        //register the value functions depending upon the connection type
        switch(subdev[SUBDEV_PROP_CONNECTION].as<subdev_conn_t>()){
        case SUBDEV_CONN_COMPLEX_IQ:
        case SUBDEV_CONN_COMPLEX_QI:
            fcns.get_value = boost::bind(&wax::obj::as<float>, codec[named_prop_t(CODEC_PROP_GAIN_I, name)]); //same as Q
            fcns.set_value = boost::bind(&gg_set_iq_value, codec, name, _1); //sets both
            break;

        case SUBDEV_CONN_REAL_I:
            fcns.get_value = boost::bind(&wax::obj::as<float>, codec[named_prop_t(CODEC_PROP_GAIN_I, name)]);
            fcns.set_value = boost::bind(&wax::obj::operator[], codec[named_prop_t(CODEC_PROP_GAIN_I, name)], _1);
            break;

        case SUBDEV_CONN_REAL_Q:
            fcns.get_value = boost::bind(&wax::obj::as<float>, codec[named_prop_t(CODEC_PROP_GAIN_Q, name)]);
            fcns.set_value = boost::bind(&wax::obj::operator[], codec[named_prop_t(CODEC_PROP_GAIN_Q, name)], _1);
            break;
        }
        gg->register_fcns(fcns);
    }
    return gg;
}
