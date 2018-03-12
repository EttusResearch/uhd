//
// Copyright 2017 Ettus Research (National Instruments Corp.)
//
// SPDX-License-Identifier: GPL-3.0+
//

#include <boost/test/unit_test.hpp>
#include <uhd/types/time_spec.hpp>
#include "system_time.hpp"
#include <iostream>
#include <iomanip>
#include <cstdint>
#include <chrono>

BOOST_AUTO_TEST_CASE(test_time_spec_get_system_time){
    std::cout << "Testing time specification get system time..." << std::endl;

    //Not really checking for high resolution timing here,
    //just need to check that system time is minimally working.

    auto start = uhd::get_system_time();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    auto stop = uhd::get_system_time();

    auto diff = stop - start;
    std::cout << "start: " << start.get_real_secs() << std::endl;
    std::cout << "stop: " << stop.get_real_secs() << std::endl;
    std::cout << "diff: " << diff.get_real_secs() << std::endl;
    BOOST_CHECK(diff.get_real_secs() > 0); //assert positive
    BOOST_CHECK(diff.get_real_secs() < 1.0); //assert under 1s
}

