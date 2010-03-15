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
#include <uhd/props.hpp>
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
    basic_rx(ctor_args_t const& args, freq_t max_freq);
    ~basic_rx(void);

    void rx_get(const wax::obj &key, wax::obj &val);
    void rx_set(const wax::obj &key, const wax::obj &val);

private:
    freq_t _max_freq;
};

class basic_tx : public tx_dboard_base{
public:
    basic_tx(ctor_args_t const& args, freq_t max_freq);
    ~basic_tx(void);

    void tx_get(const wax::obj &key, wax::obj &val);
    void tx_set(const wax::obj &key, const wax::obj &val);

private:
    freq_t _max_freq;
};

/***********************************************************************
 * Register the basic and LF dboards
 **********************************************************************/
static dboard_base::sptr make_basic_rx(dboard_base::ctor_args_t const& args){
    return dboard_base::sptr(new basic_rx(args, 90e9));
}

static dboard_base::sptr make_basic_tx(dboard_base::ctor_args_t const& args){
    return dboard_base::sptr(new basic_tx(args, 90e9));
}

static dboard_base::sptr make_lf_rx(dboard_base::ctor_args_t const& args){
    return dboard_base::sptr(new basic_rx(args, 32e6));
}

static dboard_base::sptr make_lf_tx(dboard_base::ctor_args_t const& args){
    return dboard_base::sptr(new basic_tx(args, 32e6));
}

STATIC_BLOCK(reg_dboards){
    dboard_manager::register_dboard(0x0000, &make_basic_tx, "Basic TX", list_of(""));
    dboard_manager::register_dboard(0x0001, &make_basic_rx, "Basic RX", list_of("a")("b")("ab"));
    dboard_manager::register_dboard(0x000e, &make_lf_tx,    "LF TX",    list_of(""));
    dboard_manager::register_dboard(0x000f, &make_lf_rx,    "LF RX",    list_of("a")("b")("ab"));
}

/***********************************************************************
 * Basic and LF RX dboard
 **********************************************************************/
basic_rx::basic_rx(ctor_args_t const& args, freq_t max_freq) : rx_dboard_base(args){
    _max_freq = max_freq;
    // set the gpios to safe values (all inputs)
    get_interface()->set_gpio_ddr(dboard_interface::GPIO_RX_BANK, 0x0000, 0xffff);
}

basic_rx::~basic_rx(void){
    /* NOP */
}

void basic_rx::rx_get(const wax::obj &key_, wax::obj &val){
    wax::obj key; std::string name;
    boost::tie(key, name) = extract_named_prop(key_);

    //handle the get request conditioned on the key
    switch(wax::cast<subdev_prop_t>(key)){
    case SUBDEV_PROP_NAME:
        val = std::string(str(boost::format("%s:%s")
            % dboard_id::to_string(get_rx_id())
            % get_subdev_name()
        ));
        return;

    case SUBDEV_PROP_OTHERS:
        val = prop_names_t(); //empty
        return;

    case SUBDEV_PROP_GAIN:
        val = gain_t(0);
        return;

    case SUBDEV_PROP_GAIN_RANGE:
        val = gain_range_t(0, 0, 0);
        return;

    case SUBDEV_PROP_GAIN_NAMES:
        val = prop_names_t(); //empty
        return;

    case SUBDEV_PROP_FREQ:
        val = freq_t(0);
        return;

    case SUBDEV_PROP_FREQ_RANGE:
        val = freq_range_t(+_max_freq, -_max_freq);
        return;

    case SUBDEV_PROP_ANTENNA:
        val = std::string("");
        return;

    case SUBDEV_PROP_ANTENNA_NAMES:
        val = prop_names_t(1, ""); //vector of 1 empty string
        return;

    case SUBDEV_PROP_ENABLED:
        val = true; //always enabled
        return;

    case SUBDEV_PROP_QUADRATURE:
        val = (get_subdev_name() == "ab"); //only quadrature in ab mode
        return;

    case SUBDEV_PROP_IQ_SWAPPED:
    case SUBDEV_PROP_SPECTRUM_INVERTED:
    case SUBDEV_PROP_LO_INTERFERES:
        val = false;
        return;
    }
}

