//
// Copyright 2010-2012 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_B100_REGS_HPP
#define INCLUDED_B100_REGS_HPP

#include <stdint.h>

#define TOREG(x) ((x)*4)

#define localparam static const int

localparam SR_MISC         = 0;      // 5
localparam SR_USER_REGS    = 5;      // 2
localparam SR_PADDER       = 10;     // 2

localparam SR_TX_CTRL      = 32;     // 6
localparam SR_TX_DSP       = 40;     // 5
localparam SR_TX_FE        = 48;     // 5

localparam SR_RX_CTRL0     = 96;      // 9
localparam SR_RX_DSP0      = 106;     // 7
localparam SR_RX_FE        = 114;     // 5

localparam SR_RX_CTRL1     = 128;     // 9
localparam SR_RX_DSP1      = 138;     // 7

localparam SR_TIME64       = 192;     // 6
localparam SR_SPI          = 208;     // 3
localparam SR_I2C          = 216;     // 1
localparam SR_GPIO         = 224;     // 5

#define REG_RB_TIME_NOW_HI TOREG(10)
#define REG_RB_TIME_NOW_LO TOREG(11)
#define REG_RB_TIME_PPS_HI TOREG(14)
#define REG_RB_TIME_PPS_LO TOREG(15)
#define REG_RB_SPI         TOREG(0)
#define REG_RB_COMPAT      TOREG(1)
#define REG_RB_GPIO        TOREG(3)
#define REG_RB_I2C         TOREG(2)
#define REG_RB_NUM_RX_DSP  TOREG(6)

//spi slave constants
#define B100_SPI_SS_AD9862    (1 << 2)
#define B100_SPI_SS_TX_DB     (1 << 1)
#define B100_SPI_SS_RX_DB     (1 << 0)

#endif /*INCLUDED_B100_REGS_HPP*/

