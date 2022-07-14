//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/features/discoverable_feature.hpp>
#include <memory>

namespace uhd { namespace features {

/*! Interface for running ADC self-calibration on supported devices.
 *  Currently, only the X4xx series of devices supports calibrating the
 *  internal ADCs.
 */
class UHD_API adc_self_calibration_iface : public discoverable_feature
{
public:
    using sptr = std::shared_ptr<adc_self_calibration_iface>;

    static discoverable_feature::feature_id_t get_feature_id()
    {
        return discoverable_feature::ADC_SELF_CALIBRATION;
    }

    std::string get_feature_name() const
    {
        return "ADC Self Calibration";
    }

    virtual ~adc_self_calibration_iface() = default;

    //! Runs calibration on the specified channel. This will momentarily
    //  reconfigure both the specified RX channel as well as the matching
    //  TX channel for the operation.
    //
    //  If you would like to calibrate the ADCs without interrupting the
    //  signal chain, use the rx_codec/<n>/calibration_frozen property on the
    //  motherboard's property tree.
    virtual void run(const size_t chan) = 0;
};

}} // namespace uhd::features
