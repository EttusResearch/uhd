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

#include "usrp1_impl.hpp"
#include "usrp_i2c_addr.h"
#include <uhd/usrp/dsp_utils.hpp>
#include <uhd/usrp/misc_utils.hpp>
#include <uhd/utils/assert.hpp>
#include <uhd/usrp/dboard_props.hpp>
#include <uhd/usrp/subdev_props.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <iostream>

using namespace uhd;
using namespace uhd::usrp;

/***********************************************************************
 * Helper Functions
 **********************************************************************/
static boost::uint8_t get_rx_ee_addr(usrp1_impl::dboard_slot_t dboard_slot){
    switch(dboard_slot){
    case usrp1_impl::DBOARD_SLOT_A: return I2C_ADDR_RX_A;
    case usrp1_impl::DBOARD_SLOT_B: return I2C_ADDR_RX_B;
    default: UHD_THROW_INVALID_CODE_PATH();
    }
}

static boost::uint8_t get_tx_ee_addr(usrp1_impl::dboard_slot_t dboard_slot){
    switch(dboard_slot){
    case usrp1_impl::DBOARD_SLOT_A: return I2C_ADDR_TX_A;
    case usrp1_impl::DBOARD_SLOT_B: return I2C_ADDR_TX_B;
    default: UHD_THROW_INVALID_CODE_PATH();
    }
}

/***********************************************************************
 * Dboard Initialization
 **********************************************************************/
void usrp1_impl::dboard_init(void)
{
    BOOST_FOREACH(dboard_slot_t dboard_slot, _dboard_slots){

        //read the tx and rx dboard eeproms
        _rx_db_eeproms[dboard_slot].load(*_iface, get_rx_ee_addr(dboard_slot));
        _tx_db_eeproms[dboard_slot].load(*_iface, get_tx_ee_addr(dboard_slot));

        //create a new dboard interface and manager
        _dboard_ifaces[dboard_slot] = make_dboard_iface(
            _iface, _clock_ctrl, _codec_ctrls[dboard_slot],
            dboard_slot, _rx_db_eeproms[dboard_slot].id
        );

        _dboard_managers[dboard_slot] = dboard_manager::make(
            _rx_db_eeproms[dboard_slot].id,
            _tx_db_eeproms[dboard_slot].id,
            _dboard_ifaces[dboard_slot]
        );

        //setup the dboard proxies
        _rx_dboard_proxies[dboard_slot] = wax_obj_proxy::make(
             boost::bind(&usrp1_impl::rx_dboard_get, this, _1, _2, dboard_slot),
             boost::bind(&usrp1_impl::rx_dboard_set, this, _1, _2, dboard_slot));

        _tx_dboard_proxies[dboard_slot] = wax_obj_proxy::make(
             boost::bind(&usrp1_impl::tx_dboard_get, this, _1, _2, dboard_slot),
             boost::bind(&usrp1_impl::tx_dboard_set, this, _1, _2, dboard_slot));
    }

}

/***********************************************************************
 * RX Dboard Get
 **********************************************************************/
void usrp1_impl::rx_dboard_get(const wax::obj &key_, wax::obj &val, dboard_slot_t dboard_slot)
{
    named_prop_t key = named_prop_t::extract(key_);

    //handle the get request conditioned on the key
    switch(key.as<dboard_prop_t>()){
    case DBOARD_PROP_NAME:
        val = str(boost::format("usrp1 dboard (rx unit) - %c") % char(dboard_slot));
        return;

    case DBOARD_PROP_SUBDEV:
        val = _dboard_managers[dboard_slot]->get_rx_subdev(key.name);
        return;

    case DBOARD_PROP_SUBDEV_NAMES:
        val = _dboard_managers[dboard_slot]->get_rx_subdev_names();
        return;

    case DBOARD_PROP_DBOARD_EEPROM:
        val = _rx_db_eeproms[dboard_slot];
        return;

    case DBOARD_PROP_DBOARD_IFACE:
        val = _dboard_ifaces[dboard_slot];
        return;

    case DBOARD_PROP_CODEC:
        val = _rx_codec_proxies[dboard_slot]->get_link();
        return;

    case DBOARD_PROP_GAIN_GROUP:
        val = make_gain_group(
            _rx_db_eeproms[dboard_slot].id,
            _dboard_managers[dboard_slot]->get_rx_subdev(key.name),
            _rx_codec_proxies[dboard_slot]->get_link(),
            GAIN_GROUP_POLICY_RX
        );
        return;

    default: UHD_THROW_PROP_GET_ERROR();
    }
}

/***********************************************************************
 * RX Dboard Set
 **********************************************************************/
void usrp1_impl::rx_dboard_set(const wax::obj &key, const wax::obj &val, dboard_slot_t dboard_slot)
{
    switch(key.as<dboard_prop_t>()) {
    case DBOARD_PROP_DBOARD_EEPROM:
        _rx_db_eeproms[dboard_slot] = val.as<dboard_eeprom_t>();
        _rx_db_eeproms[dboard_slot].store(*_iface, get_rx_ee_addr(dboard_slot));
        return;

    default:
        UHD_THROW_PROP_SET_ERROR();
    }
}

/***********************************************************************
 * TX Dboard Get
 **********************************************************************/
void usrp1_impl::tx_dboard_get(const wax::obj &key_, wax::obj &val, dboard_slot_t dboard_slot)
{
    named_prop_t key = named_prop_t::extract(key_);

    //handle the get request conditioned on the key
    switch(key.as<dboard_prop_t>()){
    case DBOARD_PROP_NAME:
        val = str(boost::format("usrp1 dboard (tx unit) - %c") % char(dboard_slot));
        return;

    case DBOARD_PROP_SUBDEV:
        val = _dboard_managers[dboard_slot]->get_tx_subdev(key.name);
        return;

    case DBOARD_PROP_SUBDEV_NAMES:
        val = _dboard_managers[dboard_slot]->get_tx_subdev_names();
        return;

    case DBOARD_PROP_DBOARD_EEPROM:
        val = _tx_db_eeproms[dboard_slot];
        return;

    case DBOARD_PROP_DBOARD_IFACE:
        val = _dboard_ifaces[dboard_slot];
        return;

    case DBOARD_PROP_CODEC:
        val = _tx_codec_proxies[dboard_slot]->get_link();
        return;

    case DBOARD_PROP_GAIN_GROUP:
        val = make_gain_group(
            _tx_db_eeproms[dboard_slot].id,
            _dboard_managers[dboard_slot]->get_tx_subdev(key.name),
            _tx_codec_proxies[dboard_slot]->get_link(),
            GAIN_GROUP_POLICY_TX
        );
        return;

    default: UHD_THROW_PROP_GET_ERROR();
    }
}

/***********************************************************************
 * TX Dboard Set
 **********************************************************************/
void usrp1_impl::tx_dboard_set(const wax::obj &key, const wax::obj &val, dboard_slot_t dboard_slot)
{
    switch(key.as<dboard_prop_t>()) {
    case DBOARD_PROP_DBOARD_EEPROM:
        _tx_db_eeproms[dboard_slot] = val.as<dboard_eeprom_t>();
        _tx_db_eeproms[dboard_slot].store(*_iface, get_tx_ee_addr(dboard_slot));
        return;

    default: UHD_THROW_PROP_SET_ERROR();
    }
}
