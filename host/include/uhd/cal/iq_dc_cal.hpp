//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/cal/container.hpp>
#include <uhd/config.hpp>
#include <uhd/types/iq_dc_cal_coeffs.hpp>
#include <uhd/utils/interpolation.hpp>
#include <complex>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace uhd { namespace usrp { namespace cal {

/*! Class that stores wideband IQ and DC cal data per frequency
 *
 * The following calibrations use this:
 * - Wideband (multi-tap) IQ imbalance correction
 * - DC offset correction
 *
 * This extends the legacy iq_cal class with support for:
 * - Multi-tap IQ correction coefficients (vectors instead of single values)
 * - DC offset correction
 * - Scaling factor and group delay per frequency
 */
class UHD_API iq_dc_cal : public container
{
public:
    using sptr = std::shared_ptr<iq_dc_cal>;

    //! Choose interpolation mode
    //
    // This class supports two kinds of interpolation: nearest-neighbor, and
    // linear.
    //
    // \param interp The new interpolation mode
    // \throws uhd::value_error if the given interpolation mode is not
    //         supported.
    virtual void set_interp_mode(const uhd::math::interp_mode interp) = 0;

    //! Return a calibration coefficient for a given frequency
    //
    // This function will interpolate to return a valid coefficient for any
    // given frequency.
    virtual iq_dc_cal_coeffs_t get_cal_coeff(const double freq) const = 0;

    //! Return the group delay at a given frequency
    //
    // This function will interpolate to return a valid group delay for any
    // given frequency.
    virtual double get_group_delay(const double freq) = 0;

    //! Update / set a calibration coefficient
    //
    // This usually only needs to be called by calibration utilities.
    //
    // \param freq The frequency at which this coefficient is measured
    // \param scaling_factor The amount by which the raw data should be scaled.
    // \param icross The i cross coefficients
    // \param qinline The q inline coefficients
    // \param delay The group delay of this coefficient, in samples
    // \param dc_offset_real The real part of the DC offset
    // \param dc_offset_imag The imaginary part of the DC offset
    virtual void set_cal_coeff(const double freq,
        const double scaling_factor,
        const std::vector<double> icross,
        const std::vector<double> qinline,
        const double delay          = 0,
        const double dc_offset_real = 0,
        const double dc_offset_imag = 0) = 0;

    //! Clear the list of coefficients
    //
    // This can be useful in order to drop existing cal data, and load an
    // entirely new set with deserialize().
    virtual void clear() = 0;

    //! Factory for new cal data sets
    static sptr make(
        const std::string& name, const std::string& serial, const uint64_t timestamp);

    //! Default factory
    static sptr make();
};

}}} // namespace uhd::usrp::cal
