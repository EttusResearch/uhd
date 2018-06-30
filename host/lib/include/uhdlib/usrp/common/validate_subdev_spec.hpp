//
// Copyright 2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_USRP_COMMON_VALIDATE_SUBDEV_SPEC_HPP
#define INCLUDED_LIBUHD_USRP_COMMON_VALIDATE_SUBDEV_SPEC_HPP

#include <uhd/config.hpp>
#include <uhd/usrp/subdev_spec.hpp>
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
