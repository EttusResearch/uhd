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
 * RX DDC Initialization
 **********************************************************************/
void usrp1e_impl::rx_ddc_init(void){
    _rx_ddc_proxy = wax_obj_proxy::make(
        boost::bind(&usrp1e_impl::rx_ddc_get, this, _1, _2),
        boost::bind(&usrp1e_impl::rx_ddc_set, this, _1, _2)
    );
}

/***********************************************************************
 * RX DDC Get
 **********************************************************************/
void usrp1e_impl::rx_ddc_get(const wax::obj &, wax::obj &){
    
}

/***********************************************************************
 * RX DDC Set
 **********************************************************************/
void usrp1e_impl::rx_ddc_set(const wax::obj &, const wax::obj &){
    
}

/***********************************************************************
 * TX DUC Initialization
 **********************************************************************/
void usrp1e_impl::tx_duc_init(void){
    _tx_duc_proxy = wax_obj_proxy::make(
        boost::bind(&usrp1e_impl::tx_duc_get, this, _1, _2),
        boost::bind(&usrp1e_impl::tx_duc_set, this, _1, _2)
    );
}

/***********************************************************************
 * TX DUC Get
 **********************************************************************/
void usrp1e_impl::tx_duc_get(const wax::obj &, wax::obj &){
    
}

/***********************************************************************
 * TX DUC Set
 **********************************************************************/
void usrp1e_impl::tx_duc_set(const wax::obj &, const wax::obj &){
    
}
