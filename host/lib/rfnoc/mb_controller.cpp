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
#include <future>
#include <thread>
#include <tuple>

using namespace uhd::rfnoc;
using namespace std::chrono_literals;

namespace {
const std::vector<std::string> SYNCHRONIZABLE_REF_SOURCES = {"gpsdo", "external"};

/*! Synchronize a list of timekeepers
 *
 * Function is agnostic to the owning motherboard and works either
 * on a single device/motherboard or accross multiple devices/motherboards.
 *
 * \param timekeepers A list of timekeepers to synchronize.
 * \param timespec    Time specification to synchronize \p timekeepers to.
 * \param quiet       If true, don't print errors or warnings
 *                    if synchronization fails
 * \returns true if all timekeepers that were listed on \p timekeepers
 *               could be synchronized.
 *
 */
bool sync_tks(
    std::vector<std::tuple<size_t, size_t, uhd::rfnoc::mb_controller::timekeeper::sptr>>
        timekeepers,
    const uhd::time_spec_t time_spec,
    const bool quiet)
{
    if (!quiet) {
        UHD_LOGGER_INFO("MB_CTRL")
            << "Synchronizing " << timekeepers.size() << " timekeepers";
    } else {
        UHD_LOGGER_DEBUG("MB_CTRL")
            << "Synchronizing " << timekeepers.size() << " timekeepers";
    }

    // The best way to synchronize timekeepers depends on how many devices and
    // how many timekeepers per device we have. Here, we handle two different
    // cases:
    // Case 1: Single device, single timekeeper, no need to sync to PPS
    if (timekeepers.size() == 1) {
        std::get<2>(timekeepers.front())->set_time_now(time_spec);
        return true;
    }

    // Case 2: We have multiple timekeepers. To sync them, we need to use PPS.
    if (!quiet) {
        UHD_LOGGER_INFO("MB_CTRL")
            << "  Synchronizing timekeepers to Board " << std::get<0>(timekeepers.front())
            << "/TK " << std::get<1>(timekeepers.front());
        UHD_LOGGER_INFO("MB_CTRL") << "    1) catch time transition at pps edge";
    }
    const auto end_time = std::chrono::steady_clock::now() + 1100ms;
    const uhd::time_spec_t time_start_last_pps =
        std::get<2>(timekeepers.front())->get_time_last_pps();
    while (time_start_last_pps == std::get<2>(timekeepers.front())->get_time_last_pps()) {
        if (std::chrono::steady_clock::now() > end_time) {
            // This is always bad, and we'll throw regardless of quiet
            throw uhd::runtime_error("Board "
                                     + std::to_string(std::get<0>(timekeepers.front()))
                                     + " may not be getting a PPS signal!\n"
                                       "No PPS detected within the time interval.\n"
                                       "See the application notes for your device.\n");
        }
        std::this_thread::sleep_for(1ms);
    }

    if (!quiet) {
        UHD_LOGGER_INFO("MB_CTRL") << "    2) set times next pps (synchronously)";
    }

    for (auto& timekeeper : timekeepers) {
        std::get<2>(timekeeper)->set_time_next_pps(time_spec);
    }
    std::this_thread::sleep_for(1s);

    // verify that the time registers are read to be within a few RTT
    for (auto& timekeeper : timekeepers) {
        size_t mb_idx, tk_idx;
        uhd::rfnoc::mb_controller::timekeeper::sptr tk;
        std::tie(mb_idx, tk_idx, tk) = timekeeper;

        uhd::time_spec_t time_0 = std::get<2>(timekeepers.front())->get_time_now();
        uhd::time_spec_t time_i = tk->get_time_now();
        // 10 ms: greater than RTT but not too big
        constexpr double MAX_DEVIATION = 0.01;
        if (time_i < time_0 or (time_i - time_0) > uhd::time_spec_t(MAX_DEVIATION)) {
            const auto warn_str = str(
                boost::format(
                    "Detected time deviation between board %1%/TK %2% and board %5%.\n"
                    "Board %5%/TK %6% time is %3% seconds.\n"
                    "Board %1%/TK %2% time is %4% seconds.\n")
                % mb_idx % tk_idx % time_0.get_real_secs() % time_i.get_real_secs()
                % std::get<0>(timekeepers.front()) % std::get<1>(timekeepers.front()));

            if (!quiet) {
                UHD_LOG_WARNING("MB_CTRL", warn_str);
            } else {
                UHD_LOG_DEBUG("MB_CTRL", warn_str);
            }
            return false;
        }
    }
    return true;
}


} // namespace

