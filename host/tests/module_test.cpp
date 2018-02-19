//
// Copyright 2010-2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/utils/static.hpp>
#include <iostream>

UHD_STATIC_BLOCK(module_test){
    std::cout << "---------------------------------------" << std::endl;
    std::cout << "-- Good news, everyone!" << std::endl;
    std::cout << "-- The test module has been loaded." << std::endl;
    std::cout << "---------------------------------------" << std::endl;
}
