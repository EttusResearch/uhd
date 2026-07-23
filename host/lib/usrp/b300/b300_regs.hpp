//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/utils/soft_register.hpp>
#include <cstdint>

static const std::string B300_DEFAULT_CLOCK_SOURCE     = "internal";
static const std::string B300_DEFAULT_TIME_SOURCE      = "internal";
static constexpr double B300_DEFAULT_MASTER_CLOCK_RATE = 122.88e6;
static constexpr double B300_DEFAULT_SYSTEM_REF_RATE   = 10e6;
static constexpr double B300_BUS_CLOCK_RATE            = 125e6;
static constexpr uint32_t B300_TB_I2C_DATA_RATE        = 100e3;
static constexpr size_t B300_BAR0_MAP_SIZE             = 0x100000; // 1 MB

// Generic B310 PID
static const uint32_t B310_PID = 0x7B8D;
// B310 PID specific to the PCIe variant
static const uint32_t B310_PCIE_PID = 0x7B9A;
// B310 PID specific to the Thundbolt variant
static const uint32_t B310_TB_PID = 0x7B9B;

// PCIe Registers (DMA-related only)
// This is the absolute offset from the base of BAR0.
static constexpr uint32_t PCIE_FPGA_ADDR_BASE = 0x10000;
constexpr uint32_t PCIE_FPGA_REG(uint32_t X)
{
    return PCIE_FPGA_ADDR_BASE + X;
}

static constexpr uint32_t PCIE_TX_DMA_REG_BASE = PCIE_FPGA_REG(0x0200);
static constexpr uint32_t PCIE_RX_DMA_REG_BASE = PCIE_FPGA_REG(0x0400);

static constexpr uint32_t DMA_REG_GRP_SIZE     = 16;
static constexpr uint32_t DMA_CTRL_STATUS_REG  = 0x0;
static constexpr uint32_t DMA_FRAME_SIZE_REG   = 0x4;
static constexpr uint32_t DMA_SAMPLE_COUNT_REG = 0x8;
static constexpr uint32_t DMA_PKT_COUNT_REG    = 0xC;

constexpr uint32_t PCIE_TX_DMA_REG(uint32_t REG, uint32_t CHAN)
{
    return PCIE_TX_DMA_REG_BASE + (CHAN * DMA_REG_GRP_SIZE) + REG;
}
constexpr uint32_t PCIE_RX_DMA_REG(uint32_t REG, uint32_t CHAN)
{
    return PCIE_RX_DMA_REG_BASE + (CHAN * DMA_REG_GRP_SIZE) + REG;
}

static constexpr uint32_t DMA_CTRL_DISABLED   = 0x00000000;
static constexpr uint32_t DMA_CTRL_ENABLED    = 0x00000002;
static constexpr uint32_t DMA_CTRL_CLEAR_STB  = 0x00000001;
static constexpr uint32_t DMA_CTRL_SW_BUF_U64 = 0b110000;
static constexpr uint32_t DMA_CTRL_SW_BUF_U32 = 0b100000;
static constexpr uint32_t DMA_STATUS_ERROR    = 0x00000001;
static constexpr uint32_t DMA_STATUS_BUSY     = 0x00000002;

// Number of FIFO offset between RX and TX channels
static constexpr uint32_t B300_TX_FIFO_OFFSET = 5;

// HBTIDR is exposed as a BAR0-relative offset.
static constexpr uint32_t B300_HBTIDR_REG = 0x00A0;

// -----------------------------------------------------------------------------
// UHD_PciRegs register offsets (from FixedLogicRegMap.rbm)
// -----------------------------------------------------------------------------
static constexpr uint32_t PCIE_PCI_SIGNATURE_REG =
    PCIE_FPGA_REG(0x0000); // PciSignatureRegister
