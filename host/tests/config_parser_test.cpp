//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhdlib/utils/config_parser.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/assign/list_of.hpp>
#include <fstream>
#include <cstdio>

const std::string INI1_FILENAME = "test1.ini";
const std::string INI1 =
    "[section1]\n"
    "key1=value1\n"
    "key2=4\n"
    "\n"
    "; This is a comment\n"
    "[section2]\n"
    "key3=value with spaces\n"
    "key4= leading and trailing spaces \n"
;

const std::string INI2_FILENAME = "test2.ini";
const std::string INI2 =
    "[section2]\n"
    "key3=value with even more spaces\n"
    "\n"
    "[section3]\n"
    "key4=\"with quotes\"\n";

namespace {
    //! Create files that can be read by the CUT
    void make_config_parsers()
    {
        std::ofstream ini1(INI1_FILENAME);
        ini1 << INI1 << std::endl;
        ini1.close();
        std::ofstream ini2(INI2_FILENAME);
        ini2 << INI2 << std::endl;
        ini2.close();
    }

    //! Tidy up after us
    void cleanup_config_parsers()
    {
        std::remove(INI1_FILENAME.c_str());
        std::remove(INI2_FILENAME.c_str());
    }
}

BOOST_AUTO_TEST_CASE(test_config_parser){
    make_config_parsers();
    uhd::config_parser I(INI1_FILENAME);
    BOOST_CHECK_EQUAL(I.sections().size(), 2);
    BOOST_CHECK_EQUAL(I.options("section1").size(), 2);
    BOOST_CHECK_EQUAL(
        I.get<std::string>("section1", "key1"),
        "value1"
    );
    BOOST_CHECK_EQUAL(
        I.get<int>("section1", "key2"),
        4
    );
    BOOST_CHECK_EQUAL(
        I.get<std::string>("section2", "key3"),
        "value with spaces"
    );
    BOOST_CHECK_EQUAL(
        I.get<std::string>("section2", "key4"),
        "leading and trailing spaces"
    );
    BOOST_CHECK_EQUAL(
        I.get<std::string>("section2", "non_existent_key", "default"),
        "default"
    );
    BOOST_REQUIRE_THROW(
        I.get<std::string>("section2", "non_existent_key"),
        uhd::key_error
    );
    I.read_file(INI2_FILENAME);
    BOOST_CHECK_EQUAL(
        I.get<std::string>("section2", "key3"),
        "value with even more spaces"
    );
    BOOST_CHECK_EQUAL(
        I.get<std::string>("section1", "key1"),
        "value1"
    );
    BOOST_CHECK_EQUAL(
        I.get<std::string>("section3", "key4"),
        "\"with quotes\""
    );

    cleanup_config_parsers();
}
