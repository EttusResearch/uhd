//
// Copyright 2025 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "b300_clock_ctrl.hpp"
#include "b300_regs.hpp"
#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/math.hpp>
#include <uhdlib/usrp/common/lmk04832.hpp>
#include <thread>

namespace uhd { namespace usrp { namespace b300 {

class b300_clock_ctrl_impl : public b300_clock_ctrl
{
public:
    b300_clock_ctrl_impl(bar0_regmap_t::sptr bar0_regmap,
        uhd::spi_iface::sptr spi,
        double ext_clock_rate,
        uint16_t board_rev)
        : _bar0_regmap(bar0_regmap)
        , _master_clock_rate(0.0)
        , _ext_clk_rate(ext_clock_rate)
        , _output_divider(20)
        , _pll1_n_div(3072)
        , _pll2_n_div(10)
        , _pll2_n_cal_div(10)
        , _pll2_prescaler(2)
        , _clkin0_r_div(1000)
        , _clkin1_r_div(250)
        , _clkin2_r_div(500)
        , _sysref_div(640)
        , _distributed_clocks(false)
        , _10M_ext_ref(false)
        , _board_rev(board_rev)
    {
        // Validate that the external clock rate is one of the allowable options
        bool valid_rate = false;
        for (const auto& valid_freq : EXTERNAL_FREQ_OPTIONS) {
            if (uhd::math::frequencies_are_equal(ext_clock_rate, valid_freq)) {
                valid_rate = true;
                break;
            }
        }
        if (!valid_rate) {
            throw uhd::value_error(
                "Invalid external clock rate: " + std::to_string(ext_clock_rate)
                + " Hz. Valid rates are: 10 MHz, 122.88 MHz, 125 MHz");
        }

        if (_board_rev == 1) {
            // Setup the DAC for tuning the internal reference clock.
            // Power up DAC Internal Reference Clock.
            spi->write_spi(BAR0_DAC_TCXO_SEN, spi_config_t::EDGE_FALL, 0x380001, 24);
            // Power Down VOUTB.
            spi->write_spi(BAR0_DAC_TCXO_SEN, spi_config_t::EDGE_FALL, 0x200022, 24);
            // Disable LDAC pin control.
            spi->write_spi(BAR0_DAC_TCXO_SEN, spi_config_t::EDGE_FALL, 0x300001, 24);
            // Write and Update DAC A to 0xA90 to set DAC output voltage to 1.65 to tune
            // TCXO to 40MHz.
            spi->write_spi(BAR0_DAC_TCXO_SEN, spi_config_t::EDGE_FALL, 0x18A900, 24);
            _clkin_sel = CLKin0;
        } else {
            _clkin_sel = CLKin2;
        }

        // Create LMK04832 interface
        _lmk04832 = lmk04832_iface::make(
            [spi](uint16_t addr, uint8_t value) {
                // Create a 24-bit value: R/W=0 (Write), next 15 bits=addr, 8 LSB=value
                uint32_t val_to_write = (0u << 23) | ((addr & 0x7FFF) << 8) | value;
                spi->write_spi(
                    BAR0_LMK04832_SEN, spi_config_t::EDGE_RISE, val_to_write, 24);
            },
            [spi](uint16_t addr) {
                // Create a 24-bit value: R/W=1 (Read), next 15 bits=addr
                uint32_t val_to_write = (1u << 23) | ((addr & 0x7FFF) << 8);
                return spi->read_spi(
                    BAR0_LMK04832_SEN, spi_config_t::EDGE_RISE, val_to_write, 24);
            },
            "B310");

        _lmk05318 = lmk05318_iface::make(
            [spi](uint16_t addr, uint8_t value) {
                // Create a 24-bit value: R/W=0 (Write), next 15 bits=addr, 8 LSB=value
                uint32_t val_to_write = (0u << 23) | ((addr & 0x7FFF) << 8) | value;
                spi->write_spi(
                    BAR0_LMK05318_SEN, spi_config_t::EDGE_RISE, val_to_write, 24);
            },
            [spi](uint16_t addr) {
                // Create a 24-bit value: R/W=1 (Read), next 15 bits=addr
                uint32_t val_to_write = (1u << 23) | ((addr & 0x7FFF) << 8);
                return spi->read_spi(
                    BAR0_LMK05318_SEN, spi_config_t::EDGE_RISE, val_to_write, 24);
            });
    }

    void init() override
    {
        // Enable the 12MHz Oscillator
        _bar0_regmap->clock_ctrl_reg.write(bar0_regmap_t::clk_ctrl_reg_t::TCXO_EN, 1);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        // Power up the LMK05318
        _bar0_regmap->clock_ctrl_reg.write(
            bar0_regmap_t::clk_ctrl_reg_t::LMK05318_PD_N, 1);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        reset_lmk04832(false, true);
        reset_lmk04832(true);
        reset_lmk04832(false);
        _lmk05318->soft_reset(true);
        _lmk05318->soft_reset(false);
        if (!_lmk04832->verify_chip_id()) {
            throw uhd::runtime_error("unable to locate LMK04832!");
        }
        if (!_lmk05318->verify_chip_id()) {
            throw uhd::runtime_error("unable to locate LMK05318!");
        }
        if (_board_rev != 1) {
            config_lmk05318();
        }
    }

    void reset_lmk04832(bool value, bool hard = false) override
    {
        if (hard) {
            _bar0_regmap->clock_ctrl_reg.write(
                bar0_regmap_t::clk_ctrl_reg_t::LMK04832_RST, value);
        } else {
            _lmk04832->soft_reset(value);
        }

        if (!value) {
            // Enable 4-wire spi readback after a reset. 4-wire SPI is disabled by default
            // after a reset of the LMK, but is required to perform SPI reads on the b300.
            _lmk04832->enable_4wire_spi();
        }
    }

    void set_vcxo(vcxo_sel_t vcxo_sel) override
    {
        if (vcxo_sel == VCXO_122p88MHz) {
            _bar0_regmap->clock_ctrl_reg.write(bar0_regmap_t::clk_ctrl_reg_t::LMK_SRC_SEL,
                bar0_regmap_t::clk_ctrl_reg_t::SRC_122M);
        } else if (vcxo_sel == VCXO_125MHz) {
            _bar0_regmap->clock_ctrl_reg.write(bar0_regmap_t::clk_ctrl_reg_t::LMK_SRC_SEL,
                bar0_regmap_t::clk_ctrl_reg_t::SRC_125M);
        } else {
            throw uhd::runtime_error("Invalid VCXO selection!");
        }
    }