static constexpr uint32_t PCIE_FPGA_COUNTER_LO_REG = PCIE_FPGA_REG(0x0004);
static constexpr uint32_t PCIE_FPGA_COUNTER_HI_REG = PCIE_FPGA_REG(0x0008);
static constexpr uint32_t PCIE_FPGA_FREQ_REG       = PCIE_FPGA_REG(0x000C);
static constexpr uint32_t PCIE_SCRATCH_LO_REG      = PCIE_FPGA_REG(0x0010);
static constexpr uint32_t PCIE_SCRATCH_HI_REG      = PCIE_FPGA_REG(0x0014);
static constexpr uint32_t PCIE_MISC_STATUS_REG     = PCIE_FPGA_REG(0x0020);
static constexpr uint32_t PCIE_USR_SIG0_REG        = PCIE_FPGA_REG(0x0030);
static constexpr uint32_t PCIE_USR_SIG1_REG        = PCIE_FPGA_REG(0x0034);
static constexpr uint32_t PCIE_USR_SIG2_REG        = PCIE_FPGA_REG(0x0038);
static constexpr uint32_t PCIE_USR_SIG3_REG        = PCIE_FPGA_REG(0x003C);

static constexpr uint32_t FLASH_OFFSET = 0x30000;
constexpr uint32_t FLASH_REG(uint32_t x)
{
    return FLASH_OFFSET + x;
}

static constexpr uint32_t FLASH_SCRATCH_OFFSET      = FLASH_REG(0x100);
static constexpr uint32_t FLASH_OUTPUT_OFFSET       = FLASH_REG(0x104);
static constexpr uint32_t FLASH_READ_DATA_OFFSET    = FLASH_REG(0x10C);
static constexpr uint32_t FLASH_STATUS_OFFSET       = FLASH_REG(0x110);
static constexpr uint32_t FLASH_FAST_COMMAND_OFFSET = FLASH_REG(0x11C);
static constexpr uint32_t FLASH_FAST_WRITE_OFFSET   = FLASH_REG(0x120);
static constexpr uint32_t FLASH_FAST_READ_OFFSET    = FLASH_REG(0x124);
static constexpr uint32_t FLASH_FIFO_SIZE_OFFSET    = FLASH_REG(0x128);
static constexpr uint32_t FLASH_FIFO_STATUS_OFFSET  = FLASH_REG(0x12C);

// This is the absolute offset from the base of BAR0.
static constexpr uint32_t BAR0_BASIC_ADDR_BASE = 0x20000;
constexpr uint32_t BAR0_BASIC_REG(uint32_t x)
{
    return BAR0_BASIC_ADDR_BASE + x;
}
static constexpr uint32_t BAR0_CORE_ADDR_BASE = 0x20020;
constexpr uint32_t BAR0_CORE_REG(uint32_t x)
{
    return BAR0_CORE_ADDR_BASE + x;
}

static constexpr uint32_t CORE_SIGNATURE_REG        = BAR0_BASIC_REG(0x0000);
static constexpr uint32_t CORE_REVISION_REG         = BAR0_BASIC_REG(0x0004);
static constexpr uint32_t CORE_OLDEST_REVISION_REG  = BAR0_BASIC_REG(0x0008);
static constexpr uint32_t VERSION_LAST_MODIFIED_REG = BAR0_BASIC_REG(0x000C);
static constexpr uint32_t CORE_SCRATCH_REG          = BAR0_BASIC_REG(0x0010);
static constexpr uint32_t GIT_HASH_REG              = BAR0_BASIC_REG(0x0014);

