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

#include <uhd/usrp/misc_utils.hpp>
#include <uhd/utils/assert_has.hpp>
#include <uhd/utils/algorithm.hpp>
#include <uhd/utils/gain_group.hpp>
#include <uhd/usrp/dboard_eeprom.hpp>
#include <uhd/usrp/subdev_props.hpp>
#include <uhd/usrp/mboard_props.hpp>
#include <uhd/usrp/dboard_props.hpp>
#include <uhd/usrp/codec_props.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>

using namespace uhd;
using namespace uhd::usrp;

/***********************************************************************
 * codec gain group helper functions:
 *    do this so we dont have to bind a templated function
 **********************************************************************/
static gain_range_t get_codec_gain_range(wax::obj codec, const std::string &name){
    return codec[named_prop_t(CODEC_PROP_GAIN_RANGE, name)].as<gain_range_t>();
}

static double get_codec_gain_i(wax::obj codec, const std::string &name){
    return codec[named_prop_t(CODEC_PROP_GAIN_I, name)].as<double>();
}

static double get_codec_gain_q(wax::obj codec, const std::string &name){
    return codec[named_prop_t(CODEC_PROP_GAIN_Q, name)].as<double>();
}

static void set_codec_gain_both(wax::obj codec, const std::string &name, double gain){
    codec[named_prop_t(CODEC_PROP_GAIN_I, name)] = gain;
    codec[named_prop_t(CODEC_PROP_GAIN_Q, name)] = gain;
}

static void set_codec_gain_i(wax::obj codec, const std::string &name, double gain){
    codec[named_prop_t(CODEC_PROP_GAIN_I, name)] = gain;
}

static void set_codec_gain_q(wax::obj codec, const std::string &name, double gain){
    codec[named_prop_t(CODEC_PROP_GAIN_Q, name)] = gain;
}

/***********************************************************************
 * subdev gain group helper functions:
 *    do this so we dont have to bind a templated function
 **********************************************************************/
static double get_subdev_gain(wax::obj subdev, const std::string &name){
    return subdev[named_prop_t(SUBDEV_PROP_GAIN, name)].as<double>();
}

static gain_range_t get_subdev_gain_range(wax::obj subdev, const std::string &name){
    return subdev[named_prop_t(SUBDEV_PROP_GAIN_RANGE, name)].as<gain_range_t>();
}

static void set_subdev_gain(wax::obj subdev, const std::string &name, double gain){
    subdev[named_prop_t(SUBDEV_PROP_GAIN, name)] = gain;
}

/***********************************************************************
 * gain group factory function for usrp
 **********************************************************************/
gain_group::sptr usrp::make_gain_group(
    const dboard_id_t &dboard_id,
    wax::obj subdev, wax::obj codec,
    gain_group_policy_t gain_group_policy
){
    const size_t subdev_gain_priority = 1;
    const size_t codec_gain_priority = (gain_group_policy == GAIN_GROUP_POLICY_RX)?
        (subdev_gain_priority - 1): //RX policy, codec gains fill last (lower priority)
        (subdev_gain_priority + 1); //TX policy, codec gains fill first (higher priority)
    const std::string subdev_prefix = dboard_id.to_cname() + "-";
    const std::string codec_prefix = (gain_group_policy == GAIN_GROUP_POLICY_RX)? "ADC-" : "DAC-";

    gain_group::sptr gg = gain_group::make();
    gain_fcns_t fcns;
    //add all the subdev gains first (antenna to dsp order)
    BOOST_FOREACH(const std::string &name, subdev[SUBDEV_PROP_GAIN_NAMES].as<prop_names_t>()){
        fcns.get_range = boost::bind(&get_subdev_gain_range, subdev, name);
        fcns.get_value = boost::bind(&get_subdev_gain, subdev, name);
        fcns.set_value = boost::bind(&set_subdev_gain, subdev, name, _1);
        gg->register_fcns(subdev_prefix+name, fcns, subdev_gain_priority);
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
        gg->register_fcns(codec_prefix+name, fcns, codec_gain_priority);
    }
    return gg;
}

/***********************************************************************
 * verify subdev specs
 **********************************************************************/
