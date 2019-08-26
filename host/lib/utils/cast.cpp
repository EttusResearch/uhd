//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/utils/cast.hpp>

using namespace uhd;

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
