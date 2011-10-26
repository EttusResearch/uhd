//
// Copyright 2011 Ettus Research LLC
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

#include "db_wbx_common.hpp"
#include "adf4350_regs.hpp"
#include <uhd/types/dict.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/types/sensors.hpp>
#include <uhd/utils/assert_has.hpp>
#include <uhd/utils/algorithm.hpp>
#include <uhd/utils/msg.hpp>

using namespace uhd;
using namespace uhd::usrp;
using namespace boost::assign;


/***********************************************************************
 * Gain-related functions
 **********************************************************************/
static int rx_pga0_gain_to_iobits(double &gain){
    //clip the input
    gain = wbx_rx_gain_ranges["PGA0"].clip(gain);

    //convert to attenuation
    double attn = wbx_rx_gain_ranges["PGA0"].stop() - gain;

    //calculate the attenuation
    int attn_code = boost::math::iround(attn*2);
    int iobits = ((~attn_code) << RX_ATTN_SHIFT) & RX_ATTN_MASK;

    UHD_LOGV(often) << boost::format(
        "WBX RX Attenuation: %f dB, Code: %d, IO Bits %x, Mask: %x"
    ) % attn % attn_code % (iobits & RX_ATTN_MASK) % RX_ATTN_MASK << std::endl;

    //the actual gain setting
    gain = wbx_rx_gain_ranges["PGA0"].stop() - double(attn_code)/2;

    return iobits;
}


/***********************************************************************
 * WBX Common Implementation
 **********************************************************************/
wbx_base::wbx_base(ctor_args_t args) : xcvr_dboard_base(args){
    switch(get_rx_id().to_uint16()) {
        case 0x053:
            db_actual = wbx_versionx_sptr(new wbx_version2(this));
            return;
        case 0x057:
            db_actual = wbx_versionx_sptr(new wbx_version3(this));
            return;
        case 0x063:
            db_actual = wbx_versionx_sptr(new wbx_version4(this));
            return;
        default:
            /* We didn't recognize the version of the board... */
            UHD_THROW_INVALID_CODE_PATH();
    }
}


wbx_base::~wbx_base(void){
    /* NOP */
}

/***********************************************************************
 * Enables
 **********************************************************************/
void wbx_base::set_rx_enabled(bool enb){
    this->get_iface()->set_gpio_out(dboard_iface::UNIT_RX,
        (enb)? RX_POWER_UP : RX_POWER_DOWN, RX_POWER_UP | RX_POWER_DOWN
    );
}

void wbx_base::set_tx_enabled(bool enb){
    db_actual->set_tx_enabled(enb);
}

/***********************************************************************
 * Gain Handling
 **********************************************************************/
void wbx_base::set_tx_gain(double gain, const std::string &name){
    db_actual->set_tx_gain(gain, name);
}

void wbx_base::set_rx_gain(double gain, const std::string &name){
    assert_has(wbx_rx_gain_ranges.keys(), name, "wbx rx gain name");
    if(name == "PGA0"){
        boost::uint16_t io_bits = rx_pga0_gain_to_iobits(gain);
        _rx_gains[name] = gain;

        //write the new gain to rx gpio outputs
        this->get_iface()->set_gpio_out(dboard_iface::UNIT_RX, io_bits, RX_ATTN_MASK);
    }
    else UHD_THROW_INVALID_CODE_PATH();
}

/***********************************************************************
 * Tuning
 **********************************************************************/
freq_range_t wbx_base::get_freq_range(void) {
    return db_actual->get_freq_range();
}

double wbx_base::set_lo_freq(dboard_iface::unit_t unit, double target_freq) {
    return db_actual->set_lo_freq(unit, target_freq);
}

bool wbx_base::get_locked(dboard_iface::unit_t unit){
    return (this->get_iface()->read_gpio(unit) & LOCKDET_MASK) != 0;
}


/***********************************************************************
 * RX Get and Set
 **********************************************************************/
