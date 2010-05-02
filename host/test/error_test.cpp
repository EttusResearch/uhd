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
#include <uhd/utils/assert.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <vector>
#include <iostream>

BOOST_AUTO_TEST_CASE(test_assert_has){
    std::vector<int> vec;
    vec.push_back(2);
    vec.push_back(3);
    vec.push_back(5);

    //verify the std::has utility
    BOOST_CHECK(std::has(vec, 2));
    BOOST_CHECK(not std::has(vec, 1));

    std::cout << "The output of the assert_has error:" << std::endl;
    try{
        uhd::assert_has(vec, 1, "prime");
    }catch(const boost::exception &e){
        std::cout << boost::diagnostic_information(e) << std::endl;
    }
}
