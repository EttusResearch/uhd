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

#include <uhd/usrp/dboard/base.hpp>
#include <boost/format.hpp>
#include <stdexcept>

using namespace uhd::usrp::dboard;

/***********************************************************************
 * base dboard base class
 **********************************************************************/
base::base(ctor_args_t const& args){
    boost::tie(_subdev_name, _dboard_interface, _rx_id, _tx_id) = args;
}

base::~base(void){
    /* NOP */
}

std::string base::get_subdev_name(void){
    return _subdev_name;
}

interface::sptr base::get_interface(void){
    return _dboard_interface;
}

dboard_id_t base::get_rx_id(void){
    return _rx_id;
}

dboard_id_t base::get_tx_id(void){
    return _tx_id;
}

/***********************************************************************
 * xcvr dboard base class
 **********************************************************************/
xcvr_base::xcvr_base(ctor_args_t const& args) : base(args){
    if (get_rx_id() == ID_NONE){
        throw std::runtime_error(str(boost::format(
            "cannot create xcvr board when the rx id is \"%s\""
        ) % id::to_string(ID_NONE)));
    }
    if (get_tx_id() == ID_NONE){
        throw std::runtime_error(str(boost::format(
            "cannot create xcvr board when the tx id is \"%s\""
        ) % id::to_string(ID_NONE)));
    }
}

xcvr_base::~xcvr_base(void){
    /* NOP */
}

/***********************************************************************
 * rx dboard base class
 **********************************************************************/
rx_base::rx_base(ctor_args_t const& args) : base(args){
    if (get_tx_id() != ID_NONE){
        throw std::runtime_error(str(boost::format(
            "cannot create rx board when the tx id is \"%s\""
            " -> expected a tx id of \"%s\""
        ) % id::to_string(get_tx_id()) % id::to_string(ID_NONE)));
    }
}

rx_base::~rx_base(void){
    /* NOP */
}

void rx_base::tx_get(const wax::obj &, wax::obj &){
    throw std::runtime_error("cannot call tx_get on a rx dboard");
}

void rx_base::tx_set(const wax::obj &, const wax::obj &){
    throw std::runtime_error("cannot call tx_set on a rx dboard");
}

/***********************************************************************
 * tx dboard base class
 **********************************************************************/
tx_base::tx_base(ctor_args_t const& args) : base(args){
    if (get_rx_id() != ID_NONE){
        throw std::runtime_error(str(boost::format(
            "cannot create tx board when the rx id is \"%s\""
            " -> expected a rx id of \"%s\""
        ) % id::to_string(get_rx_id()) % id::to_string(ID_NONE)));
    }
}

tx_base::~tx_base(void){
    /* NOP */
}

void tx_base::rx_get(const wax::obj &, wax::obj &){
    throw std::runtime_error("cannot call rx_get on a tx dboard");
}

void tx_base::rx_set(const wax::obj &, const wax::obj &){
    throw std::runtime_error("cannot call rx_set on a tx dboard");
}
