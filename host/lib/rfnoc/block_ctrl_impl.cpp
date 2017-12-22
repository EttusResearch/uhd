//
// Copyright 2014 Ettus Research LLC
//
// SPDX-License-Identifier: GPL-3.0
//

#include <uhd/rfnoc/block_ctrl.hpp>

using namespace uhd::rfnoc;

class block_ctrl_impl : public block_ctrl
{
public:
    UHD_RFNOC_BLOCK_CONSTRUCTOR(block_ctrl)
    {
        // nop
    }

    // Very empty class, this one
};

UHD_RFNOC_BLOCK_REGISTER(block_ctrl, DEFAULT_BLOCK_NAME);
