//
// Copyright 2011,2014,2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

namespace uhd { namespace usrp { namespace gpio_atr {

enum gpio_atr_reg_t {
    ATR_REG_IDLE        = int('i'),
    ATR_REG_TX_ONLY     = int('t'),
    ATR_REG_RX_ONLY     = int('r'),
    ATR_REG_FULL_DUPLEX = int('f')
};

}}} // namespace uhd::usrp::gpio_atr
