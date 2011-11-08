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

#ifndef INCLUDED_LIBUHD_USRP_DBOARD_CTOR_ARGS_HPP
#define INCLUDED_LIBUHD_USRP_DBOARD_CTOR_ARGS_HPP

#include <uhd/property_tree.hpp>
#include <uhd/usrp/dboard_id.hpp>
#include <uhd/usrp/dboard_base.hpp>
#include <uhd/usrp/dboard_iface.hpp>
#include <string>

namespace uhd{ namespace usrp{

    struct dboard_ctor_args_t{
        std::string               sd_name;
        dboard_iface::sptr        db_iface;
        dboard_id_t               rx_id, tx_id;
        property_tree::sptr       rx_subtree, tx_subtree;
    };

}} //namespace

#endif /* INCLUDED_LIBUHD_USRP_DBOARD_CTOR_ARGS_HPP */
