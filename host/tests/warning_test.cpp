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

#include <boost/test/unit_test.hpp>
#include <uhd/utils/warning.hpp>
#include <iostream>

BOOST_AUTO_TEST_CASE(test_warning_post){
    std::cerr << "---begin print test ---" << std::endl;
    uhd::warning::post(
        "This is a test print for a warning message.\n"
        "And this is the second line of the test print.\n"
    );
    std::cerr << "---end print test ---" << std::endl;
}
