//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhdlib/rfnoc/rf_control/dboard_iface.hpp>
#include <memory>

namespace uhd { namespace usrp { namespace x400 {

/*! Parameters used for ADC self cal on the X400.
 *
 * If the daughterboard supports ADC self-cal, min_gain and max_gain will be the
 * gains used in the gain auto detection algorithm.
 */
struct adc_self_cal_params_t
{
    double min_gain;
    double max_gain;
    double rx_freq;
    double tx_freq;
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
};

}}} // namespace uhd::usrp::x400
