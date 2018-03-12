//
// Copyright 2010-2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <boost/test/unit_test.hpp>
#include <uhd/types/dict.hpp>
#include <boost/assign/list_of.hpp>

BOOST_AUTO_TEST_CASE(test_dict_init){
    uhd::dict<int, int> d;
    d[-1] = 3;
    d[0] = 4;
    d[1] = 5;
    BOOST_CHECK(d.has_key(0));
    BOOST_CHECK(not d.has_key(2));
    BOOST_CHECK(d.keys()[1] == 0);
    BOOST_CHECK(d.vals()[1] == 4);
    BOOST_CHECK_EQUAL(d[-1], 3);
}

BOOST_AUTO_TEST_CASE(test_dict_assign){
    uhd::dict<int, int> d = boost::assign::map_list_of
        (-1, 3)
        (0, 4)
        (1, 5)
    ;
    BOOST_CHECK(d.has_key(0));
    BOOST_CHECK(not d.has_key(2));
    BOOST_CHECK(d.keys()[1] == 0);
    BOOST_CHECK(d.vals()[1] == 4);
    BOOST_CHECK_EQUAL(d[-1], 3);
}

BOOST_AUTO_TEST_CASE(test_const_dict){
    const uhd::dict<int, int> d = boost::assign::map_list_of
        (-1, 3)
        (0, 4)
        (1, 5)
    ;
    BOOST_CHECK(d.has_key(0));
    BOOST_CHECK(not d.has_key(2));
    BOOST_CHECK(d.keys()[1] == 0);
    BOOST_CHECK(d.vals()[1] == 4);
    BOOST_CHECK_EQUAL(d[-1], 3);
    BOOST_CHECK_THROW(d[2], std::exception);
}

BOOST_AUTO_TEST_CASE(test_dict_pop){
    uhd::dict<int, int> d = boost::assign::map_list_of
        (-1, 3)
        (0, 4)
        (1, 5)
    ;
    BOOST_CHECK(d.has_key(0));
    BOOST_CHECK_EQUAL(d.pop(0), 4);
    BOOST_CHECK(not d.has_key(0));
    BOOST_CHECK(d.keys()[0] == -1);
    BOOST_CHECK(d.keys()[1] == 1);
}

BOOST_AUTO_TEST_CASE(test_dict_update)
{
    uhd::dict<std::string, std::string> d1 = boost::assign::map_list_of
        ("key1", "val1")
        ("key2", "val2")
    ;
    uhd::dict<std::string, std::string> d2 = boost::assign::map_list_of
        ("key2", "val2x")
        ("key3", "val3")
    ;

    d1.update(d2, false /* don't throw cause of conflict */);
    BOOST_CHECK_EQUAL(d1["key1"], "val1");
    BOOST_CHECK_EQUAL(d1["key2"], "val2x");
    BOOST_CHECK_EQUAL(d1["key3"], "val3");

    uhd::dict<std::string, std::string> d3 = boost::assign::map_list_of
        ("key1", "val1")
        ("key2", "val2")
    ;
    BOOST_CHECK_THROW(d3.update(d2), uhd::value_error);
}