void basic_rx::rx_set(const wax::obj &key_, const wax::obj &val){
    wax::obj key; std::string name;
    boost::tie(key, name) = extract_named_prop(key_);

    //handle the get request conditioned on the key
    switch(wax::cast<subdev_prop_t>(key)){

    case SUBDEV_PROP_GAIN:
        ASSERT_THROW(wax::cast<gain_t>(val) == gain_t(0));
        return;

    case SUBDEV_PROP_ANTENNA:
        ASSERT_THROW(wax::cast<std::string>(val) == std::string(""));
        return;

    case SUBDEV_PROP_ENABLED:
        return; // it wont do you much good, but you can set it

    case SUBDEV_PROP_NAME:
    case SUBDEV_PROP_OTHERS:
    case SUBDEV_PROP_GAIN_RANGE:
    case SUBDEV_PROP_GAIN_NAMES:
    case SUBDEV_PROP_FREQ:
    case SUBDEV_PROP_FREQ_RANGE:
    case SUBDEV_PROP_ANTENNA_NAMES:
    case SUBDEV_PROP_QUADRATURE:
    case SUBDEV_PROP_IQ_SWAPPED:
    case SUBDEV_PROP_SPECTRUM_INVERTED:
    case SUBDEV_PROP_LO_INTERFERES:
        throw std::runtime_error(str(boost::format(
            "Error: trying to set read-only property on %s subdev"
        ) % dboard_id::to_string(get_rx_id())));
    }
}

/***********************************************************************
 * Basic and LF TX dboard
 **********************************************************************/
basic_tx::basic_tx(ctor_args_t const& args, freq_t max_freq) : tx_dboard_base(args){
    _max_freq = max_freq;
    // set the gpios to safe values (all inputs)
    get_interface()->set_gpio_ddr(dboard_interface::GPIO_TX_BANK, 0x0000, 0xffff);
}

basic_tx::~basic_tx(void){
    /* NOP */
}

void basic_tx::tx_get(const wax::obj &key_, wax::obj &val){
    wax::obj key; std::string name;
    boost::tie(key, name) = extract_named_prop(key_);

    //handle the get request conditioned on the key
    switch(wax::cast<subdev_prop_t>(key)){
    case SUBDEV_PROP_NAME:
        val = dboard_id::to_string(get_tx_id());
        return;

    case SUBDEV_PROP_OTHERS:
        val = prop_names_t(); //empty
        return;

    case SUBDEV_PROP_GAIN:
        val = gain_t(0);
        return;

    case SUBDEV_PROP_GAIN_RANGE:
        val = gain_range_t(0, 0, 0);
        return;

    case SUBDEV_PROP_GAIN_NAMES:
        val = prop_names_t(); //empty
        return;

    case SUBDEV_PROP_FREQ:
        val = freq_t(0);
        return;

    case SUBDEV_PROP_FREQ_RANGE:
        val = freq_range_t(+_max_freq, -_max_freq);
        return;

    case SUBDEV_PROP_ANTENNA:
        val = std::string("");
        return;

    case SUBDEV_PROP_ANTENNA_NAMES:
        val = prop_names_t(1, ""); //vector of 1 empty string
        return;

    case SUBDEV_PROP_ENABLED:
        val = true; //always enabled
        return;

    case SUBDEV_PROP_QUADRATURE:
        val = true;
        return;

    case SUBDEV_PROP_IQ_SWAPPED:
    case SUBDEV_PROP_SPECTRUM_INVERTED:
    case SUBDEV_PROP_LO_INTERFERES:
        val = false;
        return;
    }
}

void basic_tx::tx_set(const wax::obj &key_, const wax::obj &val){
    wax::obj key; std::string name;
    boost::tie(key, name) = extract_named_prop(key_);

    //handle the get request conditioned on the key
    switch(wax::cast<subdev_prop_t>(key)){

    case SUBDEV_PROP_GAIN:
        ASSERT_THROW(wax::cast<gain_t>(val) == gain_t(0));
        return;

    case SUBDEV_PROP_ANTENNA:
        ASSERT_THROW(wax::cast<std::string>(val) == std::string(""));
        return;

    case SUBDEV_PROP_ENABLED:
        return; // it wont do you much good, but you can set it

    case SUBDEV_PROP_NAME:
    case SUBDEV_PROP_OTHERS:
    case SUBDEV_PROP_GAIN_RANGE:
    case SUBDEV_PROP_GAIN_NAMES:
    case SUBDEV_PROP_FREQ:
    case SUBDEV_PROP_FREQ_RANGE:
    case SUBDEV_PROP_ANTENNA_NAMES:
    case SUBDEV_PROP_QUADRATURE:
    case SUBDEV_PROP_IQ_SWAPPED:
    case SUBDEV_PROP_SPECTRUM_INVERTED:
    case SUBDEV_PROP_LO_INTERFERES:
        throw std::runtime_error(str(boost::format(
            "Error: trying to set read-only property on %s subdev"
        ) % dboard_id::to_string(get_tx_id())));
    }
}
