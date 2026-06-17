//
// Copyright 2026 Ettus Research
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/types/device_addr.hpp>
#include <uhdlib/utils/serial_number.hpp>

namespace uhd { namespace device_filter {

inline bool device_addr_matches(
    const uhd::device_addr_t& hint, const uhd::device_addr_t& discovered_addr)
{
    // Check if the discovered address matches the hint.
    // If a key is present in the hint, it must match the corresponding key in the
    // discovered address. If a key is not present in the hint, it is ignored in the
    // comparison.
    const auto hint_key_matches =
        [&hint, &discovered_addr](
            const std::string& key,
            bool (*compare)(const std::string&,
                const std::string&) = [](const std::string& lhs, const std::string& rhs) {
                return lhs == rhs;
            }) {
            return not hint.has_key(key)
                   or (discovered_addr.has_key(key)
                       and compare(hint[key], discovered_addr[key]));
        };

    return hint_key_matches("name")
           and (hint_key_matches("product") or not discovered_addr.has_key("product"))
           and (hint_key_matches("type")
                or hint["type"] == "sim") // special case for simulator
           and (hint_key_matches("serial", uhd::utils::serial_numbers_match));
}

inline uhd::device_addrs_t filter_device_addrs(
    const uhd::device_addrs_t& discovered_addrs, const uhd::device_addr_t& hint)
{
    uhd::device_addrs_t filtered_addrs;
    for (const auto& addr : discovered_addrs) {
        if (device_addr_matches(hint, addr)) {
            filtered_addrs.push_back(addr);
        }
    }
    return filtered_addrs;
}

}} // namespace uhd::device_filter
