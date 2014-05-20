//
// Copyright 2013 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef INCLUDED_X300_REGS_HPP
#define INCLUDED_X300_REGS_HPP

#include <stdint.h>
#include <boost/cstdint.hpp>

#define TOREG(x) ((x)*4)

#define localparam static const int

localparam SR_DACSYNC   = 5;
localparam SR_LOOPBACK  = 6;
localparam SR_TEST      = 7;
localparam SR_SPI       = 8;
localparam SR_GPIO      = 16;
localparam SR_MISC_OUTS = 24;
localparam SR_READBACK  = 32;
localparam SR_TX_CTRL   = 64;
localparam SR_RX_CTRL   = 96;
localparam SR_TIME      = 128;
localparam SR_RX_DSP    = 144;
localparam SR_TX_DSP    = 184;
localparam SR_LEDS      = 196;
localparam SR_FP_GPIO   = 200;
localparam SR_RX_FRONT  = 208;
localparam SR_TX_FRONT  = 216;

localparam RB32_GPIO            = 0;
localparam RB32_SPI             = 4;
localparam RB64_TIME_NOW        = 8;
localparam RB64_TIME_PPS        = 16;
localparam RB32_TEST            = 24;
localparam RB32_RX              = 28;
localparam RB32_FP_GPIO         = 32;

localparam BL_ADDRESS     = 0;
localparam BL_DATA        = 1;

//wishbone settings map - relevant to host code
#define SET0_BASE 0xa000
#define SETXB_BASE 0xb000
#define BOOT_LDR_BASE 0xFA00
#define I2C0_BASE 0xfe00
#define I2C1_BASE 0xff00
#define SR_ADDR(base, offset) ((base) + (offset)*4)

localparam ZPU_SR_LEDS       = 00;
localparam ZPU_SR_PHY_RST    = 01;
localparam ZPU_SR_CLOCK_CTRL = 02;
localparam ZPU_SR_XB_LOCAL   = 03;
localparam ZPU_SR_SPI        = 32;
localparam ZPU_SR_ETHINT0    = 40;
localparam ZPU_SR_ETHINT1    = 56;

//clock controls
#define ZPU_SR_CLOCK_CTRL_CLK_SRC_EXTERNAL  0x00
#define ZPU_SR_CLOCK_CTRL_CLK_SRC_INTERNAL  0x02
#define ZPU_SR_CLOCK_CTRL_CLK_SRC_GPSDO     0x03
#define ZPU_SR_CLOCK_CTRL_PPS_SRC_EXTERNAL  0x00
#define ZPU_SR_CLOCK_CTRL_PPS_SRC_INTERNAL  0x02
#define ZPU_SR_CLOCK_CTRL_PPS_SRC_GPSDO     0x03

localparam ZPU_RB_SPI = 2;
localparam ZPU_RB_CLK_STATUS = 3;
localparam ZPU_RB_COMPAT_NUM = 6;
localparam ZPU_RB_ETH_TYPE0  = 4;
localparam ZPU_RB_ETH_TYPE1  = 5;

//clock status
#define ZPU_RB_CLK_STATUS_LMK_STATUS    (0x3 << 0)
#define ZPU_RB_CLK_STATUS_LMK_LOCK      (0x1 << 2)
#define ZPU_RB_CLK_STATUS_LMK_HOLDOVER  (0x1 << 3)
#define ZPU_RB_CLK_STATUS_PPS_DETECT    (0x1 << 4)

//spi slaves on radio
#define DB_DAC_SEN (1 << 7)
#define DB_ADC_SEN (1 << 6)
#define DB_RX_LSADC_SEN (1 << 5)
#define DB_RX_LSDAC_SEN (1 << 4)
#define DB_TX_LSADC_SEN (1 << 3)
#define DB_TX_LSDAC_SEN (1 << 2)
#define DB_RX_SEN (1 << 1)
#define DB_TX_SEN (1 << 0)

//-------------------------------------------------------------------
// PCIe Registers
//-------------------------------------------------------------------

static const uint32_t X300_PCIE_VID         = 0x1093;
static const uint32_t X300_PCIE_PID         = 0xC4C4;
static const uint32_t X300_USRP_PCIE_SSID   = 0x7736;
static const uint32_t X310_USRP_PCIE_SSID   = 0x76CA;
static const uint32_t X310_2940R_PCIE_SSID  = 0x772B;
static const uint32_t X310_2942R_PCIE_SSID  = 0x772C;
static const uint32_t X310_2943R_PCIE_SSID  = 0x772D;
static const uint32_t X310_2944R_PCIE_SSID  = 0x772E;
static const uint32_t X310_2950R_PCIE_SSID  = 0x772F;
static const uint32_t X310_2952R_PCIE_SSID  = 0x7730;
static const uint32_t X310_2953R_PCIE_SSID  = 0x7731;
static const uint32_t X310_2954R_PCIE_SSID  = 0x7732;

