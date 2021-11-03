//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/rfnoc/noc_block_base.hpp>

namespace uhd { namespace rfnoc {

/*! DMA FIFO Block Control Class
 *
 * \ingroup rfnoc_blocks
 */
class UHD_API dmafifo_block_control : public noc_block_base
{
public:
    RFNOC_DECLARE_BLOCK(dmafifo_block_control)
};

}} // namespace uhd::rfnoc
