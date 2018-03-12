//
// Copyright 2010-2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <boost/test/unit_test.hpp>
#include <uhd/utils/tasks.hpp>
#include <thread>
#include <chrono>
#include <vector>
#include <iostream>

void test_tasks_sleep(size_t usecs)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(usecs));
}

BOOST_AUTO_TEST_CASE(tasks_test) {

    static const size_t N_TASKS = 100;
    std::vector<uhd::task::sptr> test_vec;

    for (size_t i = 0; i < N_TASKS; i++) {
        test_vec.push_back(uhd::task::make([i](){ test_tasks_sleep(i); }));
    }
}
