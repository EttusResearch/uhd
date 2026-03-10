//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/utils/cast.hpp>
#include <type_traits>
#include <boost/algorithm/string.hpp>
#include <limits>

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
        size_t pos;
        double result = std::stod(val, &pos);
        if (pos != val.length()) {
            throw std::invalid_argument("trailing characters");
        }
        return result;
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
        size_t pos;
        int result = std::stoi(val, &pos);
        if (pos != val.length()) {
            throw std::invalid_argument("trailing characters");
        }
        return result;
    } catch (std::invalid_argument&) {
        throw uhd::runtime_error(std::string("Cannot convert `") + val + "' to int!");
    } catch (std::out_of_range&) {
        throw uhd::runtime_error(std::string("Cannot convert `") + val + "' to int!");
    }
}

template <>
uint8_t cast::from_str(const std::string& val)
{
    try {
        // Check for negative numbers manually since stoul will wrap them around
        if (!val.empty() && val[0] == '-') {
            throw std::out_of_range("negative value for uint8_t");
        }
        size_t pos;
        unsigned long tmp = std::stoul(val, &pos);
        if (pos != val.length()) {
            throw std::invalid_argument("trailing characters");
        }
        if (tmp > std::numeric_limits<uint8_t>::max()) {
            throw std::out_of_range("value out of range for uint8_t");
        }
        return static_cast<uint8_t>(tmp);
    } catch (std::invalid_argument&) {
        throw uhd::runtime_error(std::string("Cannot convert `") + val + "' to uint8_t!");
    } catch (std::out_of_range&) {
        throw uhd::runtime_error(std::string("Cannot convert `") + val + "' to uint8_t!");
    }
}

template <>
uint16_t cast::from_str(const std::string& val)
{
    try {
        // Check for negative numbers manually since stoul will wrap them around
        if (!val.empty() && val[0] == '-') {
            throw std::out_of_range("negative value for uint16_t");
        }
        size_t pos;
        unsigned long tmp = std::stoul(val, &pos);
        if (pos != val.length()) {
            throw std::invalid_argument("trailing characters");
        }
        if (tmp > std::numeric_limits<uint16_t>::max()) {
            throw std::out_of_range("value out of range for uint16_t");
        }
        return static_cast<uint16_t>(tmp);
    } catch (std::invalid_argument&) {
        throw uhd::runtime_error(
            std::string("Cannot convert `") + val + "' to uint16_t!");
    } catch (std::out_of_range&) {
        throw uhd::runtime_error(
            std::string("Cannot convert `") + val + "' to uint16_t!");
    }
}

template <>
uint32_t cast::from_str(const std::string& val)
{
    try {
        // Check for negative numbers manually since stoul will wrap them around
        if (!val.empty() && val[0] == '-') {
            throw std::out_of_range("negative value for uint32_t");
        }
        size_t pos;
        unsigned long tmp = std::stoul(val, &pos);
        if (pos != val.length()) {
            throw std::invalid_argument("trailing characters");
        }
        if (tmp > std::numeric_limits<uint32_t>::max()) {
            throw std::out_of_range("value out of range for uint32_t");
        }
        return static_cast<uint32_t>(tmp);
    } catch (std::invalid_argument&) {
        throw uhd::runtime_error(
            std::string("Cannot convert `") + val + "' to uint32_t!");
    } catch (std::out_of_range&) {
        throw uhd::runtime_error(
            std::string("Cannot convert `") + val + "' to uint32_t!");
    }
}

template <>
uint64_t cast::from_str(const std::string& val)
{
    try {
        // Check for negative numbers manually since stoull will wrap them around
        if (!val.empty() && val[0] == '-') {
            throw std::out_of_range("negative value for uint64_t");
        }
        size_t pos;
        unsigned long long tmp = std::stoull(val, &pos);
        if (pos != val.length()) {
            throw std::invalid_argument("trailing characters");
        }
        // uint64_t and unsigned long long should be the same size on most platforms
        return static_cast<uint64_t>(tmp);
    } catch (std::invalid_argument&) {
        throw uhd::runtime_error(
            std::string("Cannot convert `") + val + "' to uint64_t!");
    } catch (std::out_of_range&) {
        throw uhd::runtime_error(
            std::string("Cannot convert `") + val + "' to uint64_t!");
    }
}

std::string cast::to_ordinal_string(int val)
{
    const std::string ordinals = "thstndrd";
    std::string result         = std::to_string(val);
    int index = (val % 10) * (val % 10 < 4 && !(10 < val % 100 && val % 100 < 14));
    result += ordinals.substr(2 * index, 2);
    return result;
}
