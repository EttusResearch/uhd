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
#include <uhd/types/dict.hpp>
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
 * The basic and lf boards:
 *   They share a common class because only the frequency bounds differ.
 **********************************************************************/
class basic_rx : public rx_dboard_base{
public:
    basic_rx(ctor_args_t args, double max_freq);
    ~basic_rx(void);

    void rx_get(const wax::obj &key, wax::obj &val);
    void rx_set(const wax::obj &key, const wax::obj &val);

private:
    double _max_freq;
};

class basic_tx : public tx_dboard_base{
public:
    basic_tx(ctor_args_t args, double max_freq);
    ~basic_tx(void);

    void tx_get(const wax::obj &key, wax::obj &val);
    void tx_set(const wax::obj &key, const wax::obj &val);

private:
    double _max_freq;
};

static const uhd::dict<std::string, subdev_conn_t> sd_name_to_conn = map_list_of
    ("AB", SUBDEV_CONN_COMPLEX_IQ)
    ("A",  SUBDEV_CONN_REAL_I)
    ("B",  SUBDEV_CONN_REAL_Q)
;

/***********************************************************************
 * Register the basic and LF dboards
 **********************************************************************/
static dboard_base::sptr make_basic_rx(dboard_base::ctor_args_t args){
    return dboard_base::sptr(new basic_rx(args, 90e9));
}

static dboard_base::sptr make_basic_tx(dboard_base::ctor_args_t args){
    return dboard_base::sptr(new basic_tx(args, 90e9));
}

static dboard_base::sptr make_lf_rx(dboard_base::ctor_args_t args){
    return dboard_base::sptr(new basic_rx(args, 32e6));
}

static dboard_base::sptr make_lf_tx(dboard_base::ctor_args_t args){
    return dboard_base::sptr(new basic_tx(args, 32e6));
}

UHD_STATIC_BLOCK(reg_basic_and_lf_dboards){
    dboard_manager::register_dboard(0x0000, &make_basic_tx, "Basic TX", sd_name_to_conn.keys());
    dboard_manager::register_dboard(0x0001, &make_basic_rx, "Basic RX", sd_name_to_conn.keys());
    dboard_manager::register_dboard(0x000e, &make_lf_tx,    "LF TX",    sd_name_to_conn.keys());
    dboard_manager::register_dboard(0x000f, &make_lf_rx,    "LF RX",    sd_name_to_conn.keys());
}

/***********************************************************************
 * Basic and LF RX dboard
 **********************************************************************/
basic_rx::basic_rx(ctor_args_t args, double max_freq) : rx_dboard_base(args){
    _max_freq = max_freq;
}

basic_rx::~basic_rx(void){
    /* NOP */
}

void basic_rx::rx_get(const wax::obj &key_, wax::obj &val){
    named_prop_t key = named_prop_t::extract(key_);

    //handle the get request conditioned on the key
    switch(key.as<subdev_prop_t>()){
    case SUBDEV_PROP_NAME:
        val = std::string(str(boost::format("%s - %s")
            % get_rx_id().to_pp_string()
            % get_subdev_name()
        ));
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
        val = freq_range_t(-_max_freq, +_max_freq);
        return;

    case SUBDEV_PROP_ANTENNA:
        val = std::string("");
        return;

    case SUBDEV_PROP_ANTENNA_NAMES:
        val = prop_names_t(1, ""); //vector of 1 empty string
        return;

    case SUBDEV_PROP_CONNECTION:
        val = sd_name_to_conn[get_subdev_name()];
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

void basic_rx::rx_set(const wax::obj &key_, const wax::obj &val){
    named_prop_t key = named_prop_t::extract(key_);

    //handle the get request conditioned on the key
    switch(key.as<subdev_prop_t>()){

    case SUBDEV_PROP_GAIN:
        UHD_ASSERT_THROW(val.as<float>() == float(0));
        return;

    case SUBDEV_PROP_ANTENNA:
        if (val.as<std::string>().empty()) return;
        throw std::runtime_error("no selectable antennas on this board");

    case SUBDEV_PROP_FREQ:
        return; // it wont do you much good, but you can set it

    default: UHD_THROW_PROP_SET_ERROR();
    }
}

/***********************************************************************
 * Basic and LF TX dboard
 **********************************************************************/
basic_tx::basic_tx(ctor_args_t args, double max_freq) : tx_dboard_base(args){
    _max_freq = max_freq;
}

basic_tx::~basic_tx(void){
    /* NOP */
}

void basic_tx::tx_get(const wax::obj &key_, wax::obj &val){
    named_prop_t key = named_prop_t::extract(key_);

    //handle the get request conditioned on the key
    switch(key.as<subdev_prop_t>()){
    case SUBDEV_PROP_NAME:
        val = std::string(str(boost::format("%s - %s")
            % get_tx_id().to_pp_string()
            % get_subdev_name()
        ));
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
        val = freq_range_t(-_max_freq, +_max_freq);
        return;

    case SUBDEV_PROP_ANTENNA:
        val = std::string("");
        return;

    case SUBDEV_PROP_ANTENNA_NAMES:
        val = prop_names_t(1, ""); //vector of 1 empty string
        return;

    case SUBDEV_PROP_CONNECTION:
        val = sd_name_to_conn[get_subdev_name()];
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

void basic_tx::tx_set(const wax::obj &key_, const wax::obj &val){
    named_prop_t key = named_prop_t::extract(key_);

    //handle the get request conditioned on the key
    switch(key.as<subdev_prop_t>()){

    case SUBDEV_PROP_GAIN:
        UHD_ASSERT_THROW(val.as<float>() == float(0));
        return;

    case SUBDEV_PROP_ANTENNA:
        if (val.as<std::string>().empty()) return;
        throw std::runtime_error("no selectable antennas on this board");

    case SUBDEV_PROP_FREQ:
        return; // it wont do you much good, but you can set it

    default: UHD_THROW_PROP_SET_ERROR();
    }
}
