//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_N230_FPGA_DEFS_H
#define INCLUDED_N230_FPGA_DEFS_H

#include <stdint.h>
#ifndef __cplusplus
#include <stdbool.h>
#endif
#include <uhd/utils/soft_register.hpp>

namespace uhd {
namespace usrp {
namespace n230 {
namespace fpga {

static inline uint32_t sr_addr(uint32_t offset) {
    return (offset*4);
}

static inline uint32_t rb_addr(uint32_t offset) {
    return (offset*8);
}

static const size_t NUM_RADIOS = 2;
static const double BUS_CLK_RATE = 80e6;

/*******************************************************************
 * CVITA Routing
 *******************************************************************/
static const uint32_t CVITA_UDP_PORT    = 49153;
static const bool CVITA_BIG_ENDIAN      = true;

enum xb_endpoint_t {
    N230_XB_DST_E0    = 0,
    N230_XB_DST_E1    = 1,
    N230_XB_DST_R0    = 2,
    N230_XB_DST_R1    = 3,
    N230_XB_DST_GCTRL = 4,
    N230_XB_DST_UART  = 5
};

static const uint8_t RADIO_CTRL_SUFFIX = 0x00;
static const uint8_t RADIO_FC_SUFFIX   = 0x01;
static const uint8_t RADIO_DATA_SUFFIX = 0x02;

/*******************************************************************
 * Seting Register Base addresses
 *******************************************************************/
static const uint32_t SR_CORE_RADIO_CONTROL = 3;
static const uint32_t SR_CORE_LOOPBACK      = 4;
static const uint32_t SR_CORE_BIST1         = 5;
static const uint32_t SR_CORE_BIST2         = 6;
static const uint32_t SR_CORE_SPI           = 8;
static const uint32_t SR_CORE_MISC          = 16;
static const uint32_t SR_CORE_DATA_DELAY    = 17;
static const uint32_t SR_CORE_CLK_DELAY     = 18;
static const uint32_t SR_CORE_COMPAT        = 24;
static const uint32_t SR_CORE_READBACK      = 32;
static const uint32_t SR_CORE_GPSDO_ST      = 40;
static const uint32_t SR_CORE_PPS_SEL       = 48;
static const uint32_t SR_CORE_MS0_GPIO      = 50;
static const uint32_t SR_CORE_MS1_GPIO      = 58;

static const uint32_t RB_CORE_SIGNATUE      = 0;
static const uint32_t RB_CORE_SPI           = 1;
static const uint32_t RB_CORE_STATUS        = 2;
static const uint32_t RB_CORE_BIST          = 3;
static const uint32_t RB_CORE_VERSION_HASH  = 4;
static const uint32_t RB_CORE_MS0_GPIO      = 5;
static const uint32_t RB_CORE_MS1_GPIO      = 6;

/*******************************************************************
 * Seting Register Base addresses
 *******************************************************************/
static const uint32_t SR_RADIO_SPI          = 8;
static const uint32_t SR_RADIO_ATR          = 12;
static const uint32_t SR_RADIO_SW_RST       = 20;
static const uint32_t SR_RADIO_TEST         = 21;
static const uint32_t SR_RADIO_CODEC_IDLE   = 22;
static const uint32_t SR_RADIO_READBACK     = 32;
static const uint32_t SR_RADIO_TX_CTRL      = 64;
static const uint32_t SR_RADIO_RX_CTRL      = 96;
static const uint32_t SR_RADIO_RX_DSP       = 144;
static const uint32_t SR_RADIO_TX_DSP       = 184;
static const uint32_t SR_RADIO_TIME         = 128;
static const uint32_t SR_RADIO_RX_FMT       = 136;
static const uint32_t SR_RADIO_TX_FMT       = 138;
static const uint32_t SR_RADIO_USER_SR      = 253;

static const uint32_t RB_RADIO_TEST         = 0;
static const uint32_t RB_RADIO_TIME_NOW     = 1;
static const uint32_t RB_RADIO_TIME_PPS     = 2;
static const uint32_t RB_RADIO_CODEC_DATA   = 3;
static const uint32_t RB_RADIO_DEBUG        = 4;
static const uint32_t RB_RADIO_FRAMER       = 5;
static const uint32_t SR_RADIO_USER_RB      = 7;

static const uint32_t AD9361_SPI_SLAVE_NUM  = 0x1;
static const uint32_t ADF4001_SPI_SLAVE_NUM = 0x2;

static const uint32_t RB_N230_PRODUCT_ID    = 1;
static const uint32_t RB_N230_COMPAT_MAJOR  = 0x21;
static const uint32_t RB_N230_COMPAT_SAFE   = 0xC0;

/*******************************************************************
 * Codec Interface Specific
 *******************************************************************/

// Matches delay setting of 0x00 in AD9361 register 0x006
static const uint32_t CODEC_DATA_DELAY      = 0;
static const uint32_t CODEC_CLK_DELAY       = 16;

//This number must be < 46.08MHz to make sure we don't
//violate timing for radio_clk. It is only used during
//initialization so the exact value does not matter.
static const double CODEC_DEFAULT_CLK_RATE  = 40e6;

/*******************************************************************
 * Link Specific
 *******************************************************************/
static const double N230_LINK_RATE_BPS      = 1e9/8;

/*******************************************************************
 * GPSDO
 *******************************************************************/
static const uint32_t GPSDO_UART_BAUDRATE   = 115200;
static const uint32_t GPSDO_ST_ABSENT       = 0x83;
/*******************************************************************
 * Register Objects
 *******************************************************************/
class core_radio_ctrl_reg_t : public soft_reg32_wo_t {
public:
    UHD_DEFINE_SOFT_REG_FIELD(MIMO,         /*width*/ 1, /*shift*/ 0);  //[0]
    UHD_DEFINE_SOFT_REG_FIELD(CODEC_ARST,   /*width*/ 1, /*shift*/ 1);  //[1]

