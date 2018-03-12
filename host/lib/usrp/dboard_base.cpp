//
// Copyright 2010-2011,2017 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
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