static constexpr uint32_t FPGA_DEVICE_ID_REG  = BAR0_CORE_REG(0x0000);
static constexpr uint32_t SW_RESETS_REG       = BAR0_CORE_REG(0x0004);
static constexpr uint32_t CLOCK_CTRL_REG      = BAR0_CORE_REG(0x0008);
static constexpr uint32_t CLOCK_STATUS_REG    = BAR0_CORE_REG(0x000C);
static constexpr uint32_t DEVICE_DNA_LOW_REG  = BAR0_CORE_REG(0x0010);
static constexpr uint32_t DEVICE_DNA_HIGH_REG = BAR0_CORE_REG(0x0014);
static constexpr uint32_t COMPAT_NUM_REG      = BAR0_CORE_REG(0x0018);
static constexpr uint32_t RFNOC_INFO_REG      = BAR0_CORE_REG(0x001C);
static constexpr uint32_t INT_PPS_DIVIDER_REG = BAR0_CORE_REG(0x0028);
static constexpr uint32_t NUM_TIMEKEEPERS_REG = BAR0_CORE_REG(0x002C);
static constexpr uint32_t BUILD_SEED_REG      = BAR0_CORE_REG(0x0030);
static constexpr uint32_t FP_GPIO_SRC         = BAR0_CORE_REG(0x0034);
static constexpr uint32_t GPS_CTRL_REG        = BAR0_CORE_REG(0x0038);
static constexpr uint32_t LMK_SYNC_CTRL_REG   = BAR0_CORE_REG(0x0040);
static constexpr uint32_t PPS_IN_CTRL_REG     = BAR0_CORE_REG(0x0044);


// Motherboard I2C interfaces in BAR0
static constexpr uint32_t BAR0_MB_I2C_ADDR_BASE = 0x20070;
constexpr uint32_t BAR0_MB_I2C_REG(uint32_t x)
{
    return BAR0_MB_I2C_ADDR_BASE + x;
}

static constexpr uint32_t MB_I2C_PRER_LO = BAR0_MB_I2C_REG(0x00);
static constexpr uint32_t MB_I2C_PRER_HI = BAR0_MB_I2C_REG(0x04);
static constexpr uint32_t MB_I2C_CTR     = BAR0_MB_I2C_REG(0x08);
static constexpr uint32_t MB_I2C_TXR     = BAR0_MB_I2C_REG(0x0C);
static constexpr uint32_t MB_I2C_RXR     = BAR0_MB_I2C_REG(0x10);
static constexpr uint32_t MB_I2C_CR      = BAR0_MB_I2C_REG(0x14);
static constexpr uint32_t MB_I2C_SR      = BAR0_MB_I2C_REG(0x18);

#define I2C_PWR_MONITOR 0x40
#define I2C_TMP_SENSOR  0x4A

// Thunderbolt I2C interfaces in BAR0
static constexpr uint32_t BAR0_TB_I2C_ADDR_BASE = 0x200C0;
constexpr uint32_t BAR0_TB_I2C_REG(uint32_t x)
{
    return BAR0_TB_I2C_ADDR_BASE + x;
}

static constexpr uint32_t TB_I2C_PRER_LO = BAR0_TB_I2C_REG(0x00);
static constexpr uint32_t TB_I2C_PRER_HI = BAR0_TB_I2C_REG(0x04);
static constexpr uint32_t TB_I2C_CTR     = BAR0_TB_I2C_REG(0x08);
static constexpr uint32_t TB_I2C_TXR     = BAR0_TB_I2C_REG(0x0C);
static constexpr uint32_t TB_I2C_RXR     = BAR0_TB_I2C_REG(0x10);
static constexpr uint32_t TB_I2C_CR      = BAR0_TB_I2C_REG(0x14);
static constexpr uint32_t TB_I2C_SR      = BAR0_TB_I2C_REG(0x18);

#define I2C_EEPROM 0x50

static constexpr uint32_t BAR0_SR_SPI = 0x200A0;
static constexpr uint32_t BAR0_RB_SPI = 0x200B0;

// This is the absolute offset from the base of BAR0 for the timekeeper
// registers
static constexpr uint32_t TK_ADDR_BASE = 0x20100;
constexpr uint32_t TK_REG(uint32_t x)
{
    return TK_ADDR_BASE + x;
}
static constexpr uint32_t TK_REG_TICKS_NOW_LO    = TK_REG(0x00); // Read-only
static constexpr uint32_t TK_REG_TICKS_NOW_HI    = TK_REG(0x04); // Read-only
static constexpr uint32_t TK_REG_TICKS_EVENT_LO  = TK_REG(0x08); // Write-only
static constexpr uint32_t TK_REG_TICKS_EVENT_HI  = TK_REG(0x0C); // Write-only
static constexpr uint32_t TK_REG_TICKS_CTRL      = TK_REG(0x10); // Write-only
static constexpr uint32_t TK_REG_TICKS_PPS_LO    = TK_REG(0x14); // Read-only
static constexpr uint32_t TK_REG_TICKS_PPS_HI    = TK_REG(0x18); // Read-only
static constexpr uint32_t TK_REG_TICKS_PERIOD_LO = TK_REG(0x1C); // Read-Write
static constexpr uint32_t TK_REG_TICKS_PERIOD_HI = TK_REG(0x20); // Read-Write

