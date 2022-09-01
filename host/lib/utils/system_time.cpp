//
// Copyright 2017 Ettus Research (National Instruments Corp.)
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhdlib/utils/system_time.hpp>
#include <chrono>

uhd::time_spec_t uhd::get_system_time(void)
{
    const auto now     = std::chrono::steady_clock::now().time_since_epoch();
    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now);
    const auto nanoseconds =
        std::chrono::duration_cast<std::chrono::nanoseconds>(now - seconds);
    return uhd::time_spec_t(seconds.count(), nanoseconds.count(), 1e9);
}
