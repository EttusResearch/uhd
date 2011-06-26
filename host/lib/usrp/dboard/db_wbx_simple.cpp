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

// Antenna constants
#define ANTSW_IO        ((1 << 5)|(1 << 15))    // on UNIT_TX, 0 = TX, 1 = RX, on UNIT_RX 0 = main ant, 1 = RX2
#define ANT_TX          0                       //the tx line is transmitting
#define ANT_RX          ANTSW_IO                //the tx line is receiving
#define ANT_TXRX        0                       //the rx line is on txrx
#define ANT_RX2         ANTSW_IO                //the rx line in on rx2

#include "db_wbx_common.hpp"
#include <uhd/utils/static.hpp>
#include <uhd/utils/assert_has.hpp>
#include <uhd/usrp/dboard_manager.hpp>
#include <uhd/usrp/subdev_props.hpp>
#include <boost/assign/list_of.hpp>

using namespace uhd;
using namespace uhd::usrp;
using namespace boost::assign;

/***********************************************************************
 * The WBX Simple dboard constants
 **********************************************************************/
static const freq_range_t wbx_freq_range(68.75e6, 2.2e9);

static const prop_names_t wbx_tx_antennas = list_of("TX/RX");

static const prop_names_t wbx_rx_antennas = list_of("TX/RX")("RX2");

/***********************************************************************
 * The WBX simple implementation
 **********************************************************************/
class wbx_simple : public wbx_base{
public:
    wbx_simple(ctor_args_t args);
    ~wbx_simple(void);

    void rx_get(const wax::obj &key, wax::obj &val);
    void rx_set(const wax::obj &key, const wax::obj &val);

    void tx_get(const wax::obj &key, wax::obj &val);
    void tx_set(const wax::obj &key, const wax::obj &val);

private:
    void set_rx_lo_freq(double freq);
    void set_tx_lo_freq(double freq);
    double _rx_lo_freq, _tx_lo_freq;

    void set_rx_ant(const std::string &ant);
    void set_tx_ant(const std::string &ant);
    std::string _rx_ant;
};

/***********************************************************************
 * Register the WBX simple implementation
 **********************************************************************/
static dboard_base::sptr make_wbx_simple(dboard_base::ctor_args_t args){
    return dboard_base::sptr(new wbx_simple(args));
}

UHD_STATIC_BLOCK(reg_wbx_simple_dboards){
    dboard_manager::register_dboard(0x0053, 0x0052, &make_wbx_simple, "WBX");
    dboard_manager::register_dboard(0x0053, 0x004f, &make_wbx_simple, "WBX + Simple GDB");
    dboard_manager::register_dboard(0x0057, 0x0056, &make_wbx_simple, "WBX v3");
    dboard_manager::register_dboard(0x0057, 0x004f, &make_wbx_simple, "WBX v3 + Simple GDB");
}

/***********************************************************************
 * Structors
 **********************************************************************/
wbx_simple::wbx_simple(ctor_args_t args) : wbx_base(args){

    //set the gpio directions and atr controls (antenna switches all under ATR)
    this->get_iface()->set_pin_ctrl(dboard_iface::UNIT_TX, ANTSW_IO, ANTSW_IO);
    this->get_iface()->set_pin_ctrl(dboard_iface::UNIT_RX, ANTSW_IO, ANTSW_IO);
    this->get_iface()->set_gpio_ddr(dboard_iface::UNIT_TX, ANTSW_IO, ANTSW_IO);
    this->get_iface()->set_gpio_ddr(dboard_iface::UNIT_RX, ANTSW_IO, ANTSW_IO);

    //setup ATR for the antenna switches (constant)
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, dboard_iface::ATR_REG_IDLE,        ANT_RX, ANTSW_IO);
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, dboard_iface::ATR_REG_RX_ONLY,     ANT_RX, ANTSW_IO);
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, dboard_iface::ATR_REG_TX_ONLY,     ANT_TX, ANTSW_IO);
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, dboard_iface::ATR_REG_FULL_DUPLEX, ANT_TX, ANTSW_IO);

    this->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, dboard_iface::ATR_REG_IDLE,        ANT_TXRX, ANTSW_IO);
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, dboard_iface::ATR_REG_TX_ONLY,     ANT_RX2, ANTSW_IO);
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, dboard_iface::ATR_REG_FULL_DUPLEX, ANT_RX2, ANTSW_IO);

    //set some default values
    set_rx_lo_freq((wbx_freq_range.start() + wbx_freq_range.stop())/2.0);
    set_tx_lo_freq((wbx_freq_range.start() + wbx_freq_range.stop())/2.0);
    set_rx_ant("RX2");
}

