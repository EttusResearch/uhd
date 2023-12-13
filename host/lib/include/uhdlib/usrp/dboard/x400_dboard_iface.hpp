//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhdlib/rfnoc/rf_control/dboard_iface.hpp>
#include <complex>
#include <memory>
#include <string>

namespace uhd { namespace usrp { namespace x400 {

/*! Parameters used for ADC self cal on the X400.
 *
 * If the daughterboard supports ADC self-cal, min_gain and max_gain will be the
 * gains used in the gain auto detection algorithm.
 */
struct adc_self_cal_params_t
{
    double rx_freq;
    double tx_freq;
    std::complex<int32_t> dac_iq_values;
    uint32_t threshold_delay;
    uint32_t threshold_under;
    uint32_t threshold_over;
    std::string calibration_mode;
    uint32_t calibration_time;
};

/*! Interface for daughterboards which support being plugged into a X400 motherboard.
 */
class x400_dboard_iface : public uhd::rfnoc::rf_control::dboard_iface
{
public:
    using sptr = std::shared_ptr<x400_dboard_iface>;

    //! Returns whether this dboard supports ADC self cal
    virtual bool is_adc_self_cal_supported() = 0;

    //! Returns the parameters required to generate a suitable loopback tone at
    //! tone_freq to perform ADC self cal.
    virtual adc_self_cal_params_t get_adc_self_cal_params(double tone_freq) = 0;

    //! Ask the dboard to search for a gain within the current device settings
    //! that is suitable for ADC self calibration.
    //! Returns true, if a suitable gain was found, false otherwise.
    virtual bool select_adc_self_cal_gain(size_t chan) = 0;

    //! Returns the RFdc converter rate, i.e., the rate at which the converters
    //! are clocked. May be different from the actual sampling rate, if the RFdc
    //! resamplers are enabled.
    virtual double get_converter_rate() const = 0;

    //! Returns the number of rx channels on the daughterboard
    virtual size_t get_num_rx_channels() const = 0;
    //! Returns the number of tx channels on the daughterboard
    virtual size_t get_num_tx_channels() const = 0;
};

}}} // namespace uhd::usrp::x400
