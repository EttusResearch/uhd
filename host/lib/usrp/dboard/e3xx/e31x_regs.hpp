//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_E31X_REGS_HPP
#define INCLUDED_E31X_REGS_HPP

#include <uhd/config.hpp>
#include <cstdint>

static const uint32_t VCRX_SW_SHIFT   = 14;
static const uint32_t VCTXRX_SW_SHIFT = 12;
static const uint32_t TX_BIAS_SHIFT   = 10;
static const uint32_t RX_SWC_SHIFT    = 8;
static const uint32_t RX_SWB_SHIFT    = 6;
static const uint32_t RX_SW1_SHIFT    = 3;
static const uint32_t TX_SW1_SHIFT    = 0;

static const uint32_t LED_RX_RX_SHIFT   = 2;
static const uint32_t LED_TXRX_TX_SHIFT = 1;
static const uint32_t LED_TXRX_RX_SHIFT = 0;

#endif /* INCLUDED_E31X_REGS_HPP */
