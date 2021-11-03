//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/rfnoc/noc_block_base.hpp>

namespace uhd { namespace rfnoc {

/*! Add/Sub Block Control Class
 *
 * \ingroup rfnoc_blocks
 *
 * The Add/Sub RFNoC block takes in two streams of sc16-formatted data and
 * performs addition and subtraction on the data in the stream, creating two
 * output streams consisting of the sum and difference of the input streams.
 * The block assumes that the input and output packets are all of the same
 * length.
 *
 */
class UHD_API addsub_block_control : public noc_block_base
{
public:
    RFNOC_DECLARE_BLOCK(addsub_block_control)
};

}} // namespace uhd::rfnoc
