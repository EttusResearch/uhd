//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include "zbx_constants.hpp"
#include <uhd/types/direction.hpp>
#include <uhdlib/usrp/common/lmx2572.hpp>
#include <functional>

namespace uhd { namespace usrp { namespace zbx {

class zbx_lo_ctrl final
{
public:
    // Pass in our lo selection and poke/peek functions
    zbx_lo_ctrl(zbx_lo_t lo,
        lmx2572_iface::write_fn_t&& poke16,
        lmx2572_iface::read_fn_t&& peek16,
        lmx2572_iface::sleep_fn_t&& sleep,
        const double default_frequency,
        const double db_prc_rate,
        const bool testing_mode_enabled);

    // Passes in a desired LO frequency to the LMX driver, returns the coerced frequency
    double set_lo_freq(const double freq);

    // Returns cached LO frequency value
    double get_lo_freq();

    // Spins up a timeout loop to wait for the PLL's to lock
    // \throws uhd::runtime_error on a failure to lock
    void wait_for_lo_lock();

    // Returns the lock status of the PLL
    bool get_lock_status();

    // Enable/disable LO port
    // Targeted LO port depends on whether test mode is disabled/enabled
    void set_lo_port_enabled(bool enable);

    // Returns status of LO port
    // Targeted LO port depends on whether test mode is disabled/enabled
    bool get_lo_port_enabled();

    // Enable test mode of the LO
    void set_lo_test_mode_enabled(bool enable);

    // Returns whether the test mode has been enabled
    bool get_lo_test_mode_enabled();

    static zbx_lo_t lo_string_to_enum(
        const uhd::direction_t trx, const size_t channel, const std::string name);

    // TODO: Future implementation of spur dodging
    // void set_spur_dodging(const bool enable);
    // bool get_spur_dodging();
private:
    // Returns the appropriate output port for given LO
    lmx2572_iface::output_t _get_output_port(bool test_port);

    // String prefix for log messages
    const std::string _log_id;

    // LMX driver set up with this object specific LO
    lmx2572_iface::sptr _lmx;

    // Cached overall LO output frequency.
    // TODO: seperate between coerced/desired frequencies for recalculation once LO step
    // quantization is introduced
    double _freq;

    // Daughterboard PRC rate, used as the reference frequency
    double _db_prc_rate;

    // Set LO output mode, RF output mode is considered normal use case
    // Testing mode is for LMX V&V
    bool _testing_mode_enabled;
};

}}} // namespace uhd::usrp::zbx
