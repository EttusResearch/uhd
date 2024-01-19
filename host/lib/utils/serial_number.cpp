//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhdlib/utils/serial_number.hpp>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace uhd { namespace utils {
bool serial_numbers_match(const std::string& serial_a, const std::string& serial_b)
{
    try {
        const uint32_t a = std::stoi(serial_a, 0, 16);
        const uint32_t b = std::stoi(serial_b, 0, 16);
        return a == b;
    } catch (std::out_of_range& e) {
        return false;
    } catch (std::invalid_argument& e) {
        return false;
    }
}
}} // namespace uhd::utils
