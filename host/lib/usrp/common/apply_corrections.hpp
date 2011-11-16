//
// Copyright 2011 Ettus Research LLC
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

#ifndef INCLUDED_LIBUHD_USRP_COMMON_APPLY_CORRECTIONS_HPP
#define INCLUDED_LIBUHD_USRP_COMMON_APPLY_CORRECTIONS_HPP

#include <uhd/config.hpp>
#include <uhd/property_tree.hpp>
#include <string>

namespace uhd{ namespace usrp{

    void apply_tx_fe_corrections(
        property_tree::sptr sub_tree, //starts at mboards/x
        const std::string &slot, //name of dboard slot
        const double tx_lo_freq //actual lo freq
    );

    void apply_rx_fe_corrections(
        property_tree::sptr sub_tree, //starts at mboards/x
        const std::string &slot, //name of dboard slot
        const double rx_lo_freq //actual lo freq
    );

}} //namespace uhd::usrp

#endif /* INCLUDED_LIBUHD_USRP_COMMON_APPLY_CORRECTIONS_HPP */
