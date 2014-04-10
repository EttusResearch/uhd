//
// Copyright 2014 Ettus Research LLC
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

#include <iostream>
#include <boost/test/unit_test.hpp>
#include <boost/cstdint.hpp>
#include <uhd/utils/cast.hpp>

BOOST_AUTO_TEST_CASE(test_mac_addr){
    std::string in = "0x0100";
    boost::uint16_t correct_result = 256;
    boost::uint16_t x = uhd::cast::hexstr_cast<boost::uint16_t>(in);
    //boost::uint16_t x = uhd::cast::hexstr_cast(in);
    std::cout
        << "Testing hex -> uint16_t conversion. "
        << in << " == " << std::hex << x << "?" << std::endl;
    BOOST_CHECK_EQUAL(x, correct_result);
}

