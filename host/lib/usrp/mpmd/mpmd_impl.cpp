//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "mpmd_impl.hpp"
#include <uhd/exception.hpp>
#include <uhd/types/component_file.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/utils/tasks.hpp>
#include <uhdlib/utils/prefs.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/thread.hpp>
#include <chrono>
#include <future>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <vector>

using namespace uhd;
using namespace uhd::mpmd;

namespace {
/*************************************************************************
 * Local constants
 ************************************************************************/
//! Most pessimistic time for a CHDR query to go to device and back
const double MPMD_CHDR_MAX_RTT = 0.02;
//! MPM Compatibility number {MAJOR, MINOR}
const std::vector<size_t> MPM_COMPAT_NUM = {4, 0};

/*************************************************************************
 * Helper functions
 ************************************************************************/
void reset_time_synchronized(uhd::property_tree::sptr tree)
{
    const size_t n_mboards = tree->list("/mboards").size();
    UHD_LOGGER_DEBUG("MPMD") << "Synchronizing " << n_mboards << " timekeepers...";
    auto get_time_last_pps = [tree]() {
        return tree->access<time_spec_t>(fs_path("/mboards/0/time/pps")).get();
    };
    auto end_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(1100);
    auto time_last_pps = get_time_last_pps();
    UHD_LOG_DEBUG("MPMD", "Waiting for PPS clock edge...");
    while (time_last_pps == get_time_last_pps()) {
        if (std::chrono::steady_clock::now() > end_time) {
            throw uhd::runtime_error("Board 0 may not be getting a PPS signal!\n"
                                     "No PPS detected within the time interval.\n"
                                     "See the application notes for your device.\n");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    UHD_LOG_DEBUG("MPMD", "Setting all timekeepers to 0...");
    for (size_t mboard_idx = 0; mboard_idx < n_mboards; mboard_idx++) {
        tree->access<time_spec_t>(fs_path("/mboards") / mboard_idx / "time" / "pps")
            .set(time_spec_t(0.0));
    }

    UHD_LOG_DEBUG("MPMD", "Waiting for next PPS edge...");
    std::this_thread::sleep_for(std::chrono::seconds(1));

    UHD_LOG_DEBUG("MPMD", "Verifying all timekeepers are aligned...");
    auto get_time_now = [tree](const size_t mb_index) {
        return tree->access<time_spec_t>(fs_path("/mboards") / mb_index / "time/now")
            .get();
    };
    for (size_t m = 1; m < n_mboards; m++) {
        time_spec_t time_0 = get_time_now(0);
        time_spec_t time_i = get_time_now(m);
        if (time_i < time_0 or (time_i - time_0) > time_spec_t(MPMD_CHDR_MAX_RTT)) {
            UHD_LOGGER_WARNING("MULTI_USRP")
                << boost::format("Detected time deviation between board %d and board 0.\n"
                                 "Board 0 time is %f seconds.\n"
                                 "Board %d time is %f seconds.\n")
                       % m % time_0.get_real_secs() % m % time_i.get_real_secs();
        }
    }
}

/*! Throw an exception if compat numbers don't match.
 *
 * \param component Name of the component for which we're checking the
 *                  compat number (for logging and exceptions strings).
 * \param expected Tuple of 2 integers representing MAJOR.MINOR compat
 *                 number.
 * \param actual Tuple of 2 integers representing MAJOR.MINOR compat
 *                 number.
 * \param advice_on_failure A string that is appended to the error message
 *                          when compat number mismatches have occurred.
 */
void assert_compat_number_throw(const std::string& component,
    const std::vector<size_t>& expected,
    const std::vector<size_t>& actual,
    const std::string& advice_on_failure = "")
{
    UHD_ASSERT_THROW(expected.size() == 2);
    UHD_ASSERT_THROW(actual.size() == 2);
    UHD_LOGGER_TRACE("MPMD") << "Checking " << component
                             << " compat number. Expected: " << expected[0] << "."
                             << expected[1] << " Actual: " << actual[0] << "."
                             << actual[1];

    if (actual[0] != expected[0]) {
        const std::string err_msg =
            str(boost::format("%s major compat number mismatch. "
                              "Expected: %i.%i Actual: %i.%i.%s%s")
                % component % expected[0] % expected[1] % actual[0] % actual[1]
                % (advice_on_failure.empty() ? "" : " ") % advice_on_failure);
        UHD_LOG_ERROR("MPMD", err_msg);
        throw uhd::runtime_error(err_msg);
    }
    if (actual[1] < expected[1]) {
        const std::string err_msg =
            str(boost::format("%s minor compat number mismatch. "
                              "Expected: %i.%i Actual: %i.%i.%s%s")
                % component % expected[0] % expected[1] % actual[0] % actual[1]
                % (advice_on_failure.empty() ? "" : " ") % advice_on_failure);
        UHD_LOG_ERROR("MPMD", err_msg);
        throw uhd::runtime_error(err_msg);
    }
    if (actual[1] > expected[1]) {
        const std::string err_msg =
            str(boost::format("%s minor compat number mismatch. "
                              "Expected: %i.%i Actual: %i.%i")
                % component % expected[0] % expected[1] % actual[0] % actual[1]);
        UHD_LOG_WARNING("MPMD", err_msg);
    }
}
} // namespace

/*****************************************************************************
 * Static class attributes
 ****************************************************************************/
const std::string mpmd_impl::MPM_FINDALL_KEY            = "find_all";
const size_t mpmd_impl::MPM_DISCOVERY_PORT              = 49600;
const std::string mpmd_impl::MPM_DISCOVERY_PORT_KEY     = "discovery_port";
const size_t mpmd_impl::MPM_RPC_PORT                    = 49601;
const std::string mpmd_impl::MPM_RPC_PORT_KEY           = "rpc_port";
const std::string mpmd_impl::MPM_RPC_GET_LAST_ERROR_CMD = "get_last_error";
const std::string mpmd_impl::MPM_DISCOVERY_CMD          = "MPM-DISC";
const std::string mpmd_impl::MPM_ECHO_CMD               = "MPM-ECHO";

/*****************************************************************************
 * Structors
 ****************************************************************************/
mpmd_impl::mpmd_impl(const device_addr_t& device_args)
    : rfnoc_device(), _device_args(device_args)
{
    const device_addrs_t mb_args_without_prefs = separate_device_addr(device_args);
    device_addrs_t mb_args;
    for (size_t i = 0; i < mb_args_without_prefs.size(); ++i) {
        mb_args.push_back(prefs::get_usrp_args(mb_args_without_prefs[i]));
    }
    const size_t num_mboards = mb_args.size();
    _mb.reserve(num_mboards);
    const bool serialize_init = device_args.has_key("serialize_init");
    const bool skip_init      = device_args.has_key("skip_init");
    UHD_LOGGER_INFO("MPMD") << "Initializing " << num_mboards << " device(s) "
                            << (serialize_init ? "serially " : "in parallel ")
                            << "with args: " << device_args.to_string();

    // First, claim all the devices (so we own them and no one else can claim
    // them).
    // This can be parallelized as long as uptrs are stored in the right spot;
    // they need to be correctly indexed.
    for (size_t mb_i = 0; mb_i < num_mboards; ++mb_i) {
        UHD_LOG_DEBUG("MPMD", "Claiming mboard " << mb_i);
        _mb.push_back(claim_and_make(mb_args[mb_i]));
    }

    if (not skip_init) {
        // Run the actual device initialization. This can run in parallel.
        for (size_t mb_i = 0; mb_i < num_mboards; ++mb_i) {
            // Note: This is the only place we do compat number checks. They're
            // effectively disabled for skip_init=1
            setup_mb(_mb[mb_i].get(), mb_i);
        }
    } else {
        UHD_LOG_DEBUG("MPMD", "Claimed device, but skipped init.");
    }

    // Init the prop tree before the blocks get set up -- they might need access
    // to some of the properties. This also means that the prop tree is pristine
    // at this point in time.
    // This might be parallelized, need to verify the prop tree can handle the
    // concurrent accesses. Would shave of milliseconds per device -- probably
    // not worth it.
    for (size_t mb_i = 0; mb_i < mb_args.size(); ++mb_i) {
        init_property_tree(_tree, fs_path("/mboards") / mb_i, _mb[mb_i].get());
    }

    if (not skip_init) {
        // FIXME this section only makes sense for when the time source is external.
        // So, check for that, or something similar.
        // This section of code assumes that the prop tree is set and we have access
        // to the timekeepers. So don't move it anywhere else.
        if (device_args.has_key("sync_time")) {
            reset_time_synchronized(_tree);
        }
    } else {
        UHD_LOG_INFO("MPMD", "Claimed device without full initialization.");
    }
}

mpmd_impl::~mpmd_impl()
{
    _deinit();
}

/*****************************************************************************
 * Protected methods
 ****************************************************************************/
void mpmd_impl::_deinit()
{
    _tree.reset();
    _mb.clear();
}

/*****************************************************************************
 * Private methods
 ****************************************************************************/
mpmd_mboard_impl::uptr mpmd_impl::claim_and_make(const uhd::device_addr_t& device_args)
{
    const std::string rpc_addr = device_args.get(MGMT_ADDR_KEY);
    UHD_LOGGER_DEBUG("MPMD") << "Device args: `" << device_args.to_string()
                             << "'. RPC address: " << rpc_addr;

    if (rpc_addr.empty()) {
        UHD_LOG_ERROR("MPMD",
            "Could not determine RPC address from device args: "
                << device_args.to_string());
        throw uhd::runtime_error("Could not determine device RPC address.");
    }
    return mpmd_mboard_impl::make(device_args, rpc_addr);
}

void mpmd_impl::setup_mb(mpmd_mboard_impl* mb, const size_t mb_index)
{
    assert_compat_number_throw("MPM",
        MPM_COMPAT_NUM,
        mb->rpc->request<std::vector<size_t>>("get_mpm_compat_num"),
        "Please update the version of MPM on your USRP device.");

    UHD_LOG_DEBUG("MPMD", "Initializing mboard " << mb_index);
    mb->init();
    UHD_ASSERT_THROW(mb->mb_ctrl);
    register_mb_controller(mb_index, mb->mb_ctrl);
}

/*****************************************************************************
 * Factory & Registry
 ****************************************************************************/
static device::sptr mpmd_make(const device_addr_t& device_args)
{
    return device::sptr(std::make_shared<mpmd_impl>(device_args));
}

UHD_STATIC_BLOCK(register_mpmd_device)
{
    device::register_device(&mpmd_find, &mpmd_make, device::USRP);
}
// vim: sw=4 expandtab:
