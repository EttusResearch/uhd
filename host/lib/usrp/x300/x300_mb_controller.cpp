//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "x300_mb_controller.hpp"

using namespace uhd::rfnoc;


uint64_t x300_mb_controller::x300_timekeeper::get_ticks_now()
{
    // tbw
    return 0;
}

uint64_t x300_mb_controller::x300_timekeeper::get_ticks_last_pps()
{
    // tbw
    return 0;
}

void x300_mb_controller::x300_timekeeper::set_ticks_now(const uint64_t ticks)
{
    // tbw
}

void x300_mb_controller::x300_timekeeper::set_ticks_next_pps(const uint64_t ticks)
{
    // tbw
}

void x300_mb_controller::x300_timekeeper::set_period(const uint64_t period_ns)
{
    // tbw
}

