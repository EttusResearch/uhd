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
#include <uhd/utils/msg.hpp>
#include <iostream>

BOOST_AUTO_TEST_CASE(test_messages){
    std::cerr << "---begin print test ---" << std::endl;
    UHD_MSG(status) <<
        "This is a test print for a status message.\n"
        "And this is the second line of the test print.\n"
    ;
    UHD_MSG(warning) <<
        "This is a test print for a warning message.\n"
        "And this is the second line of the test print.\n"
    ;
    UHD_MSG(error) <<
        "This is a test print for an error message.\n"
        "And this is the second line of the test print.\n"
    ;
    UHD_HERE();
    const int x = 42;
    UHD_VAR(x);
    std::cerr << "---end print test ---" << std::endl;
}
