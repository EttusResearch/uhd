//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/rfnoc/noc_block_base.hpp>
#include <uhd/types/stream_cmd.hpp>
#include <rfnoc/gain/config.hpp>


namespace rfnoc { namespace gain {

/*! Block controller for the gain block: Multiply amplitude of signal
 *
 * This block multiplies the signal input with a fixed gain value.
 *
 * Note that this block implements an integer multiplication, i.e., the output
 * of this block is y = g * x, where g is the gain value and x is the input
 * signal, and all values are integers.
 *
 * The gain value g can be set by calling the set_gain_value() method, or by
 * setting the "gain" property.
 */
class RFNOC_GAIN_API gain_block_control : public uhd::rfnoc::noc_block_base
{
public:
    RFNOC_DECLARE_BLOCK(gain_block_control)

    static const std::string PROP_KEY_GAIN;

    //! The register address of the gain value
    static const uint32_t REG_GAIN_VALUE;

    /*! Set the gain value
     */
    virtual void set_gain_value(const uint32_t gain) = 0;

    /*! Get the current gain value (read it from the device)
     */
    virtual uint32_t get_gain_value() = 0;
};

}} // namespace rfnoc::gain
