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

#include <uhd/utils/static.hpp>
#include <uhd/utils/assert.hpp>
#include <uhd/utils/algorithm.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/usrp/subdev_props.hpp>
#include <uhd/usrp/dboard_base.hpp>
#include <uhd/usrp/dboard_manager.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/format.hpp>
#include <boost/math/special_functions/round.hpp>
#include <utility>

using namespace uhd;
using namespace uhd::usrp;
using namespace boost::assign;

/***********************************************************************
 * The XCVR 2450 dboard
 **********************************************************************/
static const freq_range_t xcvr_freq_range(2.4e9, 6.0e9);

class xcvr2450 : public xcvr_dboard_base{
public:
    xcvr2450(ctor_args_t const& args);
    ~xcvr2450(void);

    void rx_get(const wax::obj &key, wax::obj &val);
    void rx_set(const wax::obj &key, const wax::obj &val);

    void tx_get(const wax::obj &key, wax::obj &val);
    void tx_set(const wax::obj &key, const wax::obj &val);

private:
    double _lo_freq;
    uhd::dict<std::string, float> _tx_gains, _rx_gains;
    std::string _tx_ant, _rx_ant;

    void set_lo_freq(double target_freq){}
    void set_tx_ant(const std::string &ant){}
    void set_rx_ant(const std::string &ant){}
    void set_tx_gain(float gain, const std::string &name){}
    void set_rx_gain(float gain, const std::string &name){}
};

/***********************************************************************
 * Register the XCVR dboard
 **********************************************************************/
static dboard_base::sptr make_xcvr2450(dboard_base::ctor_args_t const& args){
    return dboard_base::sptr(new xcvr2450(args));
}

UHD_STATIC_BLOCK(reg_xcvr2450_dboard){
    //register the factory function for the rx and tx dbids
    dboard_manager::register_dboard(0x0060, &make_xcvr2450, "XCVR2450 TX");
    dboard_manager::register_dboard(0x0061, &make_xcvr2450, "XCVR2450 RX");
}

/***********************************************************************
 * Structors
 **********************************************************************/
xcvr2450::xcvr2450(ctor_args_t const& args) : xcvr_dboard_base(args){
    /* NOP */
}

xcvr2450::~xcvr2450(void){
    /* NOP */
}

/***********************************************************************
 * Antenna Handling
 **********************************************************************/
static const prop_names_t xcvr_antennas = list_of("J1")("J2");

/***********************************************************************
 * Gain Handling
 **********************************************************************/
static const uhd::dict<std::string, gain_range_t> xcvr_tx_gain_ranges = map_list_of
    ("VGA", gain_range_t(0, 30, 0.5))
;
static const uhd::dict<std::string, gain_range_t> xcvr_rx_gain_ranges = map_list_of
    ("RF LNA", gain_range_t(0, 30.5, 15))
    ("BB VGA", gain_range_t(0, 62, 2.0))
;

/*!
 * Convert a requested gain for the tx vga into the integer register value.
 * The gain passed into the function will be set to the actual value.
 * \param gain the requested gain in dB
 * \return 6 bit the register value
 */
static int gain_to_tx_vga_reg(float &gain){
    //calculate the register value
    int reg = std::clip(boost::math::iround(gain*60/30.0) + 3, 0, 63);

    //calculate the actual gain value
    if (reg < 4)       gain = 0;
    else if (reg < 48) gain = reg/2 - 1;
    else               gain = reg/2.0 - 1.5;

    //return register value
    return reg;
}

/*!
 * Convert a requested gain for the rx vga into the integer register value.
 * The gain passed into the function will be set to the actual value.
 * \param gain the requested gain in dB
 * \return 5 bit the register value
 */
static int gain_to_rx_bb_vga_reg(float &gain){
    int reg = std::clip(boost::math::iround(gain/2.0), 0, 31);
    gain = reg*2;
    return reg;
}

/*!
 * Convert a requested gain for the rx lna into the integer register value.
 * The gain passed into the function will be set to the actual value.
 * \param gain the requested gain in dB
 * \return 2 bit the register value
 */
static int gain_to_rx_rf_lna_reg(float &gain){
    int reg = std::clip(boost::math::iround(gain*2/30.5) + 1, 0, 3);
    switch(reg){
    case 0:
    case 1: gain = 0;    break;
    case 2: gain = 15;   break;
    case 3: gain = 30.5; break;
    }
    return reg;
}

/***********************************************************************
 * RX Get and Set
 **********************************************************************/
