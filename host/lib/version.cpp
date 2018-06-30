//
// Copyright 2010-2012 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/version.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/utils/log.hpp>
#include <boost/version.hpp>
#include <iostream>

std::string uhd::get_version_string(void){
    return "@UHD_VERSION@";
}

std::string uhd::get_abi_string(void){
    return UHD_VERSION_ABI_STRING;
}