static constexpr uint32_t GPS_UART_BASE = 0x20130;

// SPI slaves on BAR0
#define BAR0_LMK05318_SEN (1 << 2)
#define BAR0_DAC_TCXO_SEN (1 << 1)
#define BAR0_LMK04832_SEN (1 << 0)

namespace uhd { namespace usrp { namespace b300 {
class bar0_regmap_t : public uhd::soft_regmap_t
{
public:
    using sptr = std::shared_ptr<bar0_regmap_t>;

    class sw_resets_reg_t : public uhd::soft_reg32_wo_t
    {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(RADIO_CLK_GEN_RST, /*width*/ 1, /*shift*/ 0); //[0]

        sw_resets_reg_t() : uhd::soft_reg32_wo_t(SW_RESETS_REG)
        {
            // Initial values
            set(RADIO_CLK_GEN_RST, 0);
        }
    } sw_resets_reg;

    class clk_ctrl_reg_t : public uhd::soft_reg32_wo_t
    {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(REF_CLK_SRC, /*width*/ 1, /*shift*/ 0); //[0]
        UHD_DEFINE_SOFT_REG_FIELD(LMK_SRC_SEL, /*width*/ 1, /*shift*/ 4); //[4]
        UHD_DEFINE_SOFT_REG_FIELD(TCXO_EN, /*width*/ 1, /*shift*/ 8); //[8]
        UHD_DEFINE_SOFT_REG_FIELD(LMK04832_RST, /*width*/ 1, /*shift*/ 12); //[12]
        UHD_DEFINE_SOFT_REG_FIELD(LMK05318_PD_N, /*width*/ 1, /*shift*/ 13); //[13]
        UHD_DEFINE_SOFT_REG_FIELD(PPS_SRC, /*width*/ 1, /*shift*/ 16); //[16]

        static constexpr uint32_t SRC_GPSDO    = 0x0;
        static constexpr uint32_t SRC_INTERNAL = 0x1;
        static constexpr uint32_t SRC_122M     = 0x0;
        static constexpr uint32_t SRC_125M     = 0x1;
        static constexpr uint32_t PPS_SRC_INT  = 0x0;
        static constexpr uint32_t PPS_SRC_EXT  = 0x1;

        clk_ctrl_reg_t() : uhd::soft_reg32_wo_t(CLOCK_CTRL_REG)
        {
            // Initial values
            set(REF_CLK_SRC, SRC_INTERNAL);
            set(LMK_SRC_SEL, SRC_122M);
            set(TCXO_EN, 0);
            set(LMK04832_RST, 0);
            set(LMK05318_PD_N, 0);
            set(PPS_SRC, PPS_SRC_INT);
        }
    } clock_ctrl_reg;

    class int_pps_divider_reg_t : public uhd::soft_reg32_rw_t
    {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(INT_PPS_DIV, 32, 0); //[31:0]
        int_pps_divider_reg_t() : uhd::soft_reg32_rw_t(INT_PPS_DIVIDER_REG)
        {
            set(INT_PPS_DIV, 122880000); // Default to 10MHz from 122.88MHz
        }
    } int_pps_divider_reg;