    core_radio_ctrl_reg_t():
        soft_reg32_wo_t(fpga::sr_addr(fpga::SR_CORE_RADIO_CONTROL))
    {
        //Initial values
        set(CODEC_ARST, 0);
        set(MIMO, 1);   //MIMO always ON for now
    }
};

class core_misc_reg_t : public soft_reg32_wo_t {
public:
    UHD_DEFINE_SOFT_REG_FIELD(REF_SEL,      /*width*/ 1, /*shift*/ 0);  //[0]
    UHD_DEFINE_SOFT_REG_FIELD(RX_BANDSEL_C, /*width*/ 1, /*shift*/ 1);  //[1]
    UHD_DEFINE_SOFT_REG_FIELD(RX_BANDSEL_B, /*width*/ 1, /*shift*/ 2);  //[2]
    UHD_DEFINE_SOFT_REG_FIELD(RX_BANDSEL_A, /*width*/ 1, /*shift*/ 3);  //[3]
    UHD_DEFINE_SOFT_REG_FIELD(TX_BANDSEL_B, /*width*/ 1, /*shift*/ 4);  //[4]
    UHD_DEFINE_SOFT_REG_FIELD(TX_BANDSEL_A, /*width*/ 1, /*shift*/ 5);  //[5]

    core_misc_reg_t():
        soft_reg32_wo_t(fpga::sr_addr(fpga::SR_CORE_MISC))
    {
        //Initial values
        set(REF_SEL, 0);
        set(RX_BANDSEL_C, 0);
        set(RX_BANDSEL_B, 0);
        set(RX_BANDSEL_A, 0);
        set(TX_BANDSEL_B, 0);
        set(TX_BANDSEL_A, 0);
    }
};

class core_pps_sel_reg_t : public soft_reg32_wo_t {
public:
    UHD_DEFINE_SOFT_REG_FIELD(EXT_PPS_EN,   /*width*/ 1, /*shift*/ 0);  //[0]

    core_pps_sel_reg_t():
        soft_reg32_wo_t(fpga::sr_addr(fpga::SR_CORE_PPS_SEL))
    {
        //Initial values
        set(EXT_PPS_EN, 0);
    }
};

class core_status_reg_t : public soft_reg64_ro_t {
public:
    UHD_DEFINE_SOFT_REG_FIELD(REF_LOCKED,     /*width*/ 1, /*shift*/ 0);    //[0]
    UHD_DEFINE_SOFT_REG_FIELD(GPSDO_STATUS,   /*width*/ 8, /*shift*/ 32);   //[32:39]

    core_status_reg_t():
        soft_reg64_ro_t(fpga::rb_addr(fpga::RB_CORE_STATUS))
    { }
};

}}}}    //namespace

#endif /* INCLUDED_N230_FPGA_DEFS_H */
