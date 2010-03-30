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

using namespace uhd::usrp;

/***********************************************************************
 * Mboard Initialization
 **********************************************************************/
void usrp_e_impl::mboard_init(void){
    _mboard_proxy = wax_obj_proxy::make(
        boost::bind(&usrp_e_impl::mboard_get, this, _1, _2),
        boost::bind(&usrp_e_impl::mboard_set, this, _1, _2)
    );
}

/***********************************************************************
 * Mboard Get
 **********************************************************************/
void usrp_e_impl::mboard_get(const wax::obj &, wax::obj &){
    
}

/***********************************************************************
 * Mboard Set
 **********************************************************************/
void usrp_e_impl::mboard_set(const wax::obj &, const wax::obj &){
    
}