    class gpio_ctrl_reg_t : public uhd::soft_reg32_rw_t
    {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(FP_GPIO0_SRC, /*width*/ 2, /*shift*/ 0); //[0:1]
        UHD_DEFINE_SOFT_REG_FIELD(FP_GPIO1_SRC, /*width*/ 2, /*shift*/ 2); //[3:2]
        UHD_DEFINE_SOFT_REG_FIELD(FP_GPIO2_SRC, /*width*/ 2, /*shift*/ 4); //[5:4]
        UHD_DEFINE_SOFT_REG_FIELD(FP_GPIO3_SRC, /*width*/ 2, /*shift*/ 6); //[7:6]
        UHD_DEFINE_SOFT_REG_FIELD(FP_GPIO4_SRC, /*width*/ 2, /*shift*/ 8); //[9:8]
        UHD_DEFINE_SOFT_REG_FIELD(FP_GPIO5_SRC, /*width*/ 2, /*shift*/ 10); //[11:10]
        UHD_DEFINE_SOFT_REG_FIELD(FP_GPIO6_SRC, /*width*/ 2, /*shift*/ 12); //[13:12]
        UHD_DEFINE_SOFT_REG_FIELD(FP_GPIO7_SRC, /*width*/ 2, /*shift*/ 14); //[15:14]
        UHD_DEFINE_SOFT_REG_FIELD(FP_GPIO8_SRC, /*width*/ 2, /*shift*/ 16); //[17:16]
        UHD_DEFINE_SOFT_REG_FIELD(FP_GPIO9_SRC, /*width*/ 2, /*shift*/ 18); //[19:18]

        static constexpr uint32_t SRC_CH0 = 0x0;
        static constexpr uint32_t SRC_CH1 = 0x1;

        gpio_ctrl_reg_t() : uhd::soft_reg32_rw_t(FP_GPIO_SRC)
        {
            // Initial values
            set(FP_GPIO0_SRC, SRC_CH0);
            set(FP_GPIO1_SRC, SRC_CH0);
            set(FP_GPIO2_SRC, SRC_CH0);
            set(FP_GPIO3_SRC, SRC_CH0);
            set(FP_GPIO4_SRC, SRC_CH0);
            set(FP_GPIO5_SRC, SRC_CH0);
            set(FP_GPIO6_SRC, SRC_CH0);
            set(FP_GPIO7_SRC, SRC_CH0);
            set(FP_GPIO8_SRC, SRC_CH0);
            set(FP_GPIO9_SRC, SRC_CH0);
        }
    } gpio_ctrl_reg;

    class gps_ctrl_reg_t : public uhd::soft_reg32_rw_t
    {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(GPS_RESET_N, /*width*/ 1, /*shift*/ 0); //[0]
        UHD_DEFINE_SOFT_REG_FIELD(GPS_ANT_PWR_EN, /*width*/ 1, /*shift*/ 1); //[1]
        UHD_DEFINE_SOFT_REG_FIELD(GPS_PWR_FAULT, /*width*/ 1, /*shift*/ 4); //[4]
        UHD_DEFINE_SOFT_REG_FIELD(GPS_PPS_MONITOR, /*width*/ 1, /*shift*/ 8); //[8]

        gps_ctrl_reg_t() : uhd::soft_reg32_rw_t(GPS_CTRL_REG)
        {
            // Initial values
            set(GPS_RESET_N, 0);
            set(GPS_ANT_PWR_EN, 0);
            set(GPS_PWR_FAULT, 0);
            set(GPS_PPS_MONITOR, 0);
        }
    } gps_ctrl_reg;

    class hbtidr_reg_t : public uhd::soft_reg32_ro_t
    {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(HBNEGWDTH, /*width*/ 8, /*shift*/ 0); //[7:0]
        UHD_DEFINE_SOFT_REG_FIELD(HBMAXWDTH, /*width*/ 8, /*shift*/ 8); //[15:8]
        UHD_DEFINE_SOFT_REG_FIELD(HBNEGSPD, /*width*/ 4, /*shift*/ 16); //[19:16]
        UHD_DEFINE_SOFT_REG_FIELD(HBMAXSPD, /*width*/ 4, /*shift*/ 20); //[23:20]
        UHD_DEFINE_SOFT_REG_FIELD(HBTYPE, /*width*/ 8, /*shift*/ 24); //[31:24]

