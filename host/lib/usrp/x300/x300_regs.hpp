//
// Copyright 2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_X300_REGS_HPP
#define INCLUDED_X300_REGS_HPP

#include <uhd/config.hpp>
#include <stdint.h>
#include <uhd/utils/soft_register.hpp>

static const int BL_ADDRESS    = 0;
static const int BL_DATA       = 1;

//wishbone settings map - relevant to host code
#define SET0_BASE     0xa000
#define SETXB_BASE    0xb000
#define BOOT_LDR_BASE 0xfa00
#define I2C0_BASE     0xfe00
#define I2C1_BASE     0xff00
#define SR_ADDR(base, offset) ((base) + (offset)*4)

//I2C1 device addresses
#define MBOARD_EEPROM_ADDR  0x50

static const int ZPU_SR_LEDS       = 00;
static const int ZPU_SR_SW_RST     = 01;
static const int ZPU_SR_CLOCK_CTRL = 02;
static const int ZPU_SR_XB_LOCAL   = 03;
static const int ZPU_SR_SPI        = 32;
static const int ZPU_SR_ETHINT0    = 40;
static const int ZPU_SR_ETHINT1    = 56;
static const int ZPU_SR_DRAM_FIFO0 = 72;
static const int ZPU_SR_DRAM_FIFO1 = 80;

//reset bits
#define ZPU_SR_SW_RST_ETH_PHY           (1<<0)
#define ZPU_SR_SW_RST_RADIO_RST         (1<<1)
#define ZPU_SR_SW_RST_RADIO_CLK_PLL     (1<<2)
#define ZPU_SR_SW_RST_ADC_IDELAYCTRL    (1<<3)

static const int ZPU_RB_SPI        = 2;
static const int ZPU_RB_CLK_STATUS = 3;
static const int ZPU_RB_COMPAT_NUM = 6;
static const int ZPU_RB_NUM_CE     = 7;
static const int ZPU_RB_GIT_HASH   = 10;
static const int ZPU_RB_SFP0_TYPE  = 4;
static const int ZPU_RB_SFP1_TYPE  = 5;

static const uint32_t RB_SFP_1G_ETH  = 0;
static const uint32_t RB_SFP_10G_ETH = 1;
static const uint32_t RB_SFP_AURORA  = 2;

//spi slaves on radio
#define DB_DAC_SEN      (1 << 7)
#define DB_ADC_SEN      (1 << 6)
#define DB_RX_LSADC_SEN (1 << 5)
#define DB_RX_LSDAC_SEN (1 << 4)
#define DB_TX_LSADC_SEN (1 << 3)
#define DB_TX_LSDAC_SEN (1 << 2)
#define DB_RX_SEN       (1 << 1)
#define DB_TX_SEN       (1 << 0)

//-------------------------------------------------------------------
// PCIe Registers
//-------------------------------------------------------------------

