//
// Copyright 2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/utils/static.hpp>
#include <stdexcept>
#include <iostream>
_uhd_static_fixture::_uhd_static_fixture(void (*fcn)(void), const char *name){
    try{
        fcn();
    }
    catch(const std::exception &e){
        std::cerr << "Exception in static block " << name << std::endl;
        std::cerr << "  " << e.what() << std::endl;
    }
    catch(...){
        std::cerr << "Exception in static block " << name << std::endl;
    }
}