void wbx_base::rx_get(const wax::obj &key_, wax::obj &val){
    named_prop_t key = named_prop_t::extract(key_);

    //handle the get request conditioned on the key
    switch(key.as<subdev_prop_t>()){
    case SUBDEV_PROP_NAME:
        val = get_rx_id().to_pp_string();
        return;

    case SUBDEV_PROP_OTHERS:
        val = prop_names_t(); //empty
        return;

    case SUBDEV_PROP_GAIN:
        assert_has(_rx_gains.keys(), key.name, "wbx rx gain name");
        val = _rx_gains[key.name];
        return;

    case SUBDEV_PROP_GAIN_RANGE:
        assert_has(wbx_rx_gain_ranges.keys(), key.name, "wbx rx gain name");
        val = wbx_rx_gain_ranges[key.name];
        return;

    case SUBDEV_PROP_GAIN_NAMES:
        val = prop_names_t(wbx_rx_gain_ranges.keys());
        return;

    case SUBDEV_PROP_FREQ:
        val = 0.0;
        return;

    case SUBDEV_PROP_FREQ_RANGE:
        val = freq_range_t(0.0, 0.0, 0.0);;
        return;

    case SUBDEV_PROP_ANTENNA:
        val = std::string("");
        return;

    case SUBDEV_PROP_ANTENNA_NAMES:
        val = prop_names_t(1, "");
        return;

    case SUBDEV_PROP_CONNECTION:
        val = SUBDEV_CONN_COMPLEX_IQ;
        return;

    case SUBDEV_PROP_ENABLED:
        val = _rx_enabled;
        return;

    case SUBDEV_PROP_USE_LO_OFFSET:
        val = false;
        return;

    case SUBDEV_PROP_SENSOR:
        UHD_ASSERT_THROW(key.name == "lo_locked");
        val = sensor_value_t("LO", this->get_locked(dboard_iface::UNIT_RX), "locked", "unlocked");
        return;

    case SUBDEV_PROP_SENSOR_NAMES:
        val = prop_names_t(1, "lo_locked");
        return;

    case SUBDEV_PROP_BANDWIDTH:
        val = 2*20.0e6; //20MHz low-pass, we want complex double-sided
        return;

    default: UHD_THROW_PROP_GET_ERROR();
    }
}

void wbx_base::rx_set(const wax::obj &key_, const wax::obj &val){
    named_prop_t key = named_prop_t::extract(key_);

    //handle the get request conditioned on the key
    switch(key.as<subdev_prop_t>()){

    case SUBDEV_PROP_GAIN:
        this->set_rx_gain(val.as<double>(), key.name);
        return;

    case SUBDEV_PROP_ENABLED:
        _rx_enabled = val.as<bool>();
        this->set_rx_enabled(_rx_enabled);
        return;

    case SUBDEV_PROP_BANDWIDTH:
        UHD_MSG(warning) << "WBX: No tunable bandwidth, fixed filtered to 40MHz";
        return;

    default: UHD_THROW_PROP_SET_ERROR();
    }
}

/***********************************************************************
 * TX Get and Set
 **********************************************************************/
void wbx_base::tx_get(const wax::obj &key_, wax::obj &val){
    db_actual->tx_get(key_, val);
}

void wbx_base::tx_set(const wax::obj &key_, const wax::obj &val){
    named_prop_t key = named_prop_t::extract(key_);

    //handle the get request conditioned on the key
    switch(key.as<subdev_prop_t>()){

    case SUBDEV_PROP_GAIN:
        this->set_tx_gain(val.as<double>(), key.name);
        return;

    case SUBDEV_PROP_ENABLED:
        _tx_enabled = val.as<bool>();
        this->set_tx_enabled(_tx_enabled);
        return;

    case SUBDEV_PROP_BANDWIDTH:
        UHD_MSG(warning) << "WBX: No tunable bandwidth, fixed filtered to 40MHz";
        return;

    default: UHD_THROW_PROP_SET_ERROR();
    }
}


