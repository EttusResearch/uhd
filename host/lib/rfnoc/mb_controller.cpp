//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/rfnoc/mb_controller.hpp>
#include <uhd/utils/algorithm.hpp>
#include <uhd/utils/log.hpp>
#include <atomic>
#include <chrono>
#include <thread>

using namespace uhd::rfnoc;
using namespace std::chrono_literals;

namespace {
const std::vector<std::string> SYNCHRONIZABLE_REF_SOURCES = {"gpsdo", "external"};
}

bool mb_controller::synchronize(std::vector<mb_controller::sptr>& mb_controllers,
    const uhd::time_spec_t& time_spec,
    const bool quiet)
{
    if (mb_controllers.empty()) {
        return false;
    }
    if (mb_controllers.size() == 1) {
        UHD_LOG_TRACE("MB_CTRL", "Skipping time synchronization of a single USRP.");
        mb_controllers.at(0)->get_timekeeper(0)->set_time_now(time_spec);
        return true;
    }
    // Verify that all devices share a time reference, and that it is a common
    // one
    const std::string time_source = mb_controllers.at(0)->get_time_source();
    if (!uhd::has(SYNCHRONIZABLE_REF_SOURCES, time_source)) {
        if (!quiet) {
            UHD_LOG_WARNING("MB_CTRL",
                "The selected time source "
                    << time_source << " does not allow synchronization between devices.");
        }
        return false;
    }
    for (auto& mbc : mb_controllers) {
        if (mbc->get_time_source() != time_source) {
            if (!quiet) {
                UHD_LOG_WARNING("MB_CTRL",
                    "Motherboards do not share a time source, and thus cannot be "
                    "synchronized!");
            }
            return false;
        }
    }

    // Get a reference to all timekeepers
    std::vector<timekeeper::sptr> timekeepers;
    timekeepers.reserve(mb_controllers.size());
    for (auto& mbc : mb_controllers) {
        // If we also want to sync other timekeepers, this would be the place to
        // do that
        timekeepers.push_back(mbc->get_timekeeper(0));
    }

    if (!quiet) {
        UHD_LOGGER_INFO("MB_CTRL") << "    1) catch time transition at pps edge";
    }
    const auto end_time                   = std::chrono::steady_clock::now() + 1100ms;
    const time_spec_t time_start_last_pps = timekeepers.front()->get_time_last_pps();
    while (time_start_last_pps == timekeepers.front()->get_time_last_pps()) {
        if (std::chrono::steady_clock::now() > end_time) {
            // This is always bad, and we'll throw regardless of quiet
            throw uhd::runtime_error("Board 0 may not be getting a PPS signal!\n"
                                     "No PPS detected within the time interval.\n"
                                     "See the application notes for your device.\n");
        }
        std::this_thread::sleep_for(1ms);
    }

    if (!quiet) {
        UHD_LOGGER_INFO("MB_CTRL") << "    2) set times next pps (synchronously)";
    }

    for (auto& timekeeper : timekeepers) {
        timekeeper->set_time_next_pps(time_spec);
    }
    std::this_thread::sleep_for(1s);

    // verify that the time registers are read to be within a few RTT
    size_t m = 0;
    for (auto& timekeeper : timekeepers) {
        time_spec_t time_0 = timekeepers.front()->get_time_now();
        time_spec_t time_i = timekeeper->get_time_now();
        // 10 ms: greater than RTT but not too big
        constexpr double MAX_DEVIATION = 0.01;
        if (time_i < time_0 or (time_i - time_0) > time_spec_t(MAX_DEVIATION)) {
            if (!quiet) {
                UHD_LOGGER_WARNING("MULTI_USRP")
                    << boost::format(
                           "Detected time deviation between board %d and board 0.\n"
                           "Board 0 time is %f seconds.\n"
                           "Board %d time is %f seconds.\n")
                           % m % time_0.get_real_secs() % m % time_i.get_real_secs();
            }
            return false;
        }
        m++;
    }
    return true;
}

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

void mb_controller::timekeeper::set_time_now(const uhd::time_spec_t& time)
{
    set_ticks_now(time.to_ticks(_tick_rate));
}

void mb_controller::timekeeper::set_time_next_pps(const uhd::time_spec_t& time)
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
    const uint64_t period_ns =
        static_cast<uint64_t>(1e9 / tick_rate * (uint64_t(1) << 32));
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

std::vector<std::string> mb_controller::get_gpio_banks() const
{
    return {};
}

std::vector<std::string> mb_controller::get_gpio_srcs(const std::string&) const
{
    throw uhd::not_implemented_error(
        "get_gpio_srcs() not supported on this motherboard!");
}

std::vector<std::string> mb_controller::get_gpio_src(const std::string&)
{
    throw uhd::not_implemented_error("get_gpio_src() not supported on this motherboard!");
}

void mb_controller::set_gpio_src(const std::string&, const std::vector<std::string>&)
{
    throw uhd::not_implemented_error("set_gpio_src() not supported on this motherboard!");
}

void mb_controller::register_sync_source_updater(mb_controller::sync_source_updater_t)
{
    throw uhd::not_implemented_error(
        "register_sync_source_updater() not supported on this motherboard!");
}
