//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_E320_REGS_HPP
#define INCLUDED_E320_REGS_HPP

#include <uhd/config.hpp>
#include <cstdint>

static const uint32_t TX_AMP_SHIFT = 17;
static const uint32_t TRX_SW_SHIFT = 14;
static const uint32_t RX_SW1_SHIFT = 0;
static const uint32_t RX_SW2_SHIFT = 3;
static const uint32_t RX_SW3_SHIFT = 6;
static const uint32_t TX_SW1_SHIFT = 8;
static const uint32_t TX_SW2_SHIFT = 11;

static const uint32_t TRX_LED_GRN_SHIFT = 0;
static const uint32_t TX_LED_RED_SHIFT  = 1;
static const uint32_t RX_LED_GRN_SHIFT  = 2;

#endif /* INCLUDED_E320_REGS_HPP */
