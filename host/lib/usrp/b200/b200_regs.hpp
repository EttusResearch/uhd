//
// Copyright 2012-2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_B200_REGS_HPP
#define INCLUDED_B200_REGS_HPP

#include <stdint.h>

#define TOREG(x) ((x)*4)

#define localparam static const int

localparam SR_CORE_SPI       = 8;
localparam SR_CORE_MISC      = 16;
localparam SR_CORE_COMPAT    = 24;
localparam SR_CORE_GPSDO_ST  = 40;
localparam SR_CORE_SYNC      = 48;
localparam RB32_CORE_SPI     = 8;
localparam RB32_CORE_MISC    = 16;
localparam RB32_CORE_STATUS  = 20;
localparam RB32_CORE_PLL     = 24;

localparam SR_SPI       = 8;
localparam SR_ATR       = 12;
localparam SR_TEST      = 21;
localparam SR_CODEC_IDLE = 22;
localparam SR_READBACK  = 32;
localparam SR_TX_CTRL   = 64;
localparam SR_RX_CTRL   = 96;
localparam SR_RX_DSP    = 144;
localparam SR_TX_DSP    = 184;
localparam SR_TIME      = 128;
localparam SR_RX_FMT    = 136;
localparam SR_TX_FMT    = 138;
localparam SR_FP_GPIO   = 200;

localparam RB32_TEST            = 0;
localparam RB64_TIME_NOW        = 8;
localparam RB64_TIME_PPS        = 16;
localparam RB64_CODEC_READBACK  = 24;
localparam RB32_FP_GPIO         = 32;

//pll constants
static const int AD9361_SLAVENO = (1 << 0);
static const int ADF4001_SLAVENO = (1 << 1);
static const double AD9361_SPI_RATE = 1e6;
static const double ADF4001_SPI_RATE = 10e3; //slow for large time constant on spi lines

/* ATR Control Bits */
static const uint32_t TX_ENABLE1 = (1 << 7);
static const uint32_t SFDX1_RX = (1 << 6);
static const uint32_t SFDX1_TX = (1 << 5);
static const uint32_t SRX1_RX = (1 << 4);
static const uint32_t SRX1_TX = (1 << 3);
static const uint32_t LED_RX1 = (1 << 2);
static const uint32_t LED_TXRX_RX1 = (1 << 1);
static const uint32_t LED_TXRX_TX1 = (1 << 0);

static const uint32_t TX_ENABLE2 = (1 << 7);
static const uint32_t SFDX2_RX = (1 << 6);
static const uint32_t SFDX2_TX = (1 << 5);
static const uint32_t SRX2_RX = (1 << 4);
static const uint32_t SRX2_TX = (1 << 3);
static const uint32_t LED_RX2 = (1 << 2);
static const uint32_t LED_TXRX_RX2 = (1 << 1);
static const uint32_t LED_TXRX_TX2 = (1 << 0);


/* ATR State Definitions. */
static const uint32_t STATE_OFF = 0x00;

///////////////////////// side 1 ///////////////////////////////////
static const uint32_t STATE_RX1_RX2 = (SFDX1_RX
                                                | SFDX1_TX
                                                | LED_RX1);

static const uint32_t STATE_RX1_TXRX = (SRX1_RX
                                                | SRX1_TX
                                                | LED_TXRX_RX1);

static const uint32_t STATE_FDX1_TXRX = (TX_ENABLE1
                                                | SFDX1_RX
                                                | SFDX1_TX
                                                | LED_TXRX_TX1
                                                | LED_RX1);

static const uint32_t STATE_TX1_TXRX = (TX_ENABLE1
                                                | SFDX1_RX
                                                | SFDX1_TX
                                                | LED_TXRX_TX1);

///////////////////////// side 2 ///////////////////////////////////
static const uint32_t STATE_RX2_RX2 = (SFDX2_RX
                                                | SRX2_TX
                                                | LED_RX2);

static const uint32_t STATE_RX2_TXRX = (SRX2_TX
                                                | SRX2_RX
                                                | LED_TXRX_RX2);

static const uint32_t STATE_FDX2_TXRX = (TX_ENABLE2
                                                | SFDX2_RX
                                                | SFDX2_TX
                                                | LED_TXRX_TX2
                                                | LED_RX2);

static const uint32_t STATE_TX2_TXRX = (TX_ENABLE2
                                                | SFDX2_RX
                                                | SFDX2_TX
                                                | LED_TXRX_TX2);


#endif /* INCLUDED_B200_REGS_HPP */
