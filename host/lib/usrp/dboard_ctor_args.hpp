//
// Copyright 2010,2017 Ettus Research, A National Instruments Company
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
