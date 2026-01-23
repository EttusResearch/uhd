//
// Copyright 2025 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

#pragma once

#include <uhd/types/direction.hpp>
#include <uhd/types/time_spec.hpp>
#include <complex>
#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace uhd { namespace cores {

//! Complex Gain core control
//
// This is a driver for the complex gain core inside the radio_core (used in
// the RFNoC radio block). Note that this feature may not be available, and
// the caller must check its availability first.
//
// The complex gain core enables setting a complex gain value (i.e., changing
// both phase and amplitude) on multiple channels.
class complex_gain_3000
{
public:
    using poke32_fn_t =
        std::function<void(uint32_t, uint32_t, size_t, const uhd::time_spec_t&)>;
    using peek32_fn_t = std::function<uint32_t(uint32_t, size_t)>;

    using uptr = std::unique_ptr<complex_gain_3000>;

    complex_gain_3000(poke32_fn_t&& poke32_fn,
        peek32_fn_t&& peek32_fn,
        const double tick_rate,
        const uhd::direction_t trx,
        const size_t nipc);

    //! Apply the gain coefficient
    //
    // This will convert the floating-point gain gain value to a fixpoint
    // representation. It also accounts for processing delay, and will modify
    // the timestamp such that the new complex gain value is applied at the
    // desired timestamp (\p time).
    void set_gain_coeff(const std::complex<double> gain_coeff,
        const size_t chan,
        const std::optional<uhd::time_spec_t>& time = {});

    //! Return the currently set gain coefficient
    //
    // Convert the fixpoint gain value to a floating point value and return it.
    // Note that this API does not support timed reads.
    std::complex<double> get_gain_coeff(const size_t chan);

private:
    //! Writing to 32-bit coefficient register with
    // [31:16]  : Real part
    // [15:0]   : Imaginary part
    static constexpr uint32_t REG_CGAIN_COEFF       = 0x00;
    static constexpr uint32_t CGAIN_COEFF_WIDTH     = 16;
    static constexpr uint32_t CGAIN_COEFF_FRAC_BITS = 14;
    int _scale_factor                               = (1 << CGAIN_COEFF_FRAC_BITS);

    poke32_fn_t _poke32;
    peek32_fn_t _peek32;

    const uhd::time_spec_t _latency;
};

}} // namespace uhd::cores
