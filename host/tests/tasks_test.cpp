//
// Copyright 2010-2011 Ettus Research LLC
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