    void config_lmk04832(double master_clock_rate) override
    {
        if (uhd::math::frequencies_are_equal(master_clock_rate, 122.88e6)) {
            _vcxo_freq    = VCXO_122p88MHz;
            _pll1_n_div   = _10M_ext_ref ? 96 : 3072;
            _clkin0_r_div = 1000;
            _clkin1_r_div = 250;
        } else if (uhd::math::frequencies_are_equal(master_clock_rate, 125e6)) {
            _vcxo_freq    = VCXO_125MHz;
            _pll1_n_div   = _10M_ext_ref ? 100 : 3125;
            _clkin0_r_div = _10M_ext_ref ? 1024 : 1000;
            _clkin1_r_div = _10M_ext_ref ? 256 : 250;
        } else {
            throw uhd::runtime_error("Invalid master clock rate requested: "
                                     + std::to_string(master_clock_rate)
                                     + " valid rates are 122.88 MHz or 125 MHz");
        }
        _master_clock_rate = master_clock_rate;

        set_vcxo(_vcxo_freq);

        // Clear hard reset and trigger soft reset.
        reset_lmk04832(false, true);
        reset_lmk04832(true, false);
        reset_lmk04832(false, false);

        uint32_t prescaler = _lmk04832->pll2_pre_to_reg(_pll2_prescaler);

        // Calculate delays
        int clk_ddly = _get_clk_digital_delay(10, _output_divider);
        // CLKout8 needs a different digital delay, but only for non-distributed mode.
        int clkout8_ddly = _get_clk_digital_delay(9, _output_divider);
        // Sysref delay is derived from the formula in the LMK04832 datasheet
        // chapter 8.3.5. Take the constant SYSREF SCLK digital delay from each clock
        // pair. 8 used as the miniumum value for SYSREF clock digital delay (see LMK Data
        // Sheet Section 8.6.2.2.7).
        // 20 is chosen to allow the global + local sysref delay (both min 8) to sum up to
        // this value (including small adjustments based on the clock divider).
        int sysref_delay = _get_clk_digital_delay(26) - 1 - 8;

        // Program output dividers to 4 first (see datasheet, Table 18)
        _lmk04832->pokes8({
            {0x0100, 0x04},
            {0x0108, 0x04},
            {0x0130, 0x04},
        });

        if (_distributed_clocks) {
            _program_distributed_clkout(clk_ddly);
        } else {
            _program_non_distributed_clkout(clk_ddly, clkout8_ddly);
        }
        _prog_plls_enabled(sysref_delay, prescaler);
    }

    void set_lmk04832_clock_in(clkin_sel_t clkin_sel) override
    {
        if (_board_rev == 1) {
            if (clkin_sel != CLKin0 && clkin_sel != CLKin1) {
                throw uhd::value_error(
                    "Invalid CLKin selection: " + std::to_string(clkin_sel)
                    + ". Valid options are: CLKin0 (0) or CLKin1 (1).");
            }
        } else {
            if (clkin_sel != CLKin2 && clkin_sel != CLKin1) {
                throw uhd::value_error(
                    "Invalid CLKin selection: " + std::to_string(clkin_sel)
                    + ". Valid options are: CLKin2 (2) or CLKin1 (1).");
            }
        }

        // If the external clock being use is 122.88MHz or 125MHz on CLKin1, we need to
        // distribute the clocks.
        _clkin_sel = clkin_sel;
        if (_clkin_sel == CLKin1
            && (uhd::math::frequencies_are_equal(_ext_clk_rate, 122.88e6)
                or uhd::math::frequencies_are_equal(_ext_clk_rate, 125e6))) {
            _10M_ext_ref        = false;
            _distributed_clocks = true;
            _output_divider     = 1;
            _sysref_div         = 32;
            if (_board_rev == 1) {
                _bar0_regmap->lmk_sync_ctrl_reg.write(
                    bar0_regmap_t::lmk_sync_ctrl_reg_t::LMK_CLKIN0_SYNC_SEL,
                    bar0_regmap_t::lmk_sync_ctrl_reg_t::SYNC_PIN_SYNC);
            } else {
                _bar0_regmap->lmk_sync_ctrl_reg.write(
                    bar0_regmap_t::lmk_sync_ctrl_reg_t::LMK_CLKIN0_SYNC_SEL,
                    bar0_regmap_t::lmk_sync_ctrl_reg_t::SYNC_PIN_CLKIN0);
            }
        } else {
            if (_clkin_sel == CLKin1) {
                _10M_ext_ref = true;
            }
            _distributed_clocks = false;
            _output_divider     = 20;
            _sysref_div         = 640;
            _bar0_regmap->lmk_sync_ctrl_reg.write(
                bar0_regmap_t::lmk_sync_ctrl_reg_t::LMK_CLKIN0_SYNC_SEL,
                bar0_regmap_t::lmk_sync_ctrl_reg_t::SYNC_PIN_SYNC);
        }
        config_lmk04832(_master_clock_rate);
    }

