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
#include <uhd/exception.hpp>
#include <uhd/utils/assert_has.hpp>
#include <vector>
#include <iostream>

BOOST_AUTO_TEST_CASE(test_exception_methods){
    try{
        throw uhd::assertion_error("your assertion failed: 1 != 2");
    }
    catch(const uhd::exception &e){
        std::cout << "what: " << e.what() << std::endl;
        std::cout << "code: " << e.code() << std::endl;
    }
}

BOOST_AUTO_TEST_CASE(test_assert_has){
    std::vector<int> vec;
    vec.push_back(2);
    vec.push_back(3);
    vec.push_back(5);

    //verify the uhd::has utility
    BOOST_CHECK(uhd::has(vec, 2));
    BOOST_CHECK(not uhd::has(vec, 1));

    std::cout << "The output of the assert_has error:" << std::endl;
    try{
        uhd::assert_has(vec, 1, "prime");
    }
    catch(const std::exception &e){
        std::cout << e.what() << std::endl;
    }
}

BOOST_AUTO_TEST_CASE(test_assert_throw){
    std::cout << "The output of the assert throw error:" << std::endl;
    try{
        UHD_ASSERT_THROW(2 + 2 == 5);
    }
    catch(const std::exception &e){
        std::cout << e.what() << std::endl;
    }
}

BOOST_AUTO_TEST_CASE(test_exception_dynamic){
    uhd::exception *exception_clone;

    //throw an exception and dynamically clone it
    try{
        throw uhd::runtime_error("noooooo");
    }
    catch(const uhd::exception &e){
        std::cout << e.what() << std::endl;
        exception_clone = e.dynamic_clone();
    }

    //now we dynamically re-throw the exception
    try{
        exception_clone->dynamic_throw();
    }
    catch(const uhd::assertion_error &e){
        std::cout << e.what() << std::endl;
        BOOST_CHECK(false);
    }
    catch(const uhd::runtime_error &e){
        std::cout << e.what() << std::endl;
        BOOST_CHECK(true);
    }
    catch(const uhd::exception &e){
        std::cout << e.what() << std::endl;
        BOOST_CHECK(false);
    }

    delete exception_clone; //manual cleanup
}