        hbtidr_reg_t() : uhd::soft_reg32_ro_t(B300_HBTIDR_REG) {}
    } hbtidr_reg;

    class lmk_sync_ctrl_reg_t : public uhd::soft_reg32_rw_t
    {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(LMK_SYNC_DELAY, /*width*/ 27, /*shift*/ 0); //[26:0]
        UHD_DEFINE_SOFT_REG_FIELD(LMK_SYNC_TRIGGER, /*width*/ 1, /*shift*/ 28); //[28]
        UHD_DEFINE_SOFT_REG_FIELD(LMK_SYNC_DONE, /*width*/ 1, /*shift*/ 29); //[29]
        UHD_DEFINE_SOFT_REG_FIELD(LMK_SYNC_CLK_SEL, /*width*/ 1, /*shift*/ 30); //[30]
        UHD_DEFINE_SOFT_REG_FIELD(LMK_CLKIN0_SYNC_SEL, /*width*/ 1, /*shift*/ 31); //[31]

        static constexpr uint32_t LMK_SYNC_RADIO_CLK = 0x0;
        static constexpr uint32_t LMK_SYNC_REF_CLK   = 0x1;
        static constexpr uint32_t SYNC_PIN_SYNC      = 0x0;
        static constexpr uint32_t SYNC_PIN_CLKIN0    = 0x1;

        lmk_sync_ctrl_reg_t() : uhd::soft_reg32_rw_t(LMK_SYNC_CTRL_REG)
        {
            // Initial values
            set(LMK_SYNC_DELAY, 0);
            set(LMK_SYNC_TRIGGER, 0);
            set(LMK_SYNC_DONE, 0);
            set(LMK_SYNC_CLK_SEL, LMK_SYNC_RADIO_CLK);
            set(LMK_CLKIN0_SYNC_SEL, SYNC_PIN_SYNC);
        }
    } lmk_sync_ctrl_reg;

    class pps_in_ctrl_reg_t : public uhd::soft_reg32_rw_t
    {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(
            PPS_IN_TO_RCLK_DELAY, /*width*/ 10, /*shift*/ 0); //[9:0]

        pps_in_ctrl_reg_t() : uhd::soft_reg32_rw_t(PPS_IN_CTRL_REG)
        {
            // Initial values
            set(PPS_IN_TO_RCLK_DELAY, 0);
        }
    } pps_in_ctrl_reg;

    class num_timekeepers_reg_t : public uhd::soft_reg32_ro_t
    {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(NUM_TIMEKEEPERS, /*width*/ 32, /*shift*/ 0); //[31:0]

        num_timekeepers_reg_t() : uhd::soft_reg32_ro_t(NUM_TIMEKEEPERS_REG) {}
    } num_timekeepers_reg;

    // TK_ timekeeper registers
    class tk_ticks_now_lo_reg_t : public uhd::soft_reg32_ro_t
    {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(TICKS_NOW_LO, 32, 0); //[31:0]
        tk_ticks_now_lo_reg_t() : uhd::soft_reg32_ro_t(TK_REG_TICKS_NOW_LO) {}
    } tk_ticks_now_lo_reg;

    class tk_ticks_now_hi_reg_t : public uhd::soft_reg32_ro_t
    {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(TICKS_NOW_HI, 32, 0); //[31:0]
        tk_ticks_now_hi_reg_t() : uhd::soft_reg32_ro_t(TK_REG_TICKS_NOW_HI) {}
    } tk_ticks_now_hi_reg;

    class tk_ticks_event_lo_reg_t : public uhd::soft_reg32_wo_t
    {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(TICKS_EVENT_LO, 32, 0); //[31:0]
        tk_ticks_event_lo_reg_t() : uhd::soft_reg32_wo_t(TK_REG_TICKS_EVENT_LO) {}
    } tk_ticks_event_lo_reg;

    class tk_ticks_event_hi_reg_t : public uhd::soft_reg32_wo_t
    {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(TICKS_EVENT_HI, 32, 0); //[31:0]
        tk_ticks_event_hi_reg_t() : uhd::soft_reg32_wo_t(TK_REG_TICKS_EVENT_HI) {}
    } tk_ticks_event_hi_reg;

