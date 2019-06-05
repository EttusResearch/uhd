//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "mpmd_mb_controller.hpp"

using namespace uhd::rfnoc;


uint64_t mpmd_mb_controller::mpmd_timekeeper::get_ticks_now()
{
    return _rpc->request_with_token<uint64_t>("get_timekeeper_time", _tk_idx, false);
}

uint64_t mpmd_mb_controller::mpmd_timekeeper::get_ticks_last_pps()
{
    return _rpc->request_with_token<uint64_t>("get_timekeeper_time", _tk_idx, true);
}

void mpmd_mb_controller::mpmd_timekeeper::set_ticks_now(const uint64_t ticks)
{
    _rpc->notify_with_token("set_timekeeper_time", _tk_idx, ticks, false);
}

void mpmd_mb_controller::mpmd_timekeeper::set_ticks_next_pps(const uint64_t ticks)
{
    _rpc->notify_with_token("set_timekeeper_time", _tk_idx, ticks, true);
}

void mpmd_mb_controller::mpmd_timekeeper::set_period(const uint64_t period_ns)
{
    _rpc->notify_with_token("set_tick_period", _tk_idx, period_ns);
}

