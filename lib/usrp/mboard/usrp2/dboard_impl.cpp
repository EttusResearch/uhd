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

#include <boost/format.hpp>
#include <uhd/utils.hpp>
#include <uhd/props.hpp>
#include <iostream>
#include "dboard_impl.hpp"
#include "dboard_interface.hpp"

using namespace uhd;
using namespace uhd::usrp;

dboard_impl::dboard_impl(uhd::usrp::dboard::manager::sptr mgr, type_t type){
    _mgr = mgr;
    _type = type;
}

dboard_impl::~dboard_impl(void){
    /* NOP */
}

void dboard_impl::get(const wax::obj &key_, wax::obj &val){
    wax::obj key; std::string name;
    boost::tie(key, name) = extract_named_prop(key_);

    //handle the get request conditioned on the key
    switch(wax::cast<dboard_prop_t>(key)){
    case DBOARD_PROP_NAME:
        val = std::string("usrp2 dboard");
        return;

    case DBOARD_PROP_SUBDEV:
        switch(_type){
        case TYPE_RX:
            val = _mgr->get_rx_subdev(name);
            return;

        case TYPE_TX:
            val = _mgr->get_tx_subdev(name);
            return;
        }

    case DBOARD_PROP_SUBDEV_NAMES:
        switch(_type){
        case TYPE_RX:
            val = _mgr->get_rx_subdev_names();
            return;

        case TYPE_TX:
            val = _mgr->get_tx_subdev_names();
            return;
        }

    case DBOARD_PROP_CODEC:
        throw std::runtime_error("unhandled prop in usrp2 dboard");
    }
}

void dboard_impl::set(const wax::obj &, const wax::obj &){
    throw std::runtime_error("Cannot set on usrp2 dboard");
}
