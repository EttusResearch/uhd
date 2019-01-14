//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhdlib/utils/eeprom_utils.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/test/unit_test.hpp>

using namespace boost::assign;

class upper_case_char
{
public:
    upper_case_char(char ch)
    {
        value = ch;
    }
    static upper_case_char from_string(std::string str)
    {
        return upper_case_char(toupper(str[0]));
    }
    std::string to_string()
    {
        return std::string(1, value);
    }

private:
    char value;
};

BOOST_AUTO_TEST_CASE(test_eeprom_duplicate_check)
{
    const uhd::dict<std::string, std::string> curr_eeprom =
        map_list_of("0", "A")("1", "B")("2", "C");
    const uhd::dict<std::string, std::string> new_eeprom_no_dups =
        map_list_of("0", "d")("1", "e");
    const uhd::dict<std::string, std::string> new_eeprom_dups_in_curr =
        map_list_of("0", "b");
    const uhd::dict<std::string, std::string> new_eeprom_dups_in_new =
        map_list_of("0", "c")("1", "c");
    const uhd::dict<std::string, std::string> new_eeprom_dups_in_both =
        map_list_of("0", "b")("1", "B");
    const std::vector<std::string> keys = {"0", "1", "2"};

    BOOST_CHECK_EQUAL(check_for_duplicates<upper_case_char>(
                          "TEST", new_eeprom_no_dups, curr_eeprom, "Test Value", keys),
        false);
    BOOST_CHECK(check_for_duplicates<upper_case_char>(
        "TEST", new_eeprom_dups_in_curr, curr_eeprom, "Test Value", keys));
    BOOST_CHECK(check_for_duplicates<upper_case_char>(
        "TEST", new_eeprom_dups_in_new, curr_eeprom, "Test Value", keys));
    BOOST_CHECK(check_for_duplicates<upper_case_char>(
        "TEST", new_eeprom_dups_in_both, curr_eeprom, "Test Value", keys));
}
