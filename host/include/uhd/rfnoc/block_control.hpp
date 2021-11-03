//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/rfnoc/noc_block_base.hpp>

namespace uhd { namespace rfnoc {

/*! A default block controller for blocks that can't be found in the registry
 *
 * \ingroup rfnoc_blocks
 */
class UHD_API block_control : public noc_block_base
{
public:
    RFNOC_DECLARE_BLOCK(block_control)
};

}} // namespace uhd::rfnoc
