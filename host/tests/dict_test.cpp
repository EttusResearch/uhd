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

BOOST_AUTO_TEST_CASE(test_dict_equals)
{
    typedef uhd::dict<std::string, std::string> dict_ss;
    // Original dict
    dict_ss d0;
    d0["key1"] = "val1";
    d0["key2"] = "val2";
    // Same keys and vals as d1, but different order
    dict_ss d1;
    d1["key2"] = "val2";
    d1["key1"] = "val1";
    // Same vals, different keys
    dict_ss d2;
    d2["key1"] = "val1";
    d2["key3"] = "val2";
    // Same keys, different vals
    dict_ss d3;
    d3["key1"] = "val1";
    d3["key2"] = "val3";
    // Superset of d0
    dict_ss d4;
    d4["key1"] = "val1";
    d4["key2"] = "val2";
    d4["key4"] = "val3";
    // Subset of d0
    dict_ss d5;
    d5["key1"] = "val1";

    // Check that d0 and d1 are equal
    BOOST_CHECK(d0 == d1);
    BOOST_CHECK(d1 == d0);
    // Check that all other dictionaries are not equal to d0
    BOOST_CHECK(d0 != d2);
    BOOST_CHECK(d0 != d3);
    BOOST_CHECK(d0 != d4);
    BOOST_CHECK(d0 != d5);
    // Redundant, but just to be sure
    BOOST_CHECK(not (d0 == d2));
    BOOST_CHECK(not (d0 == d3));
}