    void config_lmk05318() override
    {
        _lmk05318->pokes8({{0x000, 0x10},
            {0x001, 0x0B},
            {0x002, 0x35},
            {0x003, 0x00},
            {0x004, 0x00},
            {0x005, 0x00},
            {0x006, 0x00},
            {0x007, 0x00},
            {0x008, 0x02},
            {0x00A, 0xC8},
            {0x00B, 0x00},
            {0x00C, 0x3B},
            {0x00D, 0x08},
            {0x00E, 0x80},
            {0x00F, 0x00},
            {0x010, 0x20},
            {0x011, 0x1D},
            {0x012, 0xFF},
            {0x013, 0x00},
            {0x014, 0x00},
            {0x015, 0x01},
            {0x016, 0x00},
            {0x017, 0x55},
            {0x018, 0x55},
            {0x019, 0x00},
            {0x01A, 0x00},
            {0x01B, 0x00},
            {0x01C, 0x01},
            {0x01D, 0x13},
            {0x01E, 0x40},
            {0x020, 0x44},
            {0x023, 0x00},
            {0x024, 0x03},
            {0x025, 0x00},
            {0x026, 0x00},
            {0x027, 0x02},
            {0x028, 0x0F},
            {0x029, 0x00},
            {0x02A, 0x01},
            {0x02B, 0xC2},
            {0x02C, 0x00},
            {0x02D, 0x0C},
            {0x02E, 0xC8},
            {0x02F, 0x07},
            {0x030, 0x40},
            {0x031, 0x4E},
            {0x032, 0x00},
            {0x033, 0x80},
            {0x034, 0x00},
            {0x035, 0x2F},
            {0x036, 0x80},
            {0x037, 0x00},
            {0x038, 0x2F},
            {0x039, 0x00},
            {0x03A, 0x7C},
            {0x03B, 0x3B},
            {0x03C, 0x7C},
            {0x03D, 0x3B},
            {0x03E, 0x7C},
            {0x03F, 0x3B},
            {0x040, 0x95},
            {0x041, 0x02},
            {0x042, 0xF8},
            {0x043, 0xFF},
            {0x044, 0x01},
            {0x045, 0x00},
            {0x046, 0x00},
            {0x047, 0x00},
            {0x048, 0x26},
            {0x049, 0x00},
            {0x04A, 0x00},
            {0x04B, 0x03},
            {0x04C, 0x00},
            {0x04D, 0x0F},
            {0x04E, 0x00},
            {0x04F, 0x11},
            {0x050, 0x80},
            {0x051, 0x0A},
            {0x052, 0x00},
            {0x053, 0x03},
            {0x054, 0x84},
            {0x055, 0x00},
            {0x056, 0x00},
            {0x057, 0x1E},
            {0x058, 0x84},
            {0x059, 0x80},
            {0x05A, 0x00},
            {0x05B, 0x14},
            {0x05C, 0x00},
            {0x05D, 0x03},
            {0x05E, 0x84},
            {0x05F, 0x00},
            {0x060, 0x00},
            {0x061, 0x1E},
            {0x062, 0x84},
            {0x063, 0x80},
            {0x064, 0x29},
            {0x065, 0x03},
            {0x066, 0x22},
            {0x067, 0x0F},
            {0x068, 0x18},
            {0x069, 0x09},
            {0x06A, 0x00},
            {0x06B, 0x64},
            {0x06C, 0x00},
            {0x06D, 0xD0},
            {0x06E, 0x55},
            {0x06F, 0x55},
            {0x070, 0x55},
            {0x071, 0x55},
            {0x072, 0x55},
            {0x073, 0x03},
            {0x074, 0x01},
            {0x075, 0x00},
            {0x076, 0x00},
            {0x077, 0x00},
            {0x078, 0x00},
            {0x079, 0x00},
            {0x07A, 0x00},
            {0x07B, 0x50},
            {0x07C, 0x00},
            {0x07D, 0x00},
            {0x07E, 0x00},
            {0x07F, 0x00},
            {0x080, 0x00},
            {0x081, 0x02},
            {0x082, 0x00},
            {0x083, 0x01},
            {0x084, 0x01},
            {0x085, 0x77},
            {0x086, 0x00},
            {0x087, 0x29},
            {0x088, 0x00},
            {0x089, 0x17},
            {0x08A, 0x0C},
            {0x08B, 0x03},
            {0x08C, 0x02},
            {0x08D, 0x00},
            {0x08E, 0x01},
            {0x08F, 0x01},
            {0x090, 0x77},
            {0x091, 0x05},
            {0x092, 0x81},
            {0x093, 0x20},
            {0x095, 0x0D},
            {0x096, 0x00},
            {0x097, 0x01},
            {0x098, 0x0D},
            {0x099, 0x29},
            {0x09A, 0x24},
            {0x09B, 0x34},
            {0x09C, 0x01},
            {0x09D, 0x00},
            {0x09E, 0x00},
            {0x09F, 0x00},
            {0x0A0, 0x00},
            {0x0A1, 0xC8},
            {0x0A2, 0x97},
            {0x0A4, 0x00},
            {0x0A5, 0x00},
            {0x0A7, 0x00},
            {0x0B2, 0x00},
            {0x0B4, 0x00},
            {0x0B5, 0x00},
            {0x0B6, 0x00},
            {0x0B7, 0x00},
            {0x0B8, 0x00},
            {0x0B9, 0xF5},
            {0x0BA, 0x01},
            {0x0BB, 0x00},
            {0x0BC, 0x00},
            {0x0BD, 0x00},
            {0x0BE, 0x00},
            {0x0BF, 0x00},
            {0x0C0, 0x50},
            {0x0C1, 0x18},
            {0x0C2, 0x00},
            {0x0C3, 0x1F},
            {0x0C4, 0xFF},
            {0x0C5, 0xFF},
            {0x0C6, 0x1F},
            {0x0C7, 0xFF},
            {0x0C8, 0xFF},
            {0x0C9, 0x00},
            {0x0CA, 0x1F},
            {0x0CB, 0xFF},
            {0x0CC, 0xFF},
            {0x0CD, 0x1F},
            {0x0CE, 0xFF},
            {0x0CF, 0xFF},
            {0x0D0, 0x00},
            {0x0D1, 0x03},
            {0x0D2, 0x00},
            {0x0D3, 0x03},
            {0x0D4, 0x00},
            {0x0D5, 0x03},
            {0x0D6, 0x00},
            {0x0D7, 0x03},
            {0x0D8, 0x00},
            {0x0D9, 0x00},
            {0x0DA, 0x03},
            {0x0DB, 0xF9},
            {0x0DC, 0x41},
            {0x0DD, 0x00},
            {0x0DE, 0x02},
            {0x0DF, 0x08},
            {0x0E0, 0xD6},
            {0x0E1, 0x00},
            {0x0E2, 0x03},
            {0x0E3, 0xF9},
            {0x0E4, 0x41},
            {0x0E5, 0x00},
            {0x0E6, 0x02},
            {0x0E7, 0x08},
            {0x0E8, 0xD6},
            {0x0E9, 0x10},
            {0x0EA, 0x10},
            {0x0EB, 0x00},
            {0x0EC, 0xB7},
            {0x0ED, 0x1B},
            {0x0EE, 0x00},
            {0x0EF, 0x00},
            {0x0F0, 0xB7},
            {0x0F1, 0x1B},
            {0x0F2, 0x00},
            {0x0F3, 0x3F},
            {0x0F4, 0x00},
            {0x0F9, 0x21},
            {0x0FA, 0x00},
            {0x0FB, 0x03},
            {0x0FC, 0xE9},
            {0x0FD, 0x12},
            {0x0FE, 0x06},
            {0x0FF, 0xFC},
            {0x100, 0x00},
            {0x101, 0x01},
            {0x102, 0x00},
            {0x103, 0x00},
            {0x104, 0x02},
            {0x105, 0x80},
            {0x106, 0x01},
            {0x107, 0x2A},
            {0x108, 0x05},
            {0x109, 0xF2},
            {0x10A, 0x00},
            {0x10B, 0xA0},
            {0x10C, 0x04},
            {0x10D, 0x00},
            {0x10E, 0x03},
            {0x10F, 0x76},
            {0x110, 0x00},
            {0x111, 0x00},
            {0x112, 0x00},
            {0x113, 0x19},
            {0x114, 0x19},
            {0x115, 0x19},
            {0x116, 0x00},
            {0x117, 0x00},
            {0x118, 0x00},
            {0x119, 0x00},
            {0x11A, 0x00},
            {0x11B, 0x00},
            {0x11C, 0x1E},
            {0x11D, 0x1E},
            {0x11E, 0x00},
            {0x11F, 0x00},
            {0x120, 0x00},
            {0x121, 0x00},
            {0x122, 0x01},
            {0x123, 0x40},
            {0x124, 0x00},
            {0x125, 0x01},
            {0x126, 0x00},
            {0x127, 0x21},
            {0x128, 0x02},
            {0x129, 0x02},
            {0x12A, 0x02},
            {0x12B, 0x01},
            {0x12C, 0x00},
            {0x12D, 0x19},
            {0x12E, 0x1B},
            {0x12F, 0x00},
            {0x130, 0x0F},
            {0x131, 0x04},
            {0x132, 0x61},
            {0x133, 0xF8},
            {0x134, 0x43},
            {0x135, 0xC3},
            {0x136, 0xC3},
            {0x137, 0xC3},
            {0x138, 0xC3},
            {0x139, 0xC3},
            {0x13A, 0xFF},
            {0x13B, 0xFF},
            {0x13C, 0xFF},
            {0x13D, 0xFF},
            {0x13E, 0xFF},
            {0x13F, 0x03},
            {0x140, 0x00},
            {0x141, 0x0A},
            {0x142, 0x00},
            {0x143, 0x00},
            {0x144, 0x00},
            {0x145, 0x01},
            {0x146, 0x06},
            {0x147, 0x35},
            {0x148, 0x75},
            {0x149, 0x0B},
            {0x14A, 0x00},
            {0x14B, 0x64},
            {0x14C, 0x00},
            {0x14D, 0x00},
            {0x14E, 0x30},
            {0x14F, 0xD4},
            {0x150, 0x06},
            {0x151, 0x35},
            {0x152, 0x75},
            {0x153, 0x0B},
            {0x154, 0x00},
            {0x155, 0x00},
            {0x156, 0x00},
            {0x157, 0x00},
            {0x158, 0x00},
            {0x159, 0x00},
            {0x15A, 0x00},
            {0x15B, 0x00},
            {0x15C, 0x30},
            {0x15D, 0x2E},
            {0x15E, 0x6F},
            {0x15F, 0xA9},
            {0x160, 0x00},
            {0x165, 0x28},
            {0x16F, 0x00},
            {0x19B, 0x08}});
    }

