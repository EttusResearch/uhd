//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_RFNOC_EXAMPLE_GAIN_BLOCK_CONTROL_HPP
#define INCLUDED_RFNOC_EXAMPLE_GAIN_BLOCK_CONTROL_HPP

#include <uhd/config.hpp>
#include <uhd/rfnoc/noc_block_base.hpp>
#include <uhd/types/stream_cmd.hpp>

namespace rfnoc { namespace example {

/*! Block controller for the gain block: Multiply amplitude of signal
 *
 * This block multiplies the signal input with a fixed gain value.
 */
class UHD_API gain_block_control : public uhd::rfnoc::noc_block_base
{
public:
    RFNOC_DECLARE_BLOCK(gain_block_control)

    //! The register address of the gain value
    static const uint32_t REG_GAIN_VALUE;

    /*! Set the gain value
     */
    virtual void set_gain_value(const uint32_t gain) = 0;

    /*! Get the current gain value (read it from the device)
     */
    virtual uint32_t get_gain_value() = 0;
};

}} // namespace rfnoc::example

#endif /* INCLUDED_RFNOC_EXAMPLE_GAIN_BLOCK_CONTROL_HPP */