static const uint32_t X300_PCIE_VID               = 0x1093;
static const uint32_t X300_PCIE_PID               = 0xC4C4;
//Rev 0-6 motherboard/PCIe IDs (ADC driven at 3.3V)
static const uint32_t X300_USRP_PCIE_SSID_ADC_33         = 0x7736;
static const uint32_t X310_USRP_PCIE_SSID_ADC_33         = 0x76CA;
static const uint32_t X310_2940R_40MHz_PCIE_SSID_ADC_33  = 0x772B;
static const uint32_t X310_2940R_120MHz_PCIE_SSID_ADC_33 = 0x77FB;
static const uint32_t X310_2942R_40MHz_PCIE_SSID_ADC_33  = 0x772C;
static const uint32_t X310_2942R_120MHz_PCIE_SSID_ADC_33 = 0x77FC;
static const uint32_t X310_2943R_40MHz_PCIE_SSID_ADC_33  = 0x772D;
static const uint32_t X310_2943R_120MHz_PCIE_SSID_ADC_33 = 0x77FD;
static const uint32_t X310_2944R_40MHz_PCIE_SSID_ADC_33  = 0x772E;
static const uint32_t X310_2950R_40MHz_PCIE_SSID_ADC_33  = 0x772F;
static const uint32_t X310_2950R_120MHz_PCIE_SSID_ADC_33 = 0x77FE;
static const uint32_t X310_2952R_40MHz_PCIE_SSID_ADC_33  = 0x7730;
static const uint32_t X310_2952R_120MHz_PCIE_SSID_ADC_33 = 0x77FF;
static const uint32_t X310_2953R_40MHz_PCIE_SSID_ADC_33  = 0x7731;
static const uint32_t X310_2953R_120MHz_PCIE_SSID_ADC_33 = 0x7800;
static const uint32_t X310_2954R_40MHz_PCIE_SSID_ADC_33  = 0x7732;
//Rev 7+ motherboard/PCIe IDs (ADCs driven at 1.8V)
static const uint32_t X300_USRP_PCIE_SSID_ADC_18         = 0x7861;
static const uint32_t X310_USRP_PCIE_SSID_ADC_18         = 0x7862;
static const uint32_t X310_2940R_40MHz_PCIE_SSID_ADC_18  = 0x7853;
static const uint32_t X310_2940R_120MHz_PCIE_SSID_ADC_18 = 0x785B;
static const uint32_t X310_2942R_40MHz_PCIE_SSID_ADC_18  = 0x7854;
static const uint32_t X310_2942R_120MHz_PCIE_SSID_ADC_18 = 0x785C;
static const uint32_t X310_2943R_40MHz_PCIE_SSID_ADC_18  = 0x7855;
static const uint32_t X310_2943R_120MHz_PCIE_SSID_ADC_18 = 0x785D;
static const uint32_t X310_2944R_40MHz_PCIE_SSID_ADC_18  = 0x7856;
static const uint32_t X310_2945R_PCIE_SSID_ADC_18        = 0x78EF;
static const uint32_t X310_2950R_40MHz_PCIE_SSID_ADC_18  = 0x7857;
static const uint32_t X310_2950R_120MHz_PCIE_SSID_ADC_18 = 0x785E;
static const uint32_t X310_2952R_40MHz_PCIE_SSID_ADC_18  = 0x7858;
static const uint32_t X310_2952R_120MHz_PCIE_SSID_ADC_18 = 0x785F;
static const uint32_t X310_2953R_40MHz_PCIE_SSID_ADC_18  = 0x7859;
static const uint32_t X310_2953R_120MHz_PCIE_SSID_ADC_18 = 0x7860;
static const uint32_t X310_2954R_40MHz_PCIE_SSID_ADC_18  = 0x785A;
static const uint32_t X310_2955R_PCIE_SSID_ADC_18        = 0x78F0;

static const uint32_t FPGA_X3xx_SIG_VALUE   = 0x58333030;

static const uint32_t PCIE_FPGA_ADDR_BASE   = 0xC0000;
#define PCIE_FPGA_REG(X)                    (PCIE_FPGA_ADDR_BASE + (X))

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

#define PCIE_TX_DMA_REG(REG, CHAN)          (PCIE_TX_DMA_REG_BASE + ((CHAN)*DMA_REG_GRP_SIZE) + (REG))
#define PCIE_RX_DMA_REG(REG, CHAN)          (PCIE_RX_DMA_REG_BASE + ((CHAN)*DMA_REG_GRP_SIZE) + (REG))

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
#define PCIE_ROUTER_REG(X)                  (PCIE_ROUTER_REG_BASE + (X))

static const uint32_t PCIE_ZPU_DATA_BASE    = 0x30000;
static const uint32_t PCIE_ZPU_READ_BASE    = 0x20000;  //Trig and Status share the same base
static const uint32_t PCIE_ZPU_STATUS_BASE  = 0x20000;

#define PCIE_ZPU_DATA_REG(X)                (PCIE_FPGA_REG(PCIE_ZPU_DATA_BASE) + (X))
#define PCIE_ZPU_READ_REG(X)                (PCIE_FPGA_REG(PCIE_ZPU_READ_BASE) + (X))
#define PCIE_ZPU_STATUS_REG(X)              (PCIE_FPGA_REG(PCIE_ZPU_STATUS_BASE) + (X))

