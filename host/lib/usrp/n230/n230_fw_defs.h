//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_N230_FW_DEFS_H
#define INCLUDED_N230_FW_DEFS_H

#include <stdint.h>

/*!
 * Constants specific to N230 firmware.
 * This header is shared by the firmware and host code.
 * Therefore, this header may only contain valid C code.
 * However, if it is included from within the host code,
 * it will be namespaced appropriately
 */
#ifdef __cplusplus
namespace uhd {
namespace usrp {
namespace n230 {
namespace fw {
#endif

static inline uint32_t reg_addr(uint32_t base, uint32_t offset) {
    return ((base) + (offset)*4);
}

/*******************************************************************
 * Global
 *******************************************************************/
static const uint32_t CPU_CLOCK_FREQ            = 80000000;
static const uint32_t PER_MILLISEC_CRON_JOBID   = 0;
static const uint32_t PER_SECOND_CRON_JOBID     = 1;

/*******************************************************************
 * Wishbone slave addresses
 *******************************************************************/
static const uint32_t WB_MAIN_RAM_BASE  = 0x0000;
static const uint32_t WB_PKT_RAM_BASE   = 0x8000;
static const uint32_t WB_SBRB_BASE      = 0xa000;
static const uint32_t WB_SPI_FLASH_BASE = 0xb000;
static const uint32_t WB_ETH0_MAC_BASE  = 0xc000;
static const uint32_t WB_ETH1_MAC_BASE  = 0xd000;
static const uint32_t WB_XB_SBRB_BASE   = 0xe000;
static const uint32_t WB_ETH0_I2C_BASE  = 0xf600;
static const uint32_t WB_ETH1_I2C_BASE  = 0xf700;
static const uint32_t WB_DBG_UART_BASE  = 0xf900;

/*******************************************************************
 * Seting Register Base addresses
 *******************************************************************/
static const uint32_t SR_ZPU_SW_RST     = 0;
static const uint32_t SR_ZPU_BOOT_DONE  = 1;
static const uint32_t SR_ZPU_LEDS       = 2;
static const uint32_t SR_ZPU_XB_LOCAL   = 4;
static const uint32_t SR_ZPU_SFP_CTRL0  = 16;
static const uint32_t SR_ZPU_SFP_CTRL1  = 17;
static const uint32_t SR_ZPU_ETHINT0    = 64;
static const uint32_t SR_ZPU_ETHINT1    = 80;

static const uint32_t SR_ZPU_SW_RST_NONE    = 0x0;
static const uint32_t SR_ZPU_SW_RST_PHY     = 0x1;
static const uint32_t SR_ZPU_SW_RST_RADIO   = 0x2;

/*******************************************************************
 * Readback addresses
 *******************************************************************/
static const uint32_t RB_ZPU_COMPAT         = 0;
static const uint32_t RB_ZPU_COUNTER        = 1;
static const uint32_t RB_ZPU_SFP_STATUS0    = 2;
static const uint32_t RB_ZPU_SFP_STATUS1    = 3;
static const uint32_t RB_ZPU_ETH0_PKT_CNT   = 6;
static const uint32_t RB_ZPU_ETH1_PKT_CNT   = 7;

/*******************************************************************
 * Ethernet
 *******************************************************************/
static const uint32_t WB_PKT_RAM_CTRL_OFFSET    = 0x1FFC;

static const uint32_t SR_ZPU_ETHINT_FRAMER_BASE     = 0;
static const uint32_t SR_ZPU_ETHINT_DISPATCHER_BASE = 8;

//Eth framer constants
static const uint32_t ETH_FRAMER_SRC_MAC_HI     = 0;
static const uint32_t ETH_FRAMER_SRC_MAC_LO     = 1;
static const uint32_t ETH_FRAMER_SRC_IP_ADDR    = 2;
static const uint32_t ETH_FRAMER_SRC_UDP_PORT   = 3;
static const uint32_t ETH_FRAMER_DST_RAM_ADDR   = 4;
static const uint32_t ETH_FRAMER_DST_IP_ADDR    = 5;
static const uint32_t ETH_FRAMER_DST_UDP_MAC    = 6;
static const uint32_t ETH_FRAMER_DST_MAC_LO     = 7;

/*******************************************************************
 * CODEC
 *******************************************************************/
static const uint32_t CODEC_SPI_CLOCK_FREQ      = 4000000;  //4MHz
static const uint32_t ADF4001_SPI_CLOCK_FREQ    = 200000;   //200kHz

/*******************************************************************
 * UART
 *******************************************************************/
static const uint32_t DBG_UART_BAUD     = 115200;

/*******************************************************************
 * Build Compatability Numbers
 *******************************************************************/
static const uint8_t PRODUCT_NUM = 0x01;
static const uint8_t COMPAT_MAJOR = 0x00;
static const uint16_t COMPAT_MINOR = 0x0000;

static inline uint8_t get_prod_num(uint32_t compat_reg) {
    return (compat_reg >> 24) & 0xFF;
}
static inline uint8_t get_compat_major(uint32_t compat_reg) {
    return (compat_reg >> 16) & 0xFF;
}
static inline uint8_t get_compat_minor(uint32_t compat_reg) {
    return compat_reg & 0xFFFF;
}

#ifdef __cplusplus
}}}} //namespace
#endif
#endif /* INCLUDED_N230_FW_DEFS_H */