void xcvr2450::rx_get(const wax::obj &key_, wax::obj &val){
    wax::obj key; std::string name;
    boost::tie(key, name) = extract_named_prop(key_);

    //handle the get request conditioned on the key
    switch(key.as<subdev_prop_t>()){
    case SUBDEV_PROP_NAME:
        val = dboard_id::to_string(get_rx_id());
        return;

    case SUBDEV_PROP_OTHERS:
        val = prop_names_t(); //empty
        return;

    case SUBDEV_PROP_GAIN:
        assert_has(_rx_gains.keys(), name, "xcvr rx gain name");
        val = _rx_gains[name];
        return;

    case SUBDEV_PROP_GAIN_RANGE:
        assert_has(xcvr_rx_gain_ranges.keys(), name, "xcvr rx gain name");
        val = xcvr_rx_gain_ranges[name];
        return;

    case SUBDEV_PROP_GAIN_NAMES:
        val = prop_names_t(xcvr_rx_gain_ranges.keys());
        return;

    case SUBDEV_PROP_FREQ:
        val = _lo_freq;
        return;

    case SUBDEV_PROP_FREQ_RANGE:
        val = xcvr_freq_range;
        return;

    case SUBDEV_PROP_ANTENNA:
        val = _rx_ant;
        return;

    case SUBDEV_PROP_ANTENNA_NAMES:
        val = xcvr_antennas;
        return;

    case SUBDEV_PROP_QUADRATURE:
        val = true;
        return;

    case SUBDEV_PROP_IQ_SWAPPED:
        val = false;
        return;

    case SUBDEV_PROP_SPECTRUM_INVERTED:
        val = false;
        return;

    case SUBDEV_PROP_USE_LO_OFFSET:
        val = false;
        return;
    }
}

void xcvr2450::rx_set(const wax::obj &key_, const wax::obj &val){
    wax::obj key; std::string name;
    boost::tie(key, name) = extract_named_prop(key_);

    //handle the get request conditioned on the key
    switch(key.as<subdev_prop_t>()){

    case SUBDEV_PROP_FREQ:
        this->set_lo_freq(val.as<double>());
        return;

    case SUBDEV_PROP_GAIN:
        this->set_rx_gain(val.as<float>(), name);
        return;

    case SUBDEV_PROP_ANTENNA:
        this->set_rx_ant(val.as<std::string>());
        return;

    default:
        throw std::runtime_error(str(boost::format(
            "Error: trying to set read-only property on %s subdev"
        ) % dboard_id::to_string(get_rx_id())));
    }
}

/***********************************************************************
 * TX Get and Set
 **********************************************************************/
void xcvr2450::tx_get(const wax::obj &key_, wax::obj &val){
    wax::obj key; std::string name;
    boost::tie(key, name) = extract_named_prop(key_);

    //handle the get request conditioned on the key
    switch(key.as<subdev_prop_t>()){
    case SUBDEV_PROP_NAME:
        val = dboard_id::to_string(get_tx_id());
        return;

    case SUBDEV_PROP_OTHERS:
        val = prop_names_t(); //empty
        return;

    case SUBDEV_PROP_GAIN:
        assert_has(_tx_gains.keys(), name, "xcvr tx gain name");
        val = _tx_gains[name];
        return;

    case SUBDEV_PROP_GAIN_RANGE:
        assert_has(xcvr_tx_gain_ranges.keys(), name, "xcvr tx gain name");
        val = xcvr_tx_gain_ranges[name];
        return;

    case SUBDEV_PROP_GAIN_NAMES:
        val = prop_names_t(xcvr_tx_gain_ranges.keys());
        return;

    case SUBDEV_PROP_FREQ:
        val = _lo_freq;
        return;

    case SUBDEV_PROP_FREQ_RANGE:
        val = xcvr_freq_range;
        return;

    case SUBDEV_PROP_ANTENNA:
        val = _tx_ant;
        return;

    case SUBDEV_PROP_ANTENNA_NAMES:
        val = xcvr_antennas;
        return;

    case SUBDEV_PROP_QUADRATURE:
        val = true;
        return;

    case SUBDEV_PROP_IQ_SWAPPED:
        val = true;
        return;

    case SUBDEV_PROP_SPECTRUM_INVERTED:
        val = false;
        return;

    case SUBDEV_PROP_USE_LO_OFFSET:
        val = false;
        return;
    }
}

void xcvr2450::tx_set(const wax::obj &key_, const wax::obj &val){
    wax::obj key; std::string name;
    boost::tie(key, name) = extract_named_prop(key_);

    //handle the get request conditioned on the key
    switch(key.as<subdev_prop_t>()){

    case SUBDEV_PROP_FREQ:
        set_lo_freq(val.as<double>());
        return;

    case SUBDEV_PROP_GAIN:
        this->set_tx_gain(val.as<float>(), name);
        return;

    case SUBDEV_PROP_ANTENNA:
        this->set_tx_ant(val.as<std::string>());
        return;

    default:
        throw std::runtime_error(str(boost::format(
            "Error: trying to set read-only property on %s subdev"
        ) % dboard_id::to_string(get_tx_id())));
    }
}
