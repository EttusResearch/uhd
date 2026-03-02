//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include "hbx_constants.hpp"
#include <uhd/types/direction.hpp>
#include <uhdlib/usrp/common/lmx2572.hpp>
#include <uhdlib/usrp/dboard/hbx/hbx_cpld_ctrl.hpp>
#include <functional>

namespace uhd { namespace usrp { namespace hbx {

class hbx_lo_ctrl final : public hbx_cpld_ctrl::spi_transactor
{
public:
    using time_accessor_fn_type = std::function<uhd::time_spec_t()>;

    // Pass in our lo selection and poke/peek functions
    hbx_lo_ctrl(direction_t trx,
        size_t start_address,
        hbx_cpld_ctrl::poke_fn_type&& poke_fn,
        hbx_cpld_ctrl::peek_fn_type&& peek_fn,
        const double db_prc_rate,
        time_accessor_fn_type&& time_accessor);

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
    // Additionally, set_lo_enabled() may have to be called if the LO was turned off
    // previously.
    void set_lo_port(lmx2572_iface::output_t port, bool enable);

    // Sets the outputs of the LMX driver according to the import and export settings
    void set_lo_ports(const bool lo_export, const bool lo_import);

    // Completely enables/disables the LO
    void set_lo_enabled(bool enable);

    // Returns status of LO port
    // Targeted LO port depends on whether test mode is disabled/enabled
    bool get_lo_enabled();

    // Sets the LO output power for both ports
    // \param power between 0 and 63, higher is more power.
    uint8_t set_lo_output_power(uint8_t power);

    // Sets only LO output A power
    // \param Power between 0 and 63, higher is more power.
    uint8_t set_lo_output_a_power(uint8_t power);

    // Sets only LO output B power
    // \param Power between 0 and 63, higher is more power.
    uint8_t set_lo_output_b_power(uint8_t power);

private:
    // Accessor for time, similar to hbx_dboard_impl
    time_accessor_fn_type _time_accessor;
    // Returns the appropriate output port for given LO
    lmx2572_iface::output_t _get_output_port(bool test_port);

    // String prefix for log messages
    const std::string _log_id;

    // LMX driver set up with this object specific LO
    lmx2572_iface::sptr _lmx;

    // Cached overall LO output frequency.
    double _freq;

    // Daughterboard PRC rate, used as the reference frequency
    double _db_prc_rate;

    // Save the last set LO output power
    uint8_t _lo_output_a_power = LO_MAX_PWR;
    uint8_t _lo_output_b_power = LO_MAX_PWR;
};
;

}}} // namespace uhd::usrp::hbx
