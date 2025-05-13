//
// Copyright 2023 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "x400_internal_sync.hpp"
#include <uhd/utils/log.hpp>

namespace uhd { namespace features {

internal_sync::internal_sync(uhd::usrp::internal_sync_actor::sptr db_ctrl)
    : _db_ctrl(db_ctrl)
{
}

void internal_sync::enable_sync_clk()
{
    _db_ctrl->set_internal_sync_clk(true);
}

void internal_sync::disable_sync_clk()
{
    _db_ctrl->set_internal_sync_clk(false);
}

}} // namespace uhd::features
