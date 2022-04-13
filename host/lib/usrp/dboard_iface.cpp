//
// Copyright 2016 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/usrp/dboard_iface.hpp>
#include <chrono>
#include <thread>

using namespace uhd::usrp;

void dboard_iface::sleep(const std::chrono::nanoseconds& time)
{
    // This sleep function is intended to create a delay on the
    // device.  If a command time is set, just increment that time.
    // If not, the best we can do is just do a sleep on the host.
    // FIXME: Create a delay in the FPGA on the device.
    auto cmd_time = get_command_time();
    if (cmd_time.get_real_secs() != 0.0) {
        set_command_time(cmd_time + uhd::time_spec_t(time.count()));
    } else {
        // nanosleep is not really accurate in userland and it is also not very
        // cross-platform. So just sleep for the minimum amount of time in us.
        if (time < std::chrono::microseconds(1)) {
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        } else {
            std::this_thread::sleep_for(std::chrono::nanoseconds(time.count()));
        }
    }
}
