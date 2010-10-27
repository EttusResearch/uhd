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

#include "usrp_e_impl.hpp"
#include "usrp_e_regs.hpp"
#include <uhd/utils/assert.hpp>
#include <uhd/usrp/dboard_props.hpp>
#include <uhd/usrp/subdev_props.hpp>
#include <uhd/usrp/misc_utils.hpp>
#include <boost/bind.hpp>
#include <iostream>

using namespace uhd;
using namespace uhd::usrp;

/***********************************************************************
 * Dboard Initialization
 **********************************************************************/
void usrp_e_impl::dboard_init(void){
    _rx_db_eeprom = dboard_eeprom_t(_iface->read_eeprom(I2C_ADDR_RX_DB, 0, dboard_eeprom_t::num_bytes()));
    _tx_db_eeprom = dboard_eeprom_t(_iface->read_eeprom(I2C_ADDR_TX_DB, 0, dboard_eeprom_t::num_bytes()));

    //create a new dboard interface and manager
    _dboard_iface = make_usrp_e_dboard_iface(
        _iface, _clock_ctrl, _codec_ctrl
    );
    _dboard_manager = dboard_manager::make(
        _rx_db_eeprom.id, _tx_db_eeprom.id, _dboard_iface
    );

    //setup the dboard proxies
    _rx_dboard_proxy = wax_obj_proxy::make(
        boost::bind(&usrp_e_impl::rx_dboard_get, this, _1, _2),
        boost::bind(&usrp_e_impl::rx_dboard_set, this, _1, _2)
    );
    _tx_dboard_proxy = wax_obj_proxy::make(
        boost::bind(&usrp_e_impl::tx_dboard_get, this, _1, _2),
        boost::bind(&usrp_e_impl::tx_dboard_set, this, _1, _2)
    );
}

/***********************************************************************
 * RX Dboard Get
 **********************************************************************/
void usrp_e_impl::rx_dboard_get(const wax::obj &key_, wax::obj &val){
    named_prop_t key = named_prop_t::extract(key_);

    //handle the get request conditioned on the key
    switch(key.as<dboard_prop_t>()){
    case DBOARD_PROP_NAME:
        val = std::string("usrp-e dboard (rx unit)");
        return;

    case DBOARD_PROP_SUBDEV:
        val = _dboard_manager->get_rx_subdev(key.name);
        return;

    case DBOARD_PROP_SUBDEV_NAMES:
        val = _dboard_manager->get_rx_subdev_names();
        return;

    case DBOARD_PROP_DBOARD_ID:
        val = _rx_db_eeprom.id;
        return;

    case DBOARD_PROP_DBOARD_IFACE:
        val = _dboard_iface;
        return;

    case DBOARD_PROP_CODEC:
        val = _rx_codec_proxy->get_link();
        return;

    case DBOARD_PROP_GAIN_GROUP:
        val = make_gain_group(
            _rx_db_eeprom.id,
            _dboard_manager->get_rx_subdev(key.name),
            _rx_codec_proxy->get_link(),
            GAIN_GROUP_POLICY_RX
        );
        return;

    default: UHD_THROW_PROP_GET_ERROR();
    }
}

/***********************************************************************
 * RX Dboard Set
 **********************************************************************/
void usrp_e_impl::rx_dboard_set(const wax::obj &key, const wax::obj &val){
    switch(key.as<dboard_prop_t>()){
    case DBOARD_PROP_DBOARD_ID:
        _rx_db_eeprom.id = val.as<dboard_id_t>();
        _iface->write_eeprom(I2C_ADDR_RX_DB, 0, _rx_db_eeprom.get_eeprom_bytes());
        return;

    default: UHD_THROW_PROP_SET_ERROR();
    }
}

/***********************************************************************
 * TX Dboard Get
 **********************************************************************/
void usrp_e_impl::tx_dboard_get(const wax::obj &key_, wax::obj &val){
    named_prop_t key = named_prop_t::extract(key_);

    //handle the get request conditioned on the key
    switch(key.as<dboard_prop_t>()){
    case DBOARD_PROP_NAME:
        val = std::string("usrp-e dboard (tx unit)");
        return;

    case DBOARD_PROP_SUBDEV:
        val = _dboard_manager->get_tx_subdev(key.name);
        return;

    case DBOARD_PROP_SUBDEV_NAMES:
        val = _dboard_manager->get_tx_subdev_names();
        return;

    case DBOARD_PROP_DBOARD_ID:
        val = _tx_db_eeprom.id;
        return;

    case DBOARD_PROP_DBOARD_IFACE:
        val = _dboard_iface;
        return;

    case DBOARD_PROP_CODEC:
        val = _tx_codec_proxy->get_link();
        return;

    case DBOARD_PROP_GAIN_GROUP:
        val = make_gain_group(
            _tx_db_eeprom.id,
            _dboard_manager->get_tx_subdev(key.name),
            _tx_codec_proxy->get_link(),
            GAIN_GROUP_POLICY_TX
        );
        return;

    default: UHD_THROW_PROP_GET_ERROR();
    }
}

/***********************************************************************
 * TX Dboard Set
 **********************************************************************/
void usrp_e_impl::tx_dboard_set(const wax::obj &key, const wax::obj &val){
    switch(key.as<dboard_prop_t>()){
    case DBOARD_PROP_DBOARD_ID:
        _tx_db_eeprom.id = val.as<dboard_id_t>();
        _iface->write_eeprom(I2C_ADDR_TX_DB, 0, _tx_db_eeprom.get_eeprom_bytes());
        return;

    default: UHD_THROW_PROP_SET_ERROR();
    }
}
