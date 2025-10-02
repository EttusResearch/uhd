//
// Copyright 2025 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/usrp/cores/complex_gain_3000.hpp>

namespace uhd { namespace cores {

namespace {

uhd::time_spec_t calc_latency(
    const double tick_rate, const uhd::direction_t trx, const size_t nipc)
{
    UHD_ASSERT_THROW(tick_rate > 0.0);
    //! Complex Gain Latency of 6 clock cycles on RX path
    //
    // 4 cycles delay in complex multiplier
    // 1 cycle delay in rounding and 1 cycle delay in clipping
    constexpr uint64_t _feature_cgain_latency = 6;

    if (trx == uhd::direction_t::TX_DIRECTION) {
        const uint64_t core_tx_latency = (nipc > 1) ? 4 : 2;
        return uhd::time_spec_t::from_ticks(nipc * core_tx_latency, tick_rate);

    } else { // RX
        const uint64_t core_rx_latency = (nipc > 1) ? 2 : 0;
        return uhd::time_spec_t::from_ticks(
            (-1.0) * nipc * (_feature_cgain_latency + core_rx_latency), tick_rate);
    }
}

} // namespace

complex_gain_3000::complex_gain_3000(poke32_fn_t&& poke32_fn,
    peek32_fn_t&& peek32_fn,
    const double tick_rate,
    const uhd::direction_t trx,
    const size_t nipc)
    : _trx(trx)
    , _poke32(std::move(poke32_fn))
    , _peek32(peek32_fn)
    , _latency(calc_latency(tick_rate, trx, nipc))
{
}

void complex_gain_3000::set_gain_coeff(const std::complex<double> gain_coeff,
    const size_t chan,
    const std::optional<uhd::time_spec_t>& time)
{
    // Convert to fixed point representation
    const double real_coeff = gain_coeff.real() * _scale_factor;
    const double imag_coeff = gain_coeff.imag() * _scale_factor;

    uhd::time_spec_t timestamp = bool(time) ? time.value() + _latency
                                            : uhd::time_spec_t::ASAP;

    if (real_coeff >= INT16_MAX || real_coeff <= INT16_MIN || imag_coeff >= INT16_MAX
        || imag_coeff <= INT16_MIN) {
        throw uhd::value_error("Complex gain coefficients must be in the range ["
                               + std::to_string(double(INT16_MIN) / _scale_factor) + ", "
                               + std::to_string(double(INT16_MAX) / _scale_factor) + ")");
    }

    _poke32(REG_CGAIN_COEFF,
        uint32_t(
            (int16_t(real_coeff) << CGAIN_COEFF_WIDTH) | (int16_t(imag_coeff) & 0x0FFFF)),
        chan,
        timestamp);
}

std::complex<double> complex_gain_3000::get_gain_coeff(const size_t chan)
{
    const uint32_t gain_coeff = _peek32(REG_CGAIN_COEFF, chan);
    const double real_coeff =
        double(int16_t(gain_coeff >> CGAIN_COEFF_WIDTH)) / _scale_factor;
    const double imag_coeff = double(int16_t(gain_coeff & 0x0FFFF)) / _scale_factor;
    return (std::complex<double>(real_coeff, imag_coeff));
}

}} // namespace uhd::cores