    void power_down_lmk04832_sysref() override
    {
        // This takes the original value programmed to R140 and changes SYSREF to be
        // powered down. This is because it is only needed during initialization and can
        // cause RF spurs while running.
        _lmk04832->pokes8({
            {0x0140,
                (_distributed_clocks
                        ? 0x65
                        : 0x05)}, // Enabled PLL1, VCO_LDO, VCO, OSCin, SYSREF
                                  // Digital Delay; Disabled: SYSREF, SYSREF Pulser. For
                                  // distributed clocking, VCO and VCO_LDO are powered
                                  // down as well.
        });
    }

    void config_lmk04832_for_sync() override
    {
        if (_distributed_clocks) {
            _lmk04832->pokes8({
                {0x0143, 0x91},
                {0x0144, 0x7F},
                {0x0139, 0x00},
            });
        } else if (_10M_ext_ref) {
            _lmk04832->init_pll1_r_divider_sync();
        }

        uint16_t delay = 0;
        if (_10M_ext_ref) {
            delay = uhd::math::frequencies_are_equal(_master_clock_rate, 125e6) ? 62 : 61;
        }

        // Set Ref Clock PPS Delay
        _bar0_regmap->pps_in_ctrl_reg.write(
            bar0_regmap_t::pps_in_ctrl_reg_t::PPS_IN_TO_RCLK_DELAY, delay);
        // Set PPS Sync LMK Delay
        _bar0_regmap->lmk_sync_ctrl_reg.write(
            bar0_regmap_t::lmk_sync_ctrl_reg_t::LMK_SYNC_DELAY, 0);
        // Set LMK Sync Edge
        if (_10M_ext_ref) {
            _bar0_regmap->lmk_sync_ctrl_reg.write(
                bar0_regmap_t::lmk_sync_ctrl_reg_t::LMK_SYNC_CLK_SEL,
                bar0_regmap_t::lmk_sync_ctrl_reg_t::LMK_SYNC_REF_CLK);
        } else {
            _bar0_regmap->lmk_sync_ctrl_reg.write(
                bar0_regmap_t::lmk_sync_ctrl_reg_t::LMK_SYNC_CLK_SEL,
                bar0_regmap_t::lmk_sync_ctrl_reg_t::LMK_SYNC_RADIO_CLK);
        }
        // Set LMK Trigger Sync
        _bar0_regmap->lmk_sync_ctrl_reg.write(
            bar0_regmap_t::lmk_sync_ctrl_reg_t::LMK_SYNC_TRIGGER, 1);
    }