static void verify_xx_subdev_spec(
    mboard_prop_t dboard_names_prop,
    mboard_prop_t dboard_prop,
    subdev_spec_t &subdev_spec,
    wax::obj mboard,
    std::string xx_type
){
    try{
        prop_names_t dboard_names = mboard[dboard_names_prop].as<prop_names_t>();
        UHD_ASSERT_THROW(dboard_names.size() > 0); //well i hope there is a dboard

        //the subdevice specification is empty: handle automatic
        if (subdev_spec.empty()){
            BOOST_FOREACH(const std::string &db_name, dboard_names){
                wax::obj dboard = mboard[named_prop_t(dboard_prop, db_name)];

                //if the dboard slot is populated, take the first subdevice
                if (dboard[DBOARD_PROP_DBOARD_EEPROM].as<dboard_eeprom_t>().id != dboard_id_t::none()){
                    std::string sd_name = dboard[DBOARD_PROP_SUBDEV_NAMES].as<prop_names_t>().front();
                    subdev_spec.push_back(subdev_spec_pair_t(db_name, sd_name));
                    break;
                }
            }

            //didnt find any populated dboards: add the first subdevice
            if (subdev_spec.empty()){
                std::string db_name = dboard_names.front();
                wax::obj dboard = mboard[named_prop_t(dboard_prop, db_name)];
                std::string sd_name = dboard[DBOARD_PROP_SUBDEV_NAMES].as<prop_names_t>().front();
                subdev_spec.push_back(subdev_spec_pair_t(db_name, sd_name));
            }
        }

        //sanity check that the dboard/subdevice names exist for this mboard
        BOOST_FOREACH(subdev_spec_pair_t &pair, subdev_spec){
            //empty db name means select dboard automatically
            if (pair.db_name.empty()){
                if (dboard_names.size() != 1) throw uhd::value_error(
                    "A daughterboard name must be provided for multi-slot motherboards: " + subdev_spec.to_string()
                );
                pair.db_name = dboard_names.front();
            }
            uhd::assert_has(dboard_names, pair.db_name, xx_type + " dboard name");
            wax::obj dboard = mboard[named_prop_t(dboard_prop, pair.db_name)];
            prop_names_t subdev_names = dboard[DBOARD_PROP_SUBDEV_NAMES].as<prop_names_t>();

            //empty sd name means select the subdev automatically
            if (pair.sd_name.empty()){
                if (subdev_names.size() != 1) throw uhd::value_error(
                    "A subdevice name must be provided for multi-subdev daughterboards: " + subdev_spec.to_string()
                );
                pair.sd_name = subdev_names.front();
            }
            uhd::assert_has(subdev_names, pair.sd_name, xx_type + " subdev name");
        }
    }catch(const std::exception &e){
        throw uhd::value_error(str(boost::format(
            "Validate %s subdev spec failed: %s\n    %s"
        ) % xx_type % subdev_spec.to_string() % e.what()));
    }

    //now use the subdev spec to enable the subdevices in use and vice-versa
    BOOST_FOREACH(const std::string &db_name, mboard[dboard_names_prop].as<prop_names_t>()){
        wax::obj dboard = mboard[named_prop_t(dboard_prop, db_name)];
        BOOST_FOREACH(const std::string &sd_name, dboard[DBOARD_PROP_SUBDEV_NAMES].as<prop_names_t>()){
            try{
                bool enable = std::has(subdev_spec, subdev_spec_pair_t(db_name, sd_name));
                dboard[named_prop_t(DBOARD_PROP_SUBDEV, sd_name)][SUBDEV_PROP_ENABLED] = enable;
            }
            catch(const std::exception &e){
                throw uhd::runtime_error(str(boost::format(
                    "Cannot set enabled property on subdevice %s:%s\n    %s"
                ) % db_name % sd_name % e.what()));
            }
        }
    }
}

void usrp::verify_rx_subdev_spec(subdev_spec_t &subdev_spec, wax::obj mboard){
    return verify_xx_subdev_spec(
        MBOARD_PROP_RX_DBOARD_NAMES,
        MBOARD_PROP_RX_DBOARD,
        subdev_spec, mboard, "rx"
    );
}

void usrp::verify_tx_subdev_spec(subdev_spec_t &subdev_spec, wax::obj mboard){
    return verify_xx_subdev_spec(
        MBOARD_PROP_TX_DBOARD_NAMES,
        MBOARD_PROP_TX_DBOARD,
        subdev_spec, mboard, "tx"
    );
}
