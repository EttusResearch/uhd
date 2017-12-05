//
// Copyright 2010-2011,2017 Ettus Research, A National Instruments Company
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

#include "dboard_ctor_args.hpp"
#include <uhd/usrp/dboard_base.hpp>
#include <boost/format.hpp>
#include <stdexcept>

using namespace uhd;
using namespace uhd::usrp;

/***********************************************************************
 * dboard_base dboard dboard_base class
 **********************************************************************/
struct dboard_base::impl{
    dboard_ctor_args_t args;
};

dboard_base::dboard_base(ctor_args_t args){
    _impl = UHD_PIMPL_MAKE(impl, ());
    _impl->args = dboard_ctor_args_t::cast(args);
}

std::string dboard_base::get_subdev_name(void){
    return _impl->args.sd_name;
}

dboard_iface::sptr dboard_base::get_iface(void){
    return _impl->args.db_iface;
}

dboard_id_t dboard_base::get_rx_id(void){
    return _impl->args.rx_eeprom.id;
}

dboard_id_t dboard_base::get_tx_id(void){
    return _impl->args.tx_eeprom.id;
}

dboard_eeprom_t dboard_base::get_rx_eeprom(void){
    return _impl->args.rx_eeprom;
}

dboard_eeprom_t dboard_base::get_tx_eeprom(void){
    return _impl->args.tx_eeprom;
}

property_tree::sptr dboard_base::get_rx_subtree(void){
    return _impl->args.rx_subtree;
}

property_tree::sptr dboard_base::get_tx_subtree(void){
    return _impl->args.tx_subtree;
}

/***********************************************************************
 * xcvr dboard dboard_base class
 **********************************************************************/
xcvr_dboard_base::xcvr_dboard_base(ctor_args_t args) : dboard_base(args){
    if (get_rx_id() == dboard_id_t::none()){
        throw uhd::runtime_error(str(boost::format(
            "cannot create xcvr board when the rx id is \"%s\""
        ) % dboard_id_t::none().to_pp_string()));
    }
    if (get_tx_id() == dboard_id_t::none()){
        throw uhd::runtime_error(str(boost::format(
            "cannot create xcvr board when the tx id is \"%s\""
        ) % dboard_id_t::none().to_pp_string()));
    }
}

/***********************************************************************
 * rx dboard dboard_base class
 **********************************************************************/
rx_dboard_base::rx_dboard_base(ctor_args_t args) : dboard_base(args){
    if (get_tx_id() != dboard_id_t::none()){
        throw uhd::runtime_error(str(boost::format(
            "cannot create rx board when the tx id is \"%s\""
            " -> expected a tx id of \"%s\""
        ) % get_tx_id().to_pp_string() % dboard_id_t::none().to_pp_string()));
    }
}

/***********************************************************************
 * tx dboard dboard_base class
 **********************************************************************/
tx_dboard_base::tx_dboard_base(ctor_args_t args) : dboard_base(args){
    if (get_rx_id() != dboard_id_t::none()){
        throw uhd::runtime_error(str(boost::format(
            "cannot create tx board when the rx id is \"%s\""
            " -> expected a rx id of \"%s\""
        ) % get_rx_id().to_pp_string() % dboard_id_t::none().to_pp_string()));
    }
}