static const uint32_t FPGA_X3xx_SIG_VALUE   = 0x58333030;

static const uint32_t PCIE_FPGA_ADDR_BASE   = 0xC0000;
#define PCIE_FPGA_REG(X)                    (PCIE_FPGA_ADDR_BASE + X)

static const uint32_t FPGA_PCIE_SIG_REG     = PCIE_FPGA_REG(0x0000);
static const uint32_t FPGA_CNTR_LO_REG      = PCIE_FPGA_REG(0x0004);
static const uint32_t FPGA_CNTR_HI_REG      = PCIE_FPGA_REG(0x0008);
static const uint32_t FPGA_CNTR_FREQ_REG    = PCIE_FPGA_REG(0x000C);
static const uint32_t FPGA_STATUS_REG       = PCIE_FPGA_REG(0x0020);
static const uint32_t FPGA_USR_SIG_REG_BASE = PCIE_FPGA_REG(0x0030);
static const uint32_t FPGA_USR_SIG_REG_SIZE = 16;

static const uint32_t FPGA_STATUS_DMA_ACTIVE_MASK = 0x3F3F0000;

static const uint32_t PCIE_TX_DMA_REG_BASE  = PCIE_FPGA_REG(0x0200);
static const uint32_t PCIE_RX_DMA_REG_BASE  = PCIE_FPGA_REG(0x0400);

static const uint32_t DMA_REG_GRP_SIZE      = 16;
static const uint32_t DMA_CTRL_STATUS_REG   = 0x0;
static const uint32_t DMA_FRAME_SIZE_REG    = 0x4;
static const uint32_t DMA_SAMPLE_COUNT_REG  = 0x8;
static const uint32_t DMA_PKT_COUNT_REG     = 0xC;

#define PCIE_TX_DMA_REG(REG, CHAN)          (PCIE_TX_DMA_REG_BASE + (CHAN*DMA_REG_GRP_SIZE) + REG)
#define PCIE_RX_DMA_REG(REG, CHAN)          (PCIE_RX_DMA_REG_BASE + (CHAN*DMA_REG_GRP_SIZE) + REG)

static const uint32_t DMA_CTRL_DISABLED     = 0x00000000;
static const uint32_t DMA_CTRL_ENABLED      = 0x00000002;
static const uint32_t DMA_CTRL_CLEAR_STB    = 0x00000001;
static const uint32_t DMA_CTRL_SW_BUF_U64   = (3 << 4);
static const uint32_t DMA_CTRL_SW_BUF_U32   = (2 << 4);
static const uint32_t DMA_CTRL_SW_BUF_U16   = (1 << 4);
static const uint32_t DMA_CTRL_SW_BUF_U8    = (0 << 4);
static const uint32_t DMA_STATUS_ERROR      = 0x00000001;
static const uint32_t DMA_STATUS_BUSY       = 0x00000002;

static const uint32_t PCIE_ROUTER_REG_BASE  = PCIE_FPGA_REG(0x0500);
#define PCIE_ROUTER_REG(X)                  (PCIE_ROUTER_REG_BASE + X)

static const uint32_t PCIE_ZPU_DATA_BASE    = 0x30000;
static const uint32_t PCIE_ZPU_READ_BASE    = 0x20000;  //Trig and Status share the same base
static const uint32_t PCIE_ZPU_STATUS_BASE  = 0x20000;

#define PCIE_ZPU_DATA_REG(X)                (PCIE_FPGA_REG(PCIE_ZPU_DATA_BASE) + X)
#define PCIE_ZPU_READ_REG(X)                (PCIE_FPGA_REG(PCIE_ZPU_READ_BASE) + X)
#define PCIE_ZPU_STATUS_REG(X)              (PCIE_FPGA_REG(PCIE_ZPU_STATUS_BASE) + X)

static const uint32_t PCIE_ZPU_READ_START       = 0x0;
static const uint32_t PCIE_ZPU_READ_CLOBBER     = 0x80000000;
static const uint32_t PCIE_ZPU_STATUS_BUSY      = 0x1;
static const uint32_t PCIE_ZPU_STATUS_SUSPENDED = 0x80000000;


#endif /* INCLUDED_X300_REGS_HPP */
