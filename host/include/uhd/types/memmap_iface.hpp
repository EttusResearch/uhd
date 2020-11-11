//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/types/time_spec.hpp>
#include <stdint.h>
#include <functional>

namespace uhd {

//! Represents a 32-bit, memory-mapped interface (peek/poke).
// This is a simplified version of wb_iface, useful for those cases when we use
// closures to provide bespoke peek/poke interfaces to objects.
struct UHD_API memmap32_iface
{
    using poke32_fn_t = std::function<void(const uint32_t addr, const uint32_t data)>;
    using peek32_fn_t = std::function<uint32_t(const uint32_t addr)>;

    poke32_fn_t poke32;
    peek32_fn_t peek32;
};

//! Represents a 32-bit, memory-mapped interface (peek/poke).
//
// The difference to memmap32_iface is that the poke command will require a command
// time.
struct UHD_API memmap32_iface_timed
{
    using poke32_fn_t = std::function<void(
        const uint32_t addr, const uint32_t data, const uhd::time_spec_t& time)>;
    using peek32_fn_t = std::function<uint32_t(const uint32_t addr)>;

    poke32_fn_t poke32;
    peek32_fn_t peek32;
};

} // namespace uhd