wbx_simple::~wbx_simple(void){
    /* NOP */
}

/***********************************************************************
 * Antennas
 **********************************************************************/
void wbx_simple::set_rx_ant(const std::string &ant){
    //validate input
    assert_has(wbx_rx_antennas, ant, "wbx rx antenna name");

    //shadow the setting
    _rx_ant = ant;

    //write the new antenna setting to atr regs
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, dboard_iface::ATR_REG_RX_ONLY, ((_rx_ant == "TX/RX")? ANT_TXRX : ANT_RX2), ANTSW_IO);
}

void wbx_simple::set_tx_ant(const std::string &ant){
    assert_has(wbx_tx_antennas, ant, "wbx tx antenna name");
    //only one antenna option, do nothing
}

/***********************************************************************
 * Tuning
 **********************************************************************/
void wbx_simple::set_rx_lo_freq(double freq){
    _rx_lo_freq = set_lo_freq(dboard_iface::UNIT_RX, wbx_freq_range.clip(freq));
}

void wbx_simple::set_tx_lo_freq(double freq){
    _tx_lo_freq = set_lo_freq(dboard_iface::UNIT_TX, wbx_freq_range.clip(freq));
}

/***********************************************************************
 * RX Get and Set
 **********************************************************************/
void wbx_simple::rx_get(const wax::obj &key_, wax::obj &val){
    named_prop_t key = named_prop_t::extract(key_);

    //handle the get request conditioned on the key
    switch(key.as<subdev_prop_t>()){
    case SUBDEV_PROP_NAME:
        val = std::string("WBX RX + Simple GDB");
        return;

    case SUBDEV_PROP_FREQ:
        val = _rx_lo_freq;
        return;

    case SUBDEV_PROP_FREQ_RANGE:
        val = wbx_freq_range;
        return;

    case SUBDEV_PROP_ANTENNA:
        val = _rx_ant;
        return;

    case SUBDEV_PROP_ANTENNA_NAMES:
        val = wbx_rx_antennas;
        return;

    default:
        //call into the base class for other properties
        wbx_base::rx_get(key_, val);
    }
}

void wbx_simple::rx_set(const wax::obj &key_, const wax::obj &val){
    named_prop_t key = named_prop_t::extract(key_);

    //handle the get request conditioned on the key
    switch(key.as<subdev_prop_t>()){

    case SUBDEV_PROP_FREQ:
        this->set_rx_lo_freq(val.as<double>());
        return;

    case SUBDEV_PROP_ANTENNA:
        this->set_rx_ant(val.as<std::string>());
        return;

    default:
        //call into the base class for other properties
        wbx_base::rx_set(key_, val);
    }
}

/***********************************************************************
 * TX Get and Set
 **********************************************************************/
void wbx_simple::tx_get(const wax::obj &key_, wax::obj &val){
    named_prop_t key = named_prop_t::extract(key_);

    //handle the get request conditioned on the key
    switch(key.as<subdev_prop_t>()){
    case SUBDEV_PROP_NAME:
        val = std::string("WBX TX + Simple GDB");
        return;

    case SUBDEV_PROP_FREQ:
        val = _tx_lo_freq;
        return;

    case SUBDEV_PROP_FREQ_RANGE:
        val = wbx_freq_range;
        return;

    case SUBDEV_PROP_ANTENNA:
        val = std::string("TX/RX");
        return;

    case SUBDEV_PROP_ANTENNA_NAMES:
        val = wbx_tx_antennas;
        return;

    default:
        //call into the base class for other properties
        wbx_base::tx_get(key_, val);
    }
}

void wbx_simple::tx_set(const wax::obj &key_, const wax::obj &val){
    named_prop_t key = named_prop_t::extract(key_);

    //handle the get request conditioned on the key
    switch(key.as<subdev_prop_t>()){

    case SUBDEV_PROP_FREQ:
        this->set_tx_lo_freq(val.as<double>());
        return;

    case SUBDEV_PROP_ANTENNA:
        this->set_tx_ant(val.as<std::string>());
        return;

    default:
        //call into the base class for other properties
        wbx_base::tx_set(key_, val);
    }
}
