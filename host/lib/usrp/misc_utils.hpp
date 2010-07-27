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

#ifndef INCLUDED_LIBUHD_USRP_MISC_UTILS_HPP
#define INCLUDED_LIBUHD_USRP_MISC_UTILS_HPP

#include <uhd/config.hpp>
#include <uhd/wax.hpp>
#include <uhd/utils/gain_group.hpp>

namespace uhd{ namespace usrp{

    /*!
     * Create a gain group that represents the subdevice and its codec.
     */
    gain_group::sptr make_gain_group(wax::obj subdev, wax::obj codec);

}} //namespace

#endif /* INCLUDED_LIBUHD_USRP_MISC_UTILS_HPP */

