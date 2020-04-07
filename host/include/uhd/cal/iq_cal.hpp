//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/cal/container.hpp>
#include <uhd/config.hpp>
#include <uhd/utils/interpolation.hpp>
#include <complex>
#include <memory>
#include <string>

namespace uhd { namespace usrp { namespace cal {

/*! Class that stores IQ cal data per frequency
 *
 * The following calibrations use this:
 * - Gen-2 and Gen-3 TX DC Offset
 * - Gen-2 and Gen-3 RX,TX IQ Imbalance
 */
class UHD_API iq_cal : public container
{
public:
    using sptr = std::shared_ptr<iq_cal>;

    //! Choose interpolation mode
    //
    // This class supports two kinds of interpolation: Nearest-neighbour, and
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
    virtual std::complex<double> get_cal_coeff(const double freq) const = 0;

    //! Update / set a calbration coefficient
    //
    // This usually only needs to called by calibration utilities.
    //
    // \param freq The frequency at which this coefficient is measured
    // \param coeff The value that is stored
    // \param suppression_abs The amount of impairment suppression this
    //                        coefficient provides, in dB.
    // \param suppression_delta The difference of impairment power between
    //                          applying this coefficient and applying none, in
    //                          dB.
    virtual void set_cal_coeff(const double freq,
        const std::complex<double> coeff,
        const double suppression_abs   = 0,
        const double suppression_delta = 0) = 0;

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