bool mb_controller::synchronize(std::vector<mb_controller::sptr>& mb_controllers,
    const uhd::time_spec_t& time_spec,
    const bool quiet)
{
    if (mb_controllers.empty()) {
        return false;
    }
    // Verify that all devices share a time reference, and that it is a common
    // one
    const std::string time_source = mb_controllers.at(0)->get_time_source();
    if (!uhd::has(SYNCHRONIZABLE_REF_SOURCES, time_source) && mb_controllers.size() > 1
        && !quiet) {
        UHD_LOG_WARNING("MB_CTRL",
            "The selected time source "
                << time_source << " does not allow synchronization between devices.");
    }
    for (auto& mbc : mb_controllers) {
        if (mbc->get_time_source() != time_source) {
            if (!quiet) {
                UHD_LOG_WARNING("MB_CTRL",
                    "Motherboards do not share a time source, and thus cannot be "
                    "synchronized!");
            }
        }
    }

    // Case 1: All motherboards are synchronized. We then sync all available
    // timekeepers at once.
    if (uhd::has(SYNCHRONIZABLE_REF_SOURCES, time_source)) {
        // Get a reference to all timekeepers, even if we have multiple TKs per mboard
        std::vector<std::tuple<size_t, size_t, timekeeper::sptr>> timekeepers;
        timekeepers.reserve(
            mb_controllers.size() * mb_controllers.front()->get_num_timekeepers());
        for (size_t mb_index = 0; mb_index < mb_controllers.size(); mb_index++) {
            auto& mbc = mb_controllers[mb_index];
            for (size_t tk = 0; tk < mbc->get_num_timekeepers(); tk++) {
                timekeepers.push_back({mb_index, tk, mbc->get_timekeeper(tk)});
            }
        }

        return sync_tks(timekeepers, time_spec, quiet);
    }

    // Case 2: Boards in UHD session are not synchronized (e.g., are using an 'internal'
    // clock source). In this case, we still want to sync all timekeepers that are
    // within any single board at once. If we have multiple timekeepers per board, this
    // could take up to 2 seconds per board. We therefore parallelize the sync calls
    // on all the boards.
    std::vector<std::future<bool>> sync_tasks;
    sync_tasks.reserve(mb_controllers.size());
    for (size_t mb_idx = 0; mb_idx < mb_controllers.size(); ++mb_idx) {
        // Get a reference to all timekeepers per mboard
        std::vector<std::tuple<size_t, size_t, timekeeper::sptr>> timekeepers;
        timekeepers.reserve(mb_controllers.at(mb_idx)->get_num_timekeepers());
        auto& mbc = mb_controllers[mb_idx];
        for (size_t tk = 0; tk < mbc->get_num_timekeepers(); tk++) {
            timekeepers.push_back({mb_idx, tk, mbc->get_timekeeper(tk)});
        }
        // parameter timekeepers can not be passed by reference
        sync_tasks.emplace_back(std::async(std::launch::async,
            [&, timekeepers]() { return sync_tks(timekeepers, time_spec, quiet); }));
    }

    // collect results from async sync_tasks
    std::list<bool> collated_sync_tks;
    for (auto& sync_task : sync_tasks) {
        try {
            collated_sync_tks.push_back(sync_task.get());
        } catch (const std::exception& e) {
            UHD_LOGGER_ERROR("MB_CTRL") << "Synchronization error: " << e.what();
            return false;
        }
    }

    return std::all_of(
        collated_sync_tks.cbegin(), collated_sync_tks.cend(), [](bool i) { return i; });
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