static const uint32_t PCIE_ZPU_READ_START       = 0x0;
static const uint32_t PCIE_ZPU_READ_CLOBBER     = 0x80000000;
static const uint32_t PCIE_ZPU_STATUS_BUSY      = 0x1;
static const uint32_t PCIE_ZPU_STATUS_SUSPENDED = 0x80000000;

//-------------------------------------------------------------------
// Register Maps
//-------------------------------------------------------------------
namespace uhd { namespace usrp { namespace x300 {
    class fw_regmap_t : public uhd::soft_regmap_t {
    public:
        typedef boost::shared_ptr<fw_regmap_t> sptr;

        class clk_ctrl_reg_t : public uhd::soft_reg32_wo_t {
        public:
            UHD_DEFINE_SOFT_REG_FIELD(CLK_SOURCE,   /*width*/ 2, /*shift*/ 0);  //[1:0]
            UHD_DEFINE_SOFT_REG_FIELD(PPS_SELECT,   /*width*/ 2, /*shift*/ 2);  //[3:2]
            UHD_DEFINE_SOFT_REG_FIELD(PPS_OUT_EN,   /*width*/ 1, /*shift*/ 4);  //[4]
            UHD_DEFINE_SOFT_REG_FIELD(TCXO_EN,      /*width*/ 1, /*shift*/ 5);  //[5]
            UHD_DEFINE_SOFT_REG_FIELD(GPSDO_PWR_EN, /*width*/ 1, /*shift*/ 6);  //[6]
            UHD_DEFINE_SOFT_REG_FIELD(TIME_SYNC,    /*width*/ 1, /*shift*/ 7);  //[7]

            static const uint32_t SRC_EXTERNAL = 0x0;
            static const uint32_t SRC_INTERNAL = 0x2;
            static const uint32_t SRC_GPSDO    = 0x3;

            clk_ctrl_reg_t(): uhd::soft_reg32_wo_t(SR_ADDR(SET0_BASE, ZPU_SR_CLOCK_CTRL)) {
                //Initial values
                set(CLK_SOURCE, SRC_INTERNAL);
                set(PPS_SELECT, SRC_INTERNAL);
                set(PPS_OUT_EN, 0);
                set(TCXO_EN, 1);
                set(GPSDO_PWR_EN, 1);   //GPSDO power always ON
                set(TIME_SYNC, 0);
            }
        } clock_ctrl_reg;

        class clk_status_reg_t : public uhd::soft_reg32_ro_t {
        public:
            UHD_DEFINE_SOFT_REG_FIELD(LMK_STATUS,       /*width*/ 2, /*shift*/ 0);  //[1:0]
            UHD_DEFINE_SOFT_REG_FIELD(LMK_LOCK,         /*width*/ 1, /*shift*/ 2);  //[2]
            UHD_DEFINE_SOFT_REG_FIELD(LMK_HOLDOVER,     /*width*/ 1, /*shift*/ 3);  //[3]
            UHD_DEFINE_SOFT_REG_FIELD(PPS_DETECT,       /*width*/ 1, /*shift*/ 4);  //[4]
            UHD_DEFINE_SOFT_REG_FIELD(RADIO_CLK_LOCK,   /*width*/ 1, /*shift*/ 5);  //[5]
            UHD_DEFINE_SOFT_REG_FIELD(IDELAYCTRL_LOCK,  /*width*/ 1, /*shift*/ 6);  //[6]

            clk_status_reg_t(): uhd::soft_reg32_ro_t(SR_ADDR(SET0_BASE, ZPU_RB_CLK_STATUS)) {}
        } clock_status_reg;

        fw_regmap_t() : soft_regmap_t("fw_regmap") {
            add_to_map(clock_ctrl_reg, "clock_ctrl_reg", PUBLIC);
            add_to_map(clock_status_reg, "clock_status_reg", PUBLIC);
        }
    };

}}}

#endif /* INCLUDED_X300_REGS_HPP */
