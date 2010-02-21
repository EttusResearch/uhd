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

#include "dboards.hpp"

/***********************************************************************
 * Basic RX dboard
 **********************************************************************/
basic_rx::basic_rx(ctor_args_t const& args) : rx_dboard_base(args){
    /* NOP */
}

basic_rx::~basic_rx(void){
    /* NOP */
}

void basic_rx::rx_get(const wax::obj &, wax::obj &){
    /* TODO */
}

void basic_rx::rx_set(const wax::obj &, const wax::obj &){
    /* TODO */
}

/***********************************************************************
 * Basic TX dboard
 **********************************************************************/
basic_tx::basic_tx(ctor_args_t const& args) : tx_dboard_base(args){
    /* NOP */
}

basic_tx::~basic_tx(void){
    /* NOP */
}

void basic_tx::tx_get(const wax::obj &, wax::obj &){
    /* TODO */
}

void basic_tx::tx_set(const wax::obj &, const wax::obj &){
    /* TODO */
}
