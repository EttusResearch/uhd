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

#ifndef INCLUDED_LIBUHD_USRP_COMMON_VALIDATE_SUBDEV_SPEC_HPP
#define INCLUDED_LIBUHD_USRP_COMMON_VALIDATE_SUBDEV_SPEC_HPP

#include <uhd/config.hpp>
#include <uhd/usrp/subdev_spec.hpp>
#include <uhd/utils/assert_has.hpp>
#include <uhd/property_tree.hpp>
#include <string>

namespace uhd{ namespace usrp{

    //! Validate a subdev spec against a property tree
    void validate_subdev_spec(
        property_tree::sptr tree,
        const subdev_spec_t &spec,
        const std::string &type, //rx or tx
        const std::string &mb = "0"
    );

}} //namespace uhd::usrp

#endif /* INCLUDED_LIBUHD_USRP_COMMON_VALIDATE_SUBDEV_SPEC_HPP */
