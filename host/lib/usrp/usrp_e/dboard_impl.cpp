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

#include <boost/bind.hpp>
#include "usrp_e_impl.hpp"
#include <iostream>

using namespace uhd::usrp;

/***********************************************************************
 * Dboard Initialization
 **********************************************************************/
void usrp_e_impl::dboard_init(void){
    _rx_db_eeprom = dboard_eeprom_t(_iface->read_eeprom(I2C_ADDR_RX_DB, 0, dboard_eeprom_t::num_bytes()));
    _tx_db_eeprom = dboard_eeprom_t(_iface->read_eeprom(I2C_ADDR_TX_DB, 0, dboard_eeprom_t::num_bytes()));

    std::cout << _rx_db_eeprom.id.to_pp_string() << std::endl;
    std::cout << _tx_db_eeprom.id.to_pp_string() << std::endl;

    //create a new dboard interface and manager
    dboard_iface::sptr dboard_iface(
        make_usrp_e_dboard_iface(_iface, _clock_ctrl, _codec_ctrl)
    );
    _dboard_manager = dboard_manager::make(
        _rx_db_eeprom.id, _tx_db_eeprom.id, dboard_iface
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
void usrp_e_impl::rx_dboard_get(const wax::obj &, wax::obj &){
    UHD_THROW_PROP_GET_ERROR();
}

/***********************************************************************
 * RX Dboard Set
 **********************************************************************/
void usrp_e_impl::rx_dboard_set(const wax::obj &, const wax::obj &){
    UHD_THROW_PROP_SET_ERROR();
}

/***********************************************************************
 * TX Dboard Get
 **********************************************************************/
void usrp_e_impl::tx_dboard_get(const wax::obj &, wax::obj &){
    UHD_THROW_PROP_GET_ERROR();
}

/***********************************************************************
 * TX Dboard Set
 **********************************************************************/
void usrp_e_impl::tx_dboard_set(const wax::obj &, const wax::obj &){
    UHD_THROW_PROP_SET_ERROR();
}
