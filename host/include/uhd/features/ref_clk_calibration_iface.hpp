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

/*! Interface to provide access to functions (set, get and store the tuning
 *  word) to perform calibration of the DAC for the internal reference clock
 *  source on supported devices. Currently, only the X4xx series of devices
 *  supports this.
 */
class UHD_API ref_clk_calibration_iface : public discoverable_feature
{
public:
    using sptr = std::shared_ptr<ref_clk_calibration_iface>;

    static discoverable_feature::feature_id_t get_feature_id()
    {
        return discoverable_feature::REF_CLK_CALIBRATION;
    }

    std::string get_feature_name() const
    {
        return "Ref Clk Calibration";
    }

    virtual ~ref_clk_calibration_iface() = default;

    //! Set the tuning word to be configured on the internal reference clock DAC.
    virtual void set_ref_clk_tuning_word(uint32_t tuning_word) = 0;
    //! Returns the tuning word configured on the internal reference clock DAC.
    virtual uint32_t get_ref_clk_tuning_word() = 0;
    //! Writes the reference clock tuning word to the clocking board EEPROM.
    virtual void store_ref_clk_tuning_word(uint32_t tuning_word) = 0;
};

}} // namespace uhd::features
