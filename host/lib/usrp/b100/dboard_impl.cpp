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

#include "b100_impl.hpp"
#include "b100_regs.hpp"
#include <uhd/exception.hpp>
#include <uhd/usrp/dboard_props.hpp>
#include <uhd/usrp/subdev_props.hpp>
#include <uhd/usrp/misc_utils.hpp>
#include <boost/bind.hpp>
#include "usrp_i2c_addr.h"

using namespace uhd;
using namespace uhd::usrp;

/***********************************************************************
 * Dboard Initialization
 **********************************************************************/
void b100_impl::dboard_init(void){
    //read the tx and rx dboard eeproms
    _rx_db_eeprom.load(*_iface, I2C_ADDR_RX_A);
    _tx_db_eeprom.load(*_iface, I2C_ADDR_TX_A);
    _gdb_eeprom.load(*_iface, I2C_ADDR_TX_A ^ 5);

    //create a new dboard interface and manager
    _dboard_iface = make_b100_dboard_iface(
        _iface, _clock_ctrl, _codec_ctrl
    );
    _dboard_manager = dboard_manager::make(
        _rx_db_eeprom.id,
        ((_gdb_eeprom.id == dboard_id_t::none())? _tx_db_eeprom : _gdb_eeprom).id,
        _dboard_iface
    );

    //setup the dboard proxies
    _rx_dboard_proxy = wax_obj_proxy::make(
        boost::bind(&b100_impl::rx_dboard_get, this, _1, _2),
        boost::bind(&b100_impl::rx_dboard_set, this, _1, _2)
    );
    _tx_dboard_proxy = wax_obj_proxy::make(
        boost::bind(&b100_impl::tx_dboard_get, this, _1, _2),
        boost::bind(&b100_impl::tx_dboard_set, this, _1, _2)
    );
}

/***********************************************************************
 * RX Dboard Get
 **********************************************************************/
void b100_impl::rx_dboard_get(const wax::obj &key_, wax::obj &val){
    named_prop_t key = named_prop_t::extract(key_);

    //handle the get request conditioned on the key
    switch(key.as<dboard_prop_t>()){
    case DBOARD_PROP_NAME:
        val = std::string("b100 dboard (rx unit)");
        return;

    case DBOARD_PROP_SUBDEV:
        val = _dboard_manager->get_rx_subdev(key.name);
        return;

    case DBOARD_PROP_SUBDEV_NAMES:
        val = _dboard_manager->get_rx_subdev_names();
        return;

    case DBOARD_PROP_DBOARD_EEPROM:
        val = _rx_db_eeprom;
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
void b100_impl::rx_dboard_set(const wax::obj &key, const wax::obj &val){
    switch(key.as<dboard_prop_t>()){
    case DBOARD_PROP_DBOARD_EEPROM:
        _rx_db_eeprom = val.as<dboard_eeprom_t>();
        _rx_db_eeprom.store(*_iface, I2C_ADDR_RX_A);
        return;

    default: UHD_THROW_PROP_SET_ERROR();
    }
}

/***********************************************************************
 * TX Dboard Get
 **********************************************************************/
void b100_impl::tx_dboard_get(const wax::obj &key_, wax::obj &val){
    named_prop_t key = named_prop_t::extract(key_);

    //handle the get request conditioned on the key
    switch(key.as<dboard_prop_t>()){
    case DBOARD_PROP_NAME:
        val = std::string("b100 dboard (tx unit)");
        return;

    case DBOARD_PROP_SUBDEV:
        val = _dboard_manager->get_tx_subdev(key.name);
        return;

    case DBOARD_PROP_SUBDEV_NAMES:
        val = _dboard_manager->get_tx_subdev_names();
        return;

    case DBOARD_PROP_DBOARD_EEPROM:
        val = _tx_db_eeprom;
        return;

    case DBOARD_PROP_GBOARD_EEPROM:
        val = _gdb_eeprom;
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
void b100_impl::tx_dboard_set(const wax::obj &key, const wax::obj &val){
    switch(key.as<dboard_prop_t>()){
    case DBOARD_PROP_DBOARD_EEPROM:
        _tx_db_eeprom = val.as<dboard_eeprom_t>();
        _tx_db_eeprom.store(*_iface, I2C_ADDR_TX_A);
        return;

    case DBOARD_PROP_GBOARD_EEPROM:
        _gdb_eeprom = val.as<dboard_eeprom_t>();
        _gdb_eeprom.store(*_iface, I2C_ADDR_TX_A ^ 5);
        return;

    default: UHD_THROW_PROP_SET_ERROR();
    }
}
