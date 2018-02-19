//
// Copyright 2010,2017 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_USRP_DBOARD_CTOR_ARGS_HPP
#define INCLUDED_LIBUHD_USRP_DBOARD_CTOR_ARGS_HPP

#include <uhd/property_tree.hpp>
#include <uhd/usrp/dboard_base.hpp>
#include <uhd/usrp/dboard_iface.hpp>
#include <uhd/usrp/dboard_eeprom.hpp>
#include <string>

namespace uhd{ namespace usrp{

    class dboard_ctor_args_t {
    public:
        std::string               sd_name;
        dboard_iface::sptr        db_iface;
        dboard_eeprom_t           rx_eeprom, tx_eeprom;
        property_tree::sptr       rx_subtree, tx_subtree;
        dboard_base::sptr         rx_container, tx_container;

        static const dboard_ctor_args_t& cast(dboard_base::ctor_args_t args) {
            return *static_cast<dboard_ctor_args_t*>(args);
        }
    };

}} //namespace

#endif /* INCLUDED_LIBUHD_USRP_DBOARD_CTOR_ARGS_HPP */
