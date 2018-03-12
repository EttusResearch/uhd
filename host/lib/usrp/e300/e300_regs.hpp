//
// Copyright 2012-2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_E300_REGS_HPP
#define INCLUDED_E300_REGS_HPP

#include <stdint.h>
#include <uhd/config.hpp>

static const uint32_t VCRX_V2     = 15;
static const uint32_t VCRX_V1     = 14;
static const uint32_t VCTXRX_V2   = 13;
static const uint32_t VCTXRX_V1   = 12;
static const uint32_t TX_ENABLEB  = 11;
static const uint32_t TX_ENABLEA  = 10;
static const uint32_t RXC_BANDSEL = 8;
static const uint32_t RXB_BANDSEL = 6;
static const uint32_t RX_BANDSEL  = 3;
static const uint32_t TX_BANDSEL  = 0;

#endif /* INCLUDED_E300_REGS_HPP */