    void finish_lmk04832_sync() override
    {
        // Wait for LMK_SYNC_DONE with 1 second timeout. This will happen on the next
        // PPS pulse
        auto start     = std::chrono::steady_clock::now();
        bool sync_done = false;
        while (!sync_done) {
            uint32_t reg_value = _bar0_regmap->lmk_sync_ctrl_reg.read(
                bar0_regmap_t::lmk_sync_ctrl_reg_t::LMK_SYNC_DONE);
            sync_done = (reg_value != 0);

            if (!sync_done) {
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - start)
                                   .count();
                // Give a 100 ms buffer for the second timeout.
                if (elapsed >= 1100) {
                    throw uhd::runtime_error(
                        "Timeout waiting for LMK04832 sync to complete");
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }

        // Unset LMK Trigger Sync
        _bar0_regmap->lmk_sync_ctrl_reg.write(
            bar0_regmap_t::lmk_sync_ctrl_reg_t::LMK_SYNC_TRIGGER, 0);

        _lmk04832->pokes8({
            {0x0144, 0xFF},
            {0x0139, 0x03},
            {0x0143, 0x11},
        });

        if (_10M_ext_ref) {
            _lmk04832->deinit_pll1_r_divider_sync();
        }

        if (!_distributed_clocks) {
            _bar0_regmap->sw_resets_reg.write(
                bar0_regmap_t::sw_resets_reg_t::RADIO_CLK_GEN_RST, 1);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            _bar0_regmap->sw_resets_reg.write(
                bar0_regmap_t::sw_resets_reg_t::RADIO_CLK_GEN_RST, 0);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    void set_ext_clk_rate(const double ext_clk_rate) override
    {
        if (!uhd::math::frequencies_are_equal(ext_clk_rate, _master_clock_rate)
            && !uhd::math::frequencies_are_equal(ext_clk_rate, 10e6)) {
            throw uhd::value_error(
                "Invalid external clock rate: " + std::to_string(ext_clk_rate / 1e6)
                + " MHz. Valid rates are 10 MHz or the master clock rate.");
        }
        _ext_clk_rate = ext_clk_rate;
    }

    bool get_ref_locked() const override
    {
        if (_distributed_clocks) {
            // The distributed clocking case only uses PLL1.
            return _lmk04832->check_plls_locked(lmk04832_iface::lmk04832_pll_sel::PLL1);
        } else {
            return _lmk04832->check_plls_locked(lmk04832_iface::lmk04832_pll_sel::BOTH);
        }
    }

    bool get_ref_stable() const override
    {
        if (_distributed_clocks) {
            // The distributed clocking case only uses PLL1.
            return _lmk04832->check_plls_locked(
                lmk04832_iface::lmk04832_pll_sel::PLL1, true);
        } else {
            return _lmk04832->check_plls_locked(
                lmk04832_iface::lmk04832_pll_sel::BOTH, true);
        }
    }

    bool get_dpll_locked(lmk05318_iface::dpll_lock_check_t check =
                             lmk05318_iface::dpll_lock_check_t::BOTH) const override
    {
        return _lmk05318->get_dpll_lock(check);
    }

    bool wait_for_ref_locked(uint32_t timeout_ms) const override
    {
        if (_distributed_clocks) {
            // The distributed clocking case only uses PLL1.
            return _lmk04832->wait_for_pll_lock(
                lmk04832_iface::lmk04832_pll_sel::PLL1, timeout_ms);
        } else {
            return _lmk04832->wait_for_pll_lock(
                lmk04832_iface::lmk04832_pll_sel::BOTH, timeout_ms);
        }
    }

    double get_master_clock_rate(void) override
    {
        return _master_clock_rate;
    }

    bool validate_lmk05318_priref(void) const override
    {
        // Check if the LMK05318 primary reference is valid by reading the PRIREF_VALSTAT
        // bit.
        uint8_t status = _lmk05318->peek8(0x19B);
        return ((status >> 2) & 0x01) != 0; // Check the PRIREF_VALSTAT bit.
    }

private:
    lmk04832_iface::sptr _lmk04832;
    lmk05318_iface::sptr _lmk05318;
    bar0_regmap_t::sptr _bar0_regmap;
    // This is technically constant, but it can be coerced during initialization
    double _master_clock_rate;
    double _ext_clk_rate;
    int _output_divider;
    int _pll1_n_div;
    int _pll2_n_div;
    int _pll2_n_cal_div;
    int _pll2_prescaler;
    int _clkin0_r_div;
    int _clkin1_r_div;
    int _clkin2_r_div;
    int _sysref_div;
    clkin_sel_t _clkin_sel;
    vcxo_sel_t _vcxo_freq;
    bool _distributed_clocks;
    bool _10M_ext_ref;
    uint16_t _board_rev;

    void _program_non_distributed_clkout(int clk_ddly, int clkout8_ddly)
    {
        double vco_freq = _master_clock_rate * _output_divider;
        uint8_t int_vco = 0x1;
        if (vco_freq >= LMK04832_VCO0_RANGE_MIN && vco_freq <= LMK04832_VCO0_RANGE_MAX) {
            int_vco = 0x0;
        } else if (vco_freq >= LMK04832_VCO1_RANGE_MIN
                   && vco_freq <= LMK04832_VCO1_RANGE_MAX) {
            int_vco = 0x1;
        } else {
            throw uhd::runtime_error("Invalid VCO frequency requested for LMK04832");
        }
        // CLKout Config
        _lmk04832->pokes8({
            // CLKout 0 goes to Palma DEVCLK, CLKout 1 goes to Palma SYSREF Input
            {0x0100, _output_divider}, // set Device CLKout 0 and 1 divider
            {0x0101, clk_ddly}, // set Digital Delay for CLKout 0 and 1
            {0x0102, 0x60}, // enable CLKout 0 and 1, set output and input drive to higher
            // current consumption, enable digital delay for 0 and 1
            {0x0103, 0x46}, // select Device clock output for 0, enable Device Clk Output,
            // high performance bypass disabled, duty cycle correction
            // enabled, inverted polarity, no half step adjustment
            {0x0104, 0x22}, // select SYSREF Clock output for 1, enable SYSREF path,
                            // normal SYSREF disable mode, inverted SYSREF polarity,
                            // disable half step adjustment
            {0x0105, 0x00}, // disable analog SYSREF delay for 0 and 1, default analog
                            // delay value set to 125 ps
            {0x0106, 0x01}, // set local SYSREF digital delay to 1
            {0x0107,
                0x14}, // Set Output format of CLKout 0 to LVPECL 1600mV and 1 to LVDS

            // CLKout 8 goes to the LMK Test Point, CLKout 9 is NC
            {0x0120, (_sysref_div & 0x00FF)}, // set Device CLKout 8 and 9 divider
            {0x0121, clkout8_ddly}, // set Digital Delay for CLKout 8 and 9
            {0x0122,
                (_10M_ext_ref ? 0x60 : 0x90)
                    | ((_sysref_div & 0x0300)
                        >> 8)}, // disable CLKout 8 and 9 except for 10MHz external
                                // ref, set output and input drive to higher current
                                // consumption, enable digital delay for 8 and 9 for
                                // 10MHz external ref only
            {0x0123, 0x44}, // select Device clock output for 8, enable Device Clk Output,
            // high performance bypass disabled, duty cycle correction
            // enabled, normal polarity, no half step adjustment
            {0x0124, 0x10}, // select Device Clock output for 9, disable SYSREF path,
                            // normal SYSREF disable mode, normal SYSREF polarity,
                            // disable half step adjustment
            {0x0125, 0x00}, // disable analog SYSREF delay for 8 and 9, default analog
                            // delay value set to 125 ps
            {0x0126, 0x01}, // set local SYSREF SCLK digital delay to 1
            {0x0127, 0x00}, // Set Output format of CLKout 8 and 9 to Powerdown

            // CLKout 10 goes to the FPGA (DevCLK), CLKout 11 goes to the FPGA as
            // SysRef
            {0x0128, _output_divider}, // set Device CLKout 10 and 11 divider
            {0x0129, clk_ddly}, // set Digital Delay for CLKout 10 and 11
            {0x012A, 0x60}, // enable CLKout 10 and 11, set output and input drive to
                            // higher
            // current consumption, enable digital delay for 10 and 11
            {0x012B,
                0x44}, // select SYSREF clock output for 10, enable Device Clk Output,
            // high performance bypass disabled, duty cycle correction enabled,
            // normal polarity, no half step adjustment
            {0x012C, 0x20}, // select SYSREF Clock output for 11, enable SYSREF path,
                            // normal SYSREF disable mode, normal SYSREF polarity,
                            // disable half step adjustment
            {0x012D, 0x00}, // disable analog SYSREF delay for 10 and 11, default
                            // analog delay value set to 125 ps
            {0x012E, 0x01}, // set local SYSREF SCLK digital delay to 1
            {0x012F, 0x11}, // Set Output format of CLKout 10 and 11 both to LVDS

            // CLKout 12 is NC, CLKout 13 goes to the FPGA MGT Ref Clock for the MGT
            // lanes
            // associated with JESD
            {0x0130, _output_divider}, // set Device CLKout 12 and 13 divider
            {0x0131, clk_ddly}, // set Digital Delay for CLKout 12 and 13
            {0x0132, 0x00}, // enable CLKout 12 and 13, set output and input drive
                            // level normal, enable digital delay for 12 and 13
            {0x0133, 0x44}, // select Device clock output for 12, enable CLKout 12 and 13,
            // high performance bypass disabled, duty cycle correction
            // enabled, normal polarity, no half step adjustment
            {0x0134, 0x10}, // select Device Clock output for 13, disable SYSREF path,
                            // normal SYSREF disable mode, normal SYSREF polarity,
                            // disable half step adjustment
            {0x0135, 0x00}, // disable analog SYSREF delay for 12 and 13, default
                            // analog delay value set to 125 ps
            {0x0136, 0x01}, // set local SYSREF SCLK digital delay to 1
            {0x0137, 0x10}, // Set Output format of CLKout 12 to Powerdown and 13 to LVDS
            {0x0138,
                (int_vco & 0x1) << 5}, // VCO_MUX, choose VCO0 or VCO1, power down OSCout
        });
    }

    void _program_distributed_clkout(int clk_ddly)
    {
        // CLKout Config
        _lmk04832->pokes8({
            // CLKout 0 goes to Palma DEVCLK, CLKout 1 goes to Palma SYSREF Input
            {0x0100, _output_divider}, // set Device CLKout 0 and 1 divider
            {0x0101, clk_ddly}, // set Digital Delay for CLKout 0 and 1
            {0x0102, 0x70}, // enable CLKout 0 and 1, set output and input drive to higher
            // current consumption, disable digital delay for 0 and 1
            {0x0103, 0x46}, // select Device clock output for 0, enable Device Clk Output,
            // high performance bypass disabled, duty cycle correction
            // enabled, inverted polarity, no half step adjustment
            {0x0104, 0x23}, // select SYSREF Clock output for 1, enable SYSREF path,
                            // normal SYSREF disable mode, inverted SYSREF polarity,
                            // enable half step adjustment
            {0x0105, 0x00}, // disable analog SYSREF delay for 0 and 1, default analog
                            // delay value set to 125 ps
            {0x0106, 0x01}, // set local SYSREF digital delay to 1
            {0x0107,
                0x14}, // Set Output format of CLKout 0 to LVPECL 1600mV and 1 to LVDS

            // CLKout 8 goes to the LMK Test Point, CLKout 9 is NC
            {0x0120, (_sysref_div & 0x00FF)}, // set Device CLKout 8 and 9 divider
            {0x0121, clk_ddly}, // set Digital Delay for CLKout 8 and 9
            {0x0122,
                0x80
                    | ((_sysref_div & 0x0300)
                        >> 8)}, // disable CLKout 8 and 9, set output and input drive
                                // level normal, disable digital delay for 8 and 9
            {0x0123, 0x44}, // select Device clock output for 8, enable Device Clk Output,
            // high performance bypass disabled, duty cycle correction
            // enabled, normal polarity, no half step adjustment
            {0x0124, 0x10}, // select Device Clock output for 9, disable SYSREF path,
                            // normal SYSREF disable mode, normal SYSREF polarity,
                            // disable half step adjustment
            {0x0125, 0x00}, // disable analog SYSREF delay for 8 and 9, default analog
                            // delay value set to 125 ps
            {0x0126, 0x01}, // set local SYSREF SCLK digital delay to 1
            {0x0127, 0x00}, // Set Output format of CLKout 8 and 9 to Powerdown

            // CLKout 10 goes to the FPGA (DevCLK), CLKout 11 goes to the FPGA as
            // SysRef
            {0x0128, _output_divider}, // set Device CLKout 10 and 11 divider
            {0x0129, clk_ddly}, // set Digital Delay for CLKout 10 and 11
            {0x012A, 0x70}, // enable CLKout 10 and 11, set output and input drive to
                            // higher
            // current consumption, disable digital delay for 10 and 11
            {0x012B,
                0x44}, // select Device clock output for 10, enable Device Clk Output,
            // high performance bypass disabled, duty cycle correction
            // enabled, normal polarity, no half step adjustment
            {0x012C, 0x21}, // select SYSREF Clock output for 11, enable SYSREF path,
                            // normal SYSREF disable mode, normal SYSREF polarity,
                            // enable half step adjustment
            {0x012D, 0x00}, // disable analog SYSREF delay for 10 and 11, default
                            // analog delay value set to 125 ps
            {0x012E, 0x01}, // set local SYSREF SCLK digital delay to 1
            {0x012F, 0x11}, // Set Output format of CLKout 10 and 11 both to LVDS

            // CLKout 12 is NC, CLKout 13 goes to the FPGA MGT Ref Clock for the MGT
            // lanes associated with JESD
            {0x0130, _output_divider}, // set Device CLKout 12 and 13 divider
            {0x0131, clk_ddly}, // set Digital Delay for CLKout 12 and 13
            {0x0132, 0x00}, // enable CLKout 12 and 13, set output and input drive
                            // level normal, enable digital delay for 12 and 13
            {0x0133,
                0x44}, // select Device clock output for 12, enable Device Clk Output,
            // high performance bypass disabled, duty cycle correction
            // enabled, normal polarity, no half step adjustment
            {0x0134, 0x10}, // select Device Clock output for 13, disable SYSREF path,
                            // normal SYSREF disable mode, normal SYSREF polarity,
                            // disable half step adjustment
            {0x0135, 0x00}, // disable analog SYSREF delay for 12 and 13, default
                            // analog delay value set to 125 ps
            {0x0136, 0x01}, // set local SYSREF SCLK digital delay to 1
            {0x0137, 0x10}, // Set Output format of CLKout 12 to Powerdown and 13 to LVDS
            {0x0138,
                0x50}, // VCO_MUX set to Fin1 / CLKin1, OSCout_MUX set to Feedback Mux
        });
    }

    void _prog_plls_enabled(int sysref_delay, uint32_t prescaler)
    {
        int clkin_select_val;
        if (_board_rev == 1) {
            clkin_select_val =
                (_clkin_sel == CLKin0) ? 0x8E : (_distributed_clocks ? 0x82 : 0x9B);
        } else {
            clkin_select_val =
                (_clkin_sel == CLKin2) ? 0xAC : (_distributed_clocks ? 0xA0 : 0x98);
        }
        // PLL Config
        _lmk04832->pokes8({
            {0x0139, 0x00}, // Set SysRef source to 'Normal SYNC' (SYSREF_MUX=0) as we
                            // initially use the sync signal to synchronize dividers,
                            // disable SYSREF_REQ pin, normal SYSREF polarity, normal SYNC
            {0x013A,
                (_sysref_div & 0x1F00)
                    >> 8}, // SYSREF Divide [12:8], set global SYSREF divider
            {0x013B,
                (_sysref_div & 0x00FF)
                    >> 0}, // SYSREF Divide [7:0], set global SYSREF divider
            {0x013C,
                (sysref_delay & 0x1F00)
                    >> 8}, // SYSREF DDLY [12:8], set global SYSREF digital delay
            {0x013D,
                (sysref_delay & 0x00FF) >> 0}, // shift SYSREF according to LMK data sheet
            {0x013E, 0x03}, // set number of SYSREF pulse to 8(Default)
            {0x013F,
                _10M_ext_ref
                    ? 0x0B
                    : 0x00}, // PLL2_RCLK_MUX = OSCin, PLL2_NCLK_MUX = PLL2 Prescaler,
                             // PLL1_NCLK_MUX = OSCin, FB_MUX = CLKout6, FB_MUX_EN =
                             // Powered Down. For external 10MHz clock PLL1_NCLK_MUX =
                             // Feedback Mux, FB_MUX enabled at CLKout8.
            {0x0140,
                (_distributed_clocks
                        ? 0x61
                        : 0x01)}, // Enabled PLL1, VCO_LDO, VCO, OSCin, SYSREF, SYSREF
                                  // Digital Delay; Disabled: SYSREF Pulser. For
                                  // distributed clocking, VCO and VCO_LDO are powered
                                  // down as well.
            {0x0141, 0x00}, // Disable dynamic digital delay.
            {0x0142, 0x00}, // Set dynamic digital delay step count to 0.
            {0x0143, 0x11}, // Initial SYNC configuration: SYSREF_CLR=0, SYNC_1SHOT_EN=0,
                            // SYNC_POL=0, SYNC_EN=1, SYNC_PLL{1,2}_DLD=0, SYNC_MODE=1
            {0x0144, 0x00}, // Most Clock Outputs prevented from being synchronized.
            {0x0145,
                (_board_rev == 1)
                    ? 0x10
                    : 0x20}, // Disable PLL1 R divider SYNC, source for PLL1 R Divider
                             // Sync set to SYNC Pin for Rev A, CLKin0 for later
                             // revs, disable PLL2 R divider SYNC
            {0x0146,
                (_board_rev == 1)
                    ? 0x1C
                    : 0x31}, // disable CLKin_SEL_PIN, set CLKin_SEL_PIN to normal
                             // polarity, Rev A: enable CLKIn0/1 auto-switching mode -
                             // later revs: enable CLKin1/2 auto-switching mode, disable
                             // CLKIn2 auto-switching mode, CLKIN0 type = Bipolar for Rev
                             // A and MOS for later revs, CLKIN1 type = Bipolar, CLKIN2
                             // type = MOS for Rev A and Bipolar for later revs.
            {0x0147, clkin_select_val}, // Auto Clkin Select disabled,
                                        // CLKin_SEL_MANUAL= _clkin_sel, for active
                                        // clocking source DEMUX set to PLL1, inactive
                                        // set to Off. In distributed clocking mode,
                                        // CLKin1 set to Fin for distribution and
                                        // CLKin0 set to PLL1 for tracking.
            {0x0148, 0x33}, // CLKIn_SEL0_MUX = SPI readback with CLKin_SEL0_TYPE
                            // output set to push-pull
            {0x0149, 0x42}, // Set SPI readback Output, open drain
            {0x014A, 0x00}, // Set RESET pin as Input
            {0x014B, 0x06}, // Default
            {0x014C, 0x00}, // Default
            {0x014D, 0x00}, // Default
            {0x014E, 0xC0}, // Default
            {0x014F, 0x7F}, // Default
            {0x0150, 0x20}, // Default and holdover set to Exit based on PLL1 DLD
            {0x0151, 0x02}, // Default
            {0x0152, 0x00}, // Default
            {0x0153, (_clkin0_r_div & 0x3F00) >> 8}, // CLKin0_R divider [13:8]
            {0x0154, (_clkin0_r_div & 0x00FF) >> 0}, // CLKin0_R divider [7:0]
            {0x0155, (_clkin1_r_div & 0x3F00) >> 8}, // CLKin1_R divider [13:8]
            {0x0156, (_clkin1_r_div & 0x00FF) >> 0}, // CLKin1_R divider [7:0]
            {0x0157, (_clkin2_r_div & 0x3F00) >> 8}, // CLKin2_R divider [13:8]
            {0x0158, (_clkin2_r_div & 0x00FF) >> 0}, // CLKin2_R divider [7:0]
            {0x0159, (_pll1_n_div & 0x3F00) >> 8}, // PLL1 N divider [13:8], default = 0
            {0x015A, (_pll1_n_div & 0x00FF) >> 0}, // PLL1 N divider [7:0], default = d120
            {0x015B,
                (_board_rev == 1)
                    ? 0xDE
                    : 0xC5}, // Set PLL1 window size to 43ns, PLL1 CP ON, negative
                             // polarity (rev A is positive polarity), CP gain is 1.45 mA
                             // for Rev A and 550 uA for later revs.
            {0x015C, 0x20}, // Pll1 lock detect count is 8192 cycles (default)
            {0x015D, 0x00}, // Pll1 lock detect count is 8192 cycles (default)
            {0x015E, 0x1E}, // Default holdover relative time between PLL1 R and PLL1
                            // N divider of "30"
            {0x015F,
                _distributed_clocks ? 0x0B
                                    : 0x1B}, // PLL1 & PLL2 locked status in Status_LD1
                                             // pin, only PLL1 for distributed clocks.
                                             // Status_LD1 pin is output (push-pull).
            {0x0160, 0x00}, // PLL2 R divider is 1
            {0x0161, 0x01}, // PLL2 R divider is 1
            {0x0162, prescaler}, // PLL2 prescaler; OSCin freq;
            {0x0163, (_pll2_n_cal_div & 0x030000) >> 16}, // PLL2 N Cal[17:16]
            {0x0164, (_pll2_n_cal_div & 0x00FF00) >> 8}, // PLL2 N Cal[15:8]
            {0x0165, (_pll2_n_cal_div & 0x0000FF) >> 0}, // PLL2 N Cal[7:0]
            {0x0169, 0x58}, // PLL2 CP gain is 3.2 mA, PLL2 window is 1.8 ns, negative
                            // CP polarity, PLL2_DLD is forced on
            {0x016A, 0x20}, // PLL2 lock detect count is 8192 cycles (default)
            {0x016B, 0x00}, // PLL2 lock detect count is 8192 cycles (default)
            {0x016E, 0x13}, // Status_LD2 pin not used. Don't care about this
                            // register, select PLL2_DLD as output (push-pull)
            {0x0173,
                (_distributed_clocks
                        ? 0x70
                        : 0x10)}, // PLL2 prescaler and PLL2 are enabled, for
                                  // distributed clocking mode they are both disabled.
            {0x0177, 0x00}, // PLL1 R divider not in reset
            // programming the register 0x168 last as it triggers the VCO calibration
            // routine
            {0x0166, (_pll2_n_div & 0x030000) >> 16}, // PLL2 N[17:16]
            {0x0167, (_pll2_n_div & 0x00FF00) >> 8}, // PLL2 N[15:8]
            {0x0168, (_pll2_n_div & 0x0000FF) >> 0}, // PLL2 N[7:0]
        });

        // Synchronize Output and SYSREF Dividers
        _lmk04832->pokes8({
            {0x0143, 0x91}, // Set SYNC_EN=1
            {0x0143, 0xB1}, // Toggle SYNC_POL on
            {0x0143, 0x91}, // ...and off again
            {0x0144, 0xFF}, // Prevent sysref and other clock outputs from being
                            // synchronized or interrupted by a SYNC event
            {0x0143, 0x11}, // Now set SYNC_EN=1, SYNC_MODE=1, and clear SYSREF_CLR
            {0x0139, 0x03}, // SYSREF_REQ_EN=0, SYSREF_MUX=3 (continuous SYSREF)
        });

        // Wait for PLL lock

        if (!_distributed_clocks) {
            // PLL2 should lock first and be relatively fast (300 us)
            if (!_lmk04832->wait_for_pll_lock(
                    lmk04832_iface::lmk04832_pll_sel::PLL2, 100)) {
                throw uhd::runtime_error("Sample Clock PLL2 failed to lock!");
            }
        }
        // PLL1 may take up to 2 seconds to lock
        if (!_lmk04832->wait_for_pll_lock(lmk04832_iface::lmk04832_pll_sel::PLL1, 2000)) {
            throw uhd::runtime_error("Sample Clock PLL1 failed to lock!");
        }
    }

    int _get_clk_digital_delay(int digital_delay, int clock_divider = -1) const
    {
        // Table 18 adjustments from LMK04832 datasheet.
        // e.g. with a Dev. CLK divider of '2' the Data Sheet mentions an adjustment
        // value of '-2' which requires us to compensate by adding '+2' to the digital
        // delay
        if (clock_divider != -1) {
            switch (clock_divider) {
                case 2:
                case 3:
                    digital_delay += 2;
                    break;
                case 5:
                    digital_delay -= 3;
                    break;
                case 6:
                    digital_delay -= 1;
                    break;
                default:
                    break;
            }
        }
        return digital_delay;
    }
};

b300_clock_ctrl::sptr b300_clock_ctrl::make(bar0_regmap_t::sptr bar0_regmap,
    uhd::spi_iface::sptr spi,
    double ext_clock_rate,
    uint16_t board_rev)
{
    return std::make_shared<b300_clock_ctrl_impl>(
        std::move(bar0_regmap), std::move(spi), ext_clock_rate, board_rev);
}
}}} // namespace uhd::usrp::b300
