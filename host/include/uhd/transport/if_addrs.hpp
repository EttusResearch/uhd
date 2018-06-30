//
// Copyright 2010-2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_TRANSPORT_IF_ADDRS_HPP
#define INCLUDED_UHD_TRANSPORT_IF_ADDRS_HPP

#include <uhd/config.hpp>
#include <string>
#include <vector>

namespace uhd{ namespace transport{

    /*!
     * The address for a network interface.
     */
    struct UHD_API if_addrs_t{
        std::string inet;
        std::string mask;
        std::string bcast;
    };

    /*!
     * Get a list of network interface addresses.
     * The internal implementation is system-dependent.
     * \return a vector of if addrs
     */
    UHD_API std::vector<if_addrs_t> get_if_addrs(void);

}} //namespace


#endif /* INCLUDED_UHD_TRANSPORT_IF_ADDRS_HPP */
