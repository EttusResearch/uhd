//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/rfnoc/mb_controller.hpp>
#include <uhd/exception.hpp>
#include <atomic>

using namespace uhd::rfnoc;


/******************************************************************************
 * Timekeeper API
 *****************************************************************************/
mb_controller::timekeeper::timekeeper()
{
    // nop
}

uhd::time_spec_t mb_controller::timekeeper::get_time_now()
{
    return time_spec_t::from_ticks(get_ticks_now(), _tick_rate);
}

uhd::time_spec_t mb_controller::timekeeper::get_time_last_pps()
{
    return time_spec_t::from_ticks(get_ticks_last_pps(), _tick_rate);
}

void mb_controller::timekeeper::set_time_now(const uhd::time_spec_t &time)
{
    set_ticks_now(time.to_ticks(_tick_rate));
}

void mb_controller::timekeeper::set_time_next_pps(const uhd::time_spec_t &time)
{
    set_ticks_next_pps(time.to_ticks(_tick_rate));
}

void mb_controller::timekeeper::set_tick_rate(const double tick_rate)
{
    if (_tick_rate == tick_rate) {
        return;
    }
    _tick_rate = tick_rate;

    // The period is the inverse of the tick rate, normalized by nanoseconds,
    // and represented as Q32 (e.g., period == 1ns means period_ns == 1<<32)
    const uint64_t period_ns = static_cast<uint64_t>(1e9 / tick_rate * (uint64_t(1) << 32));
    set_period(period_ns);
}

size_t mb_controller::get_num_timekeepers() const
{
    return _timekeepers.size();
}

mb_controller::timekeeper::sptr mb_controller::get_timekeeper(const size_t tk_idx) const
{
    if (!_timekeepers.count(tk_idx)) {
        throw uhd::index_error(
            std::string("No timekeeper with index ") + std::to_string(tk_idx));
    }

    return _timekeepers.at(tk_idx);
}

void mb_controller::register_timekeeper(const size_t idx, timekeeper::sptr tk)
{
    _timekeepers.emplace(idx, std::move(tk));
}
