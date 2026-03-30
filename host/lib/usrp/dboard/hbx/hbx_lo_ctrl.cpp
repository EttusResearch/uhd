//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/usrp/dboard/hbx/hbx_lo_ctrl.hpp>
#include <hbx_cpld_regs.hpp>
#include <thread>

namespace uhd { namespace usrp { namespace hbx {

hbx_lo_ctrl::hbx_lo_ctrl(direction_t trx,
    const std::string& unique_id,
    size_t start_address,
    hbx_cpld_ctrl::poke_fn_type&& poke_fn,
    hbx_cpld_ctrl::peek_fn_type&& peek_fn,
    const double db_prc_rate,
    time_accessor_fn_type&& time_accessor)
    : hbx_cpld_ctrl::spi_transactor(
        start_address, std::move(poke_fn), std::move(peek_fn), true)
    , _time_accessor(std::move(time_accessor))
    , _log_id(unique_id + "::" + (trx == RX_DIRECTION ? "RX_LO" : "TX_LO"))
    , _lmx()
    , _freq(LMX2572_DEFAULT_FREQ)
    , _db_prc_rate(db_prc_rate)
{
    _lmx = lmx2572_iface::make(
        [this](uint8_t addr, uint16_t data) { this->spi_write(addr, data); },
        [this](uint8_t addr) -> uint16_t { return this->spi_read(addr); },
        [](const uhd::time_spec_t& ts) {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(static_cast<int>(ts.get_real_secs() * 1000)));
        },
        unique_id + "::" + (trx == RX_DIRECTION ? "RX" : "TX"));
    UHD_ASSERT_THROW(_lmx);
    UHD_LOG_TRACE(_log_id, "LO initialized...");
    _lmx->reset();

    // In HBX, port A is used if the internal LO shall be used, and port B is used if the
    // LO shall be exported to the front panel connector.
    set_lo_port(lmx2572_iface::output_t::RF_OUTPUT_A, true);
    set_lo_enabled(true);
    // In HBX, sync mode is not used as we are going to solve this by using LO sharing.
    // By setting this to false, `_set_phase_sync()` within the driver will only set the
    // normal operation mode to VCO_PHASE_SYNC_EN (no phase sync mode).
    _lmx->set_sync_mode(false);
    set_lo_ports(false, false);
    set_lo_freq(LMX2572_DEFAULT_FREQ);
    wait_for_lo_lock();
}

double hbx_lo_ctrl::set_lo_freq(const double freq)
{
    UHD_ASSERT_THROW(_lmx);
    UHD_LOG_TRACE(_log_id, "Setting LO frequency " << freq / 1e6 << " MHz");

    _freq = _lmx->set_frequency(freq, _db_prc_rate, false);
    _lmx->set_output_power(lmx2572_iface::output_t::RF_OUTPUT_A, _lo_output_a_power);
    _lmx->set_output_power(lmx2572_iface::output_t::RF_OUTPUT_B, _lo_output_b_power);
    _lmx->commit();
    // only read back the value if timed commands are disabled
    const auto time_spec = _time_accessor();
    if (time_spec == time_spec_t::ASAP) {
        UHD_LOG_TRACE(_log_id,
            "Check LO lock status after setting frequency since timed commands are "
            "disabled.");
        if (get_lo_enabled()) {
            wait_for_lo_lock();
        }
    } else {
        UHD_LOG_TRACE(_log_id,
            "Not checking LO lock status after setting frequency since timed commands "
            "are enabled.");
    }

    return _freq;
}

double hbx_lo_ctrl::get_lo_freq()
{
    return _freq;
}

void hbx_lo_ctrl::wait_for_lo_lock()
{
    UHD_LOG_TRACE(_log_id, "Waiting for LO lock,  " << HBX_LO_LOCK_TIMEOUT_MS << " ms");
    const auto timeout = std::chrono::steady_clock::now()
                         + std::chrono::milliseconds(HBX_LO_LOCK_TIMEOUT_MS);
    while (std::chrono::steady_clock::now() < timeout && !get_lock_status()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    if (!get_lock_status()) {
        // If we can't lock our LO, this could be a lot of possible issues
        throw uhd::runtime_error(_log_id + " has failed to lock!");
    }
}

bool hbx_lo_ctrl::get_lock_status()
{
    return _lmx->get_lock_status();
}

void hbx_lo_ctrl::set_lo_port(lmx2572_iface::output_t port, bool enable)
{
    UHD_LOG_TRACE(_log_id, "Enabling LO output port");

    _lmx->set_output_enable(port, enable);
    _lmx->commit();
}

void hbx_lo_ctrl::set_lo_enabled(bool enable)
{
    UHD_LOG_TRACE(_log_id, "Setting LO enabled state to " << enable);
    _lmx->set_enabled(enable);
}

bool hbx_lo_ctrl::get_lo_enabled()
{
    return _lmx->get_enabled();
}

void hbx_lo_ctrl::set_lo_ports(const bool lo_export, const bool lo_import)
{
    UHD_LOG_TRACE(_log_id, "Setting LO ports");
    _lmx->set_output_enable(lmx2572_iface::output_t::RF_OUTPUT_A, !lo_import);
    _lmx->set_output_enable(lmx2572_iface::output_t::RF_OUTPUT_B, lo_export);
    _lmx->commit();
}

uint8_t hbx_lo_ctrl::set_lo_output_power(uint8_t power)
{
    _lo_output_a_power = power;
    _lo_output_b_power = power;
    UHD_LOG_TRACE(_log_id, "Setting LO output power to " << static_cast<int>(power));
    _lmx->set_output_power(lmx2572_iface::output_t::RF_OUTPUT_A, _lo_output_a_power);
    _lmx->set_output_power(lmx2572_iface::output_t::RF_OUTPUT_B, _lo_output_b_power);
    _lmx->commit();
    return power;
}

uint8_t hbx_lo_ctrl::set_lo_output_a_power(uint8_t power)
{
    _lo_output_a_power = power;
    UHD_LOG_TRACE(_log_id, "Setting LO output A power to " << static_cast<int>(power));
    _lmx->set_output_power(lmx2572_iface::output_t::RF_OUTPUT_A, _lo_output_a_power);
    _lmx->commit();
    return power;
}

uint8_t hbx_lo_ctrl::set_lo_output_b_power(uint8_t power)
{
    _lo_output_b_power = power;
    UHD_LOG_TRACE(_log_id, "Setting LO output B power to " << static_cast<int>(power));
    _lmx->set_output_power(lmx2572_iface::output_t::RF_OUTPUT_B, _lo_output_b_power);
    _lmx->commit();
    return power;
}
}}} // namespace uhd::usrp::hbx
