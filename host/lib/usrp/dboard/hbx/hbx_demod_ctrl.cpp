//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/usrp/dboard/hbx/hbx_demod_ctrl.hpp>

namespace uhd { namespace usrp { namespace hbx {

hbx_demod_ctrl::hbx_demod_ctrl(size_t start_address,
    hbx_cpld_ctrl::poke_fn_type&& poke_fn,
    hbx_cpld_ctrl::peek_fn_type&& peek_fn,
    const std::string& unique_id)
    : hbx_cpld_ctrl::spi_transactor(
        start_address, std::move(poke_fn), std::move(peek_fn), true)
    , _log_id(unique_id + "::HBX_DEMOD")
{
    _ltc = ltc5594_iface::make(
        [this](uint8_t addr, uint8_t data) { this->spi_write(addr, data); },
        [this](uint8_t addr) -> uint8_t { return this->spi_read(addr); },
        unique_id);

    UHD_ASSERT_THROW(_ltc);
    UHD_LOG_TRACE(_log_id, "Demodulator initialized...");
}
void hbx_demod_ctrl::set_lo_matching(const double lo_freq)
{
    UHD_LOG_TRACE(
        _log_id, "Setting LO matching for frequency " << lo_freq / 1e6 << " MHz");
    _ltc->set_lo_matching(lo_freq);
}
}}} // namespace uhd::usrp::hbx
