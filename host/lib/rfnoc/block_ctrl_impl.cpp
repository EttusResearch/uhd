//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
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
