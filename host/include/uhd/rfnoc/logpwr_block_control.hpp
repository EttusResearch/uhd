//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/rfnoc/noc_block_base.hpp>

namespace uhd { namespace rfnoc {

/*! Log Power Block Control Class
 *
 * \ingroup rfnoc_blocks
 *
 * The Log Power Block is an RFNoC block that takes in a packet of signed
 * 16-bit complex samples and computes an estimate of 1024 * log2(i^2 + q^2),
 * putting the result in the upper 16 bits of each 32-bit output sample.
 */
class UHD_API logpwr_block_control : public noc_block_base
{
public:
    RFNOC_DECLARE_BLOCK(logpwr_block_control)
};

}} // namespace uhd::rfnoc
