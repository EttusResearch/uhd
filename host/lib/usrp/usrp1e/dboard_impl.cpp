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
#include <uhd/utils.hpp>
#include "usrp1e_impl.hpp"

using namespace uhd::usrp;

/***********************************************************************
 * Dboard Initialization
 **********************************************************************/
void usrp1e_impl::dboard_init(void){
    dboard_id_t rx_dboard_id = dboard_id::NONE; //TODO get these from the eeprom
    dboard_id_t tx_dboard_id = dboard_id::NONE;

    //create a new dboard interface and manager
    dboard_interface::sptr dboard_interface(
        make_usrp1e_dboard_interface(this)
    );
    _dboard_manager = dboard_manager::make(
        rx_dboard_id, tx_dboard_id, dboard_interface
    );

    //setup the dboard proxies
    _rx_dboard_proxy = wax_obj_proxy(
        boost::bind(&usrp1e_impl::rx_dboard_get, this, _1, _2),
        boost::bind(&usrp1e_impl::rx_dboard_set, this, _1, _2)
    );
    _tx_dboard_proxy = wax_obj_proxy(
        boost::bind(&usrp1e_impl::tx_dboard_get, this, _1, _2),
        boost::bind(&usrp1e_impl::tx_dboard_set, this, _1, _2)
    );
}

/***********************************************************************
 * RX Dboard Get
 **********************************************************************/
void usrp1e_impl::rx_dboard_get(const wax::obj &, wax::obj &){
    
}

/***********************************************************************
 * RX Dboard Set
 **********************************************************************/
void usrp1e_impl::rx_dboard_set(const wax::obj &, const wax::obj &){
    
}

/***********************************************************************
 * TX Dboard Get
 **********************************************************************/
void usrp1e_impl::tx_dboard_get(const wax::obj &, wax::obj &){
    
}

/***********************************************************************
 * TX Dboard Set
 **********************************************************************/
void usrp1e_impl::tx_dboard_set(const wax::obj &, const wax::obj &){
    
}
