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

#include <uhd/usrp/subdev_props.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/utils/assert.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/usrp/dboard_base.hpp>
#include <uhd/usrp/dboard_manager.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/format.hpp>

using namespace uhd;
using namespace uhd::usrp;
using namespace boost::assign;

/***********************************************************************
 * The unknown boards:
 *   Like a basic board, but with only one subdev.
 **********************************************************************/
class unknown_rx : public rx_dboard_base{
public:
    unknown_rx(ctor_args_t args);
    ~unknown_rx(void);

    void rx_get(const wax::obj &key, wax::obj &val);
    void rx_set(const wax::obj &key, const wax::obj &val);
};

class unknown_tx : public tx_dboard_base{
public:
    unknown_tx(ctor_args_t args);
    ~unknown_tx(void);

    void tx_get(const wax::obj &key, wax::obj &val);
    void tx_set(const wax::obj &key, const wax::obj &val);
};

/***********************************************************************
 * Register the unknown dboards
 **********************************************************************/
static dboard_base::sptr make_unknown_rx(dboard_base::ctor_args_t args){
    return dboard_base::sptr(new unknown_rx(args));
}

static dboard_base::sptr make_unknown_tx(dboard_base::ctor_args_t args){
    return dboard_base::sptr(new unknown_tx(args));
}

UHD_STATIC_BLOCK(reg_unknown_dboards){
    dboard_manager::register_dboard(0xfff0, &make_unknown_tx, "Unknown TX");
    dboard_manager::register_dboard(0xfff1, &make_unknown_rx, "Unknown RX");
}

/***********************************************************************
 * Unknown RX dboard
 **********************************************************************/
unknown_rx::unknown_rx(ctor_args_t args) : rx_dboard_base(args){
    /* NOP */
}

unknown_rx::~unknown_rx(void){
    /* NOP */
}

void unknown_rx::rx_get(const wax::obj &key_, wax::obj &val){
    named_prop_t key = named_prop_t::extract(key_);

    //handle the get request conditioned on the key
    switch(key.as<subdev_prop_t>()){
    case SUBDEV_PROP_NAME:
        val = "Unknown - " + get_rx_id().to_pp_string();
        return;

    case SUBDEV_PROP_OTHERS:
        val = prop_names_t(); //empty
        return;

    case SUBDEV_PROP_GAIN:
        val = float(0);
        return;

    case SUBDEV_PROP_GAIN_RANGE:
        val = gain_range_t(0, 0, 0);
        return;

    case SUBDEV_PROP_GAIN_NAMES:
        val = prop_names_t(); //empty
        return;

    case SUBDEV_PROP_FREQ:
        val = double(0);
        return;

    case SUBDEV_PROP_FREQ_RANGE:
        val = freq_range_t(0, 0);
        return;

    case SUBDEV_PROP_ANTENNA:
        val = std::string("");
        return;

    case SUBDEV_PROP_ANTENNA_NAMES:
        val = prop_names_t(1, ""); //vector of 1 empty string
        return;

    case SUBDEV_PROP_CONNECTION:
        val = SUBDEV_CONN_COMPLEX_IQ;
        return;

    case SUBDEV_PROP_USE_LO_OFFSET:
        val = false;
        return;

    case SUBDEV_PROP_LO_LOCKED:
        val = true; //there is no LO, so it must be true!
        return;

    default: UHD_THROW_PROP_GET_ERROR();
    }
}

void unknown_rx::rx_set(const wax::obj &key_, const wax::obj &val){
    named_prop_t key = named_prop_t::extract(key_);

    //handle the get request conditioned on the key
    switch(key.as<subdev_prop_t>()){

    case SUBDEV_PROP_GAIN:
        UHD_ASSERT_THROW(val.as<float>() == float(0));
        return;

    case SUBDEV_PROP_ANTENNA:
        UHD_ASSERT_THROW(val.as<std::string>() == std::string(""));
        return;

    case SUBDEV_PROP_FREQ:
        return; // it wont do you much good, but you can set it

    default: UHD_THROW_PROP_SET_ERROR();
    }
}

/***********************************************************************
 * Basic and LF TX dboard
 **********************************************************************/
unknown_tx::unknown_tx(ctor_args_t args) : tx_dboard_base(args){
    /* NOP */
}

unknown_tx::~unknown_tx(void){
    /* NOP */
}

void unknown_tx::tx_get(const wax::obj &key_, wax::obj &val){
    named_prop_t key = named_prop_t::extract(key_);

    //handle the get request conditioned on the key
    switch(key.as<subdev_prop_t>()){
    case SUBDEV_PROP_NAME:
        val = "Unknown - " + get_tx_id().to_pp_string();
        return;

    case SUBDEV_PROP_OTHERS:
        val = prop_names_t(); //empty
        return;

    case SUBDEV_PROP_GAIN:
        val = float(0);
        return;

    case SUBDEV_PROP_GAIN_RANGE:
        val = gain_range_t(0, 0, 0);
        return;

    case SUBDEV_PROP_GAIN_NAMES:
        val = prop_names_t(); //empty
        return;

    case SUBDEV_PROP_FREQ:
        val = double(0);
        return;

    case SUBDEV_PROP_FREQ_RANGE:
        val = freq_range_t(0, 0);
        return;

    case SUBDEV_PROP_ANTENNA:
        val = std::string("");
        return;

    case SUBDEV_PROP_ANTENNA_NAMES:
        val = prop_names_t(1, ""); //vector of 1 empty string
        return;

    case SUBDEV_PROP_CONNECTION:
        val = SUBDEV_CONN_COMPLEX_IQ;
        return;

    case SUBDEV_PROP_USE_LO_OFFSET:
        val = false;
        return;

    case SUBDEV_PROP_LO_LOCKED:
        val = true; //there is no LO, so it must be true!
        return;

    default: UHD_THROW_PROP_GET_ERROR();
    }
}

void unknown_tx::tx_set(const wax::obj &key_, const wax::obj &val){
    named_prop_t key = named_prop_t::extract(key_);

    //handle the get request conditioned on the key
    switch(key.as<subdev_prop_t>()){

    case SUBDEV_PROP_GAIN:
        UHD_ASSERT_THROW(val.as<float>() == float(0));
        return;

    case SUBDEV_PROP_ANTENNA:
        UHD_ASSERT_THROW(val.as<std::string>() == std::string(""));
        return;

    case SUBDEV_PROP_FREQ:
        return; // it wont do you much good, but you can set it

    default: UHD_THROW_PROP_SET_ERROR();
    }
}
