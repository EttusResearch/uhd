//
// Copyright 2010-2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
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