    class tk_ticks_ctrl_reg_t : public uhd::soft_reg32_wo_t
    {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(TIME_NOW, 1, 0); //[0]
        UHD_DEFINE_SOFT_REG_FIELD(TIME_PPS, 1, 1); //[1]
        tk_ticks_ctrl_reg_t() : uhd::soft_reg32_wo_t(TK_REG_TICKS_CTRL) {}
    } tk_ticks_ctrl_reg;

    class tk_ticks_pps_lo_reg_t : public uhd::soft_reg32_ro_t
    {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(TICKS_PPS_LO, 32, 0); //[31:0]
        tk_ticks_pps_lo_reg_t() : uhd::soft_reg32_ro_t(TK_REG_TICKS_PPS_LO) {}
    } tk_ticks_pps_lo_reg;

    class tk_ticks_pps_hi_reg_t : public uhd::soft_reg32_ro_t
    {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(TICKS_PPS_HI, 32, 0); //[31:0]
        tk_ticks_pps_hi_reg_t() : uhd::soft_reg32_ro_t(TK_REG_TICKS_PPS_HI) {}
    } tk_ticks_pps_hi_reg;

    class tk_ticks_period_lo_reg_t : public uhd::soft_reg32_rw_t
    {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(TICKS_PERIOD_LO, 32, 0); //[31:0]
        tk_ticks_period_lo_reg_t() : uhd::soft_reg32_rw_t(TK_REG_TICKS_PERIOD_LO) {}
    } tk_ticks_period_lo_reg;

    class tk_ticks_period_hi_reg_t : public uhd::soft_reg32_rw_t
    {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(TICKS_PERIOD_HI, 32, 0); //[31:0]
        tk_ticks_period_hi_reg_t() : uhd::soft_reg32_rw_t(TK_REG_TICKS_PERIOD_HI) {}
    } tk_ticks_period_hi_reg;

    bar0_regmap_t() : soft_regmap_t("bar0_regmap")
    {
        add_to_map(sw_resets_reg, "sw_resets_reg", PUBLIC);
        add_to_map(clock_ctrl_reg, "clock_ctrl_reg", PUBLIC);
        add_to_map(gpio_ctrl_reg, "gpio_ctrl_reg", PUBLIC);
        add_to_map(gps_ctrl_reg, "gps_ctrl_reg", PUBLIC);
        add_to_map(hbtidr_reg, "hbtidr_reg", PUBLIC);
        add_to_map(int_pps_divider_reg, "int_pps_divider_reg", PUBLIC);
        add_to_map(lmk_sync_ctrl_reg, "lmk_sync_ctrl_reg", PUBLIC);
        add_to_map(pps_in_ctrl_reg, "pps_in_ctrl_reg", PUBLIC);
        add_to_map(num_timekeepers_reg, "num_timekeepers_reg", PUBLIC);
        add_to_map(tk_ticks_now_lo_reg, "tk_ticks_now_lo_reg", PUBLIC);
        add_to_map(tk_ticks_now_hi_reg, "tk_ticks_now_hi_reg", PUBLIC);
        add_to_map(tk_ticks_event_lo_reg, "tk_ticks_event_lo_reg", PUBLIC);
        add_to_map(tk_ticks_event_hi_reg, "tk_ticks_event_hi_reg", PUBLIC);
        add_to_map(tk_ticks_ctrl_reg, "tk_ticks_ctrl_reg", PUBLIC);
        add_to_map(tk_ticks_pps_lo_reg, "tk_ticks_pps_lo_reg", PUBLIC);
        add_to_map(tk_ticks_pps_hi_reg, "tk_ticks_pps_hi_reg", PUBLIC);
        add_to_map(tk_ticks_period_lo_reg, "tk_ticks_period_lo_reg", PUBLIC);
        add_to_map(tk_ticks_period_hi_reg, "tk_ticks_period_hi_reg", PUBLIC);
    }
};

}}} // namespace uhd::usrp::b300
