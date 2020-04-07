//
// Copyright 2018 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/property_tree.hpp>
#include <uhdlib/utils/narrow.hpp>
#include <stdint.h>
#include <type_traits>
#include <functional>
#include <memory>

namespace uhd { namespace rfnoc {

class traffic_counter
{
public:
    typedef std::shared_ptr<traffic_counter> sptr;
    typedef std::function<void(const uint32_t addr, const uint32_t data)> write_reg_fn_t;
    typedef std::function<uint64_t(const uint32_t addr)> read_reg_fn_t;

    traffic_counter(uhd::property_tree::sptr tree,
        uhd::fs_path root_path,
        write_reg_fn_t write_reg_fn,
        read_reg_fn_t read_reg_fn)
        : _write_reg_fn(write_reg_fn), _read_reg_fn(read_reg_fn)
    {
        const uint32_t id_reg_offset        = 0;
        const uint32_t first_counter_offset = 1;
        const uint64_t traffic_counter_id   = 0x712AFF1C00000000ULL;

        // Check traffic counter id to determine if it's present
        const uint64_t id = _read_reg_fn(id_reg_offset);

        // If present, add properties
        if (id == traffic_counter_id) {
            tree->create<bool>(root_path / "traffic_counter/enable")
                .add_coerced_subscriber([this](const bool enable) {
                    uint32_t val = enable ? 1 : 0;
                    return _write_reg_fn(0, val);
                })
                .set(false);

            const char* counters[] = {"bus_clock_ticks",
                "xbar_to_shell_xfer_count",
                "xbar_to_shell_pkt_count",
                "shell_to_xbar_xfer_count",
                "shell_to_xbar_pkt_count",
                "shell_to_ce_xfer_count",
                "shell_to_ce_pkt_count",
                "ce_to_shell_xfer_count",
                "ce_to_shell_pkt_count"};

            for (size_t i = 0; i < std::extent<decltype(counters)>::value; i++) {
                tree->create<uint64_t>(root_path / "traffic_counter" / counters[i])
                    .set_publisher([this, i, first_counter_offset]() {
                        return _read_reg_fn(
                            uhd::narrow_cast<uint32_t>(i) + first_counter_offset);
                    });
            }
        }
    }

private:
    write_reg_fn_t _write_reg_fn;
    read_reg_fn_t _read_reg_fn;
};

}} /* namespace uhd::rfnoc */
