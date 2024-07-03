//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/utils/cast.hpp>
#include <boost/algorithm/string.hpp>

using namespace uhd;

template <>
bool cast::from_str(const std::string& val)
{
    try {
        return (std::stoi(val) != 0);
    } catch (std::exception& ex) {
        if (boost::algorithm::to_lower_copy(val) == "true"
            || boost::algorithm::to_lower_copy(val) == "yes"
            || boost::algorithm::to_lower_copy(val) == "y" || val == "1") {
            return true;
        } else if (boost::algorithm::to_lower_copy(val) == "false"
                   || boost::algorithm::to_lower_copy(val) == "no"
                   || boost::algorithm::to_lower_copy(val) == "n" || val == "0") {
            return false;
        } else {
            throw uhd::runtime_error("Cannot convert `" + val + "' to boolean!");
            return false;
        }
    }
}

template <>
double cast::from_str(const std::string& val)
{
    try {
        return std::stod(val);
    } catch (std::invalid_argument&) {
        throw uhd::runtime_error(std::string("Cannot convert `") + val + "' to double!");
    } catch (std::out_of_range&) {
        throw uhd::runtime_error(std::string("Cannot convert `") + val + "' to double!");
    }
}

template <>
int cast::from_str(const std::string& val)
{
    try {
        return std::stoi(val);
    } catch (std::invalid_argument&) {
        throw uhd::runtime_error(std::string("Cannot convert `") + val + "' to int!");
    } catch (std::out_of_range&) {
        throw uhd::runtime_error(std::string("Cannot convert `") + val + "' to int!");
    }
}

template <>
std::string cast::from_str(const std::string& val)
{
    return val;
}

std::string cast::to_ordinal_string(int val)
{
    const std::string ordinals = "thstndrd";
    std::string result         = std::to_string(val);
    int index = (val % 10) * (val % 10 < 4 && !(10 < val % 100 && val % 100 < 14));
    result += ordinals.substr(2 * index, 2);
    return result;
}
