//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "x300_defaults.hpp"
#include "x300_mb_controller.hpp"
#include <uhd/property_tree.hpp>
#include <uhd/utils/math.hpp>

using namespace uhd;
using namespace uhd::rfnoc;

namespace uhd { namespace usrp { namespace x300 {

void init_prop_tree(const size_t mb_idx, x300_mb_controller* mbc, property_tree::sptr pt)
{
    const fs_path mb_path = fs_path("/mboards") / mb_idx;
    try {
        pt->create<std::string>("/name").set("X-Series Device");
    } catch (const uhd::runtime_error&) {
        // property_tree lacks an atomic check to only create a new node if it
        // doesn't exist, so we simply try and create it and when it fails, we
        // assume that another device has already created this node and we move
        // on. If we did "if exists" before creating, there's a non-zero chance
        // that a concurrent device init would still throw.
    }
    pt->create<std::string>(mb_path / "name").set(mbc->get_mboard_name());
    pt->create<std::string>(mb_path / "codename").set("Yetti");

    ////////////////////////////////////////////////////////////////////
    // create clock properties
    ////////////////////////////////////////////////////////////////////
    pt->create<double>(mb_path / "master_clock_rate").set_publisher([mbc]() {
        return mbc->get_clock_ctrl()->get_master_clock_rate();
    });

    ////////////////////////////////////////////////////////////////////
    // setup time sources and properties
    ////////////////////////////////////////////////////////////////////
    pt->create<std::string>(mb_path / "time_source" / "value")
        .set(mbc->get_time_source())
        .add_coerced_subscriber(
            [mbc](const std::string& time_source) { mbc->set_time_source(time_source); });
    pt->create<std::vector<std::string>>(mb_path / "time_source" / "options")
        .set(mbc->get_time_sources());

    // setup the time output, default to ON
    pt->create<bool>(mb_path / "time_source" / "output")
        .add_coerced_subscriber(
            [mbc](const bool time_output) { mbc->set_time_source_out(time_output); })
        .set(true);

    ////////////////////////////////////////////////////////////////////
    // setup clock sources and properties
    ////////////////////////////////////////////////////////////////////
    pt->create<std::string>(mb_path / "clock_source" / "value")
        .set(mbc->get_clock_source())
        .add_coerced_subscriber([mbc](const std::string& clock_source) {
            mbc->set_clock_source(clock_source);
        })
        .set_publisher([mbc]() { return mbc->get_clock_source(); });
    pt->create<std::vector<std::string>>(mb_path / "clock_source" / "options")
        .set(mbc->get_clock_sources());

    // setup external reference options. default to 10 MHz input reference
    pt->create<std::string>(mb_path / "clock_source" / "external");
    pt->create<std::vector<double>>(
          mb_path / "clock_source" / "external" / "freq" / "options")
        .set(EXTERNAL_FREQ_OPTIONS);
    pt->create<double>(mb_path / "clock_source" / "external" / "value")
        .set(mbc->get_clock_ctrl()->get_sysref_clock_rate())
        .set_coercer([current_rate = mbc->get_clock_ctrl()->get_sysref_clock_rate()](
                         const double clock_rate) {
            if (!uhd::math::frequencies_are_equal(clock_rate, current_rate)) {
                UHD_LOG_WARNING(
                    "X300", "Cannot change the sysref clock rate at runtime!");
            }
            return clock_rate;
        });

    // setup the clock output, default to ON
    pt->create<bool>(mb_path / "clock_source" / "output")
        .add_coerced_subscriber(
            [mbc](const bool clock_output) { mbc->set_clock_source_out(clock_output); });

    // Initialize tick rate (must be done before setting time)
    // Note: The master tick rate can't be changed at runtime!
    const double master_clock_rate = mbc->get_clock_ctrl()->get_master_clock_rate();
    pt->create<double>(mb_path / "tick_rate")
        .set_coercer([master_clock_rate](const double rate) {
            // The contract of multi_usrp::set_master_clock_rate() is to coerce
            // and not throw, so we'll follow that behaviour here.
            if (!uhd::math::frequencies_are_equal(rate, master_clock_rate)) {
                UHD_LOGGER_WARNING("X300")
                    << "Cannot update master clock rate! X300 Series does not "
                       "allow changing the clock rate during runtime.";
            }
            return master_clock_rate;
        })
        .set(master_clock_rate);

    ////////////////////////////////////////////////////////////////////
    // and do the misc mboard sensors
    ////////////////////////////////////////////////////////////////////
    for (const std::string& sensor_name : mbc->get_sensor_names()) {
        pt->create<sensor_value_t>(mb_path / "sensors" / sensor_name)
            .set_publisher([mbc, sensor_name]() { return mbc->get_sensor(sensor_name); });
    }
}

}}} // namespace uhd::usrp::x300
