//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/usrp/dboard/zbx/zbx_lo_ctrl.hpp>
#include <thread>

namespace uhd { namespace usrp { namespace zbx {

zbx_lo_ctrl::zbx_lo_ctrl(zbx_lo_t lo,
    lmx2572_iface::write_fn_t&& poke16,
    lmx2572_iface::read_fn_t&& peek16,
    lmx2572_iface::sleep_fn_t&& sleep,
    const double default_frequency,
    const double db_prc_rate,
    const bool testing_mode_enabled)
    : _log_id(ZBX_LO_LOG_ID.at(lo))
    , _freq(default_frequency)
    , _db_prc_rate(db_prc_rate)
    , _testing_mode_enabled(testing_mode_enabled)
{
    _lmx = lmx2572_iface::make(std::move(poke16), std::move(peek16), std::move(sleep));
    UHD_ASSERT_THROW(_lmx);
    UHD_LOG_TRACE(_log_id, "LO initialized...");
    _lmx->reset();

    set_lo_port_enabled(true);
    // In ZBX, we always run the LOs in sync mode. It is theoretically possible
    // to not do so, but we gain nothing by doing that.
    _lmx->set_sync_mode(true);
    set_lo_freq(LMX2572_DEFAULT_FREQ);
    wait_for_lo_lock();
}

double zbx_lo_ctrl::set_lo_freq(const double freq)
{
    UHD_ASSERT_THROW(_lmx);
    UHD_LOG_TRACE(_log_id, "Setting LO frequency " << freq / 1e6 << " MHz");

    _freq = _lmx->set_frequency(freq,
        _db_prc_rate,
        false /*TODO: get_spur_dodging()*/);
    _lmx->commit();
    return _freq;
}

double zbx_lo_ctrl::get_lo_freq()
{
    return _freq;
}

void zbx_lo_ctrl::wait_for_lo_lock()
{
    UHD_LOG_TRACE(_log_id, "Waiting for LO lock,  " << ZBX_LO_LOCK_TIMEOUT_MS << " ms");
    const auto timeout = std::chrono::steady_clock::now()
                         + std::chrono::milliseconds(ZBX_LO_LOCK_TIMEOUT_MS);
    while (std::chrono::steady_clock::now() < timeout && !get_lock_status()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    if (!get_lock_status()) {
        // If we can't lock our LO, this could be a lot of possible issues
        throw uhd::runtime_error(_log_id + " has failed to lock!");
    }
}

bool zbx_lo_ctrl::get_lock_status()
{
    return _lmx->get_lock_status();
}

void zbx_lo_ctrl::set_lo_port_enabled(bool enable)
{
    UHD_LOG_TRACE(_log_id,
        "Enabling LO " << (_testing_mode_enabled ? "test" : "output") << " port");

    // We want to set the output port regardless of test mode being enabled
    _lmx->set_output_enable(_get_output_port(false), enable);

    if (_testing_mode_enabled && enable) {
        // If testing mode is enabled, also set the test port
        _lmx->set_output_enable(_get_output_port(true), true);
    } else {
        // If testing mode is disabled, test port should be disabled
        _lmx->set_output_enable(_get_output_port(true), false);
        _lmx->set_mux_input(
            _get_output_port(true), lmx2572_iface::mux_in_t::HIGH_IMPEDANCE);
    }

    _lmx->set_enabled(enable);
    _lmx->commit();
}

bool zbx_lo_ctrl::get_lo_port_enabled()
{
    return _lmx->get_enabled();
}

void zbx_lo_ctrl::set_lo_test_mode_enabled(bool enable)
{
    _testing_mode_enabled = enable;
    set_lo_port_enabled(get_lo_port_enabled());
}

bool zbx_lo_ctrl::get_lo_test_mode_enabled()
{
    return _testing_mode_enabled;
}

zbx_lo_t zbx_lo_ctrl::lo_string_to_enum(
    const uhd::direction_t trx, const size_t channel, const std::string name)
{
    if (trx == TX_DIRECTION) {
        if (channel == 0) {
            if (name == ZBX_LO1) {
                return zbx_lo_t::TX0_LO1;
            } else if (name == ZBX_LO2) {
                return zbx_lo_t::TX0_LO2;
            }
        } else if (channel == 1) {
            if (name == ZBX_LO1) {
                return zbx_lo_t::TX1_LO1;
            } else if (name == ZBX_LO2) {
                return zbx_lo_t::TX1_LO2;
            }
        }
    } else {
        if (channel == 0) {
            if (name == ZBX_LO1) {
                return zbx_lo_t::RX0_LO1;
            } else if (name == ZBX_LO2) {
                return zbx_lo_t::RX0_LO2;
            }
        } else if (channel == 1) {
            if (name == ZBX_LO1) {
                return zbx_lo_t::RX1_LO1;
            } else if (name == ZBX_LO2) {
                return zbx_lo_t::RX1_LO2;
            }
        }
    }
    UHD_THROW_INVALID_CODE_PATH();
}

lmx2572_iface::output_t zbx_lo_ctrl::_get_output_port(bool testing_mode)
{
    // Note: The LO output ports here are dependent to the LO and zbx hardware
    // configuration, in no particular order (zbx radio configuration output vs.
    // test port output)
    if (!testing_mode) {
        // Rev B has all LO outputs on Port A
        return lmx2572_iface::output_t::RF_OUTPUT_A;
    } else {
        return lmx2572_iface::output_t::RF_OUTPUT_B;
    }
}

}}} // namespace uhd::usrp::zbx
