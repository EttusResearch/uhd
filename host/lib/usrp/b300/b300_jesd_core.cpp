//
// Copyright 2025 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "b300_jesd_core.hpp"
#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <chrono>
#include <map>
#include <thread>

namespace {
static constexpr uint32_t JESD_CTRL_WINDOW = 0x81000;
constexpr uint32_t JESD_ADDR(uint32_t x)
{
    return JESD_CTRL_WINDOW + x;
}
// Register offsets
static constexpr uint32_t MGT_QPLL_CONTROL           = JESD_ADDR(0x000);
static constexpr uint32_t MGT_CPLL_CONTROL1          = JESD_ADDR(0x004);
static constexpr uint32_t MGT_CPLL_CONTROL2          = JESD_ADDR(0x008);
static constexpr uint32_t MGT_PLL_POWER_DOWN_CONTROL = JESD_ADDR(0x00C);
static constexpr uint32_t MGT_CPLL_CONTROL3          = JESD_ADDR(0x010);
static constexpr uint32_t MGT_TX_RESET_CONTROL       = JESD_ADDR(0x020);
static constexpr uint32_t MGT_RX_RESET_CONTROL       = JESD_ADDR(0x024);
static constexpr uint32_t MGT_RECEIVER_CONTROL       = JESD_ADDR(0x040);
static constexpr uint32_t MGT_RX_LPM_LF_CONTROL      = JESD_ADDR(0x044);
static constexpr uint32_t MGT_RX_LPM_HF_CONTROL      = JESD_ADDR(0x048);
static constexpr uint32_t MGT_RX_LPM_UPDATE          = JESD_ADDR(0x04C);
static constexpr uint32_t MGT_RX_DESCRAMBLER_CONTROL = JESD_ADDR(0x050);
static constexpr uint32_t MGT_TRANSMITTER_CONTROL    = JESD_ADDR(0x060);
static constexpr uint32_t MGT_TX_TRANSCEIVER_CONTROL = JESD_ADDR(0x064);
static constexpr uint32_t MGT_TX_SCRAMBLER_CONTROL   = JESD_ADDR(0x068);
static constexpr uint32_t LMK_SYNC_CONTROL           = JESD_ADDR(0x06C);
static constexpr uint32_t JESD_MGT_DRP_CONTROL       = JESD_ADDR(0x070);
static constexpr uint32_t JESD_MGT_TEST_CONTROL      = JESD_ADDR(0x074);
static constexpr uint32_t SYSREF_CAPTURE_CONTROL     = JESD_ADDR(0x078);
static constexpr uint32_t JESD_SIGNATURE_REG         = JESD_ADDR(0x100);
static constexpr uint32_t JESD_REVISION_REG          = JESD_ADDR(0x104);
static constexpr uint32_t JESD_OLD_COMPAT_REV_REG    = JESD_ADDR(0x108);
static constexpr uint32_t JESD_SCRATCH_REG           = JESD_ADDR(0x10C);

static constexpr uint32_t OLDEST_COMPAT_REVISION = 0x17080815;
static constexpr uint32_t CURRENT_REVISION       = 0x18110916;
} // namespace

using namespace uhd::usrp::b300;

b300_jesd_core::b300_jesd_core(write_fn_t&& poke32,
    read_fn_t&& peek32,
    size_t num_qplls,
    size_t num_cplls,
    uint8_t lmfc_divider,
    uint8_t rx_sysref_delay,
    uint8_t tx_sysref_delay)
    : _poke32(std::move(poke32))
    , _peek32(std::move(peek32))
    , _num_qplls(num_qplls)
    , _num_cplls(num_cplls)
    , _lmfc_divider(lmfc_divider)
    , _rx_sysref_delay(rx_sysref_delay)
    , _tx_sysref_delay(tx_sysref_delay)
{
    // Initial test of scratch register before full JESD config
    _poke32(JESD_SCRATCH_REG, 0x12345678);
    if (_peek32(JESD_SCRATCH_REG) != 0x12345678) {
        throw uhd::runtime_error("Failed to read or write JESD scratch register");
    }
    if (_peek32(JESD_SIGNATURE_REG) != 0x4a455344) {
        throw uhd::runtime_error(
            "JESD Core signature mismatch! Check that core is mapped correctly");
    }

    uint32_t jesd_current_revision    = _peek32(JESD_REVISION_REG);
    uint32_t jesd_old_compat_revision = _peek32(JESD_OLD_COMPAT_REV_REG);
    if (jesd_current_revision < OLDEST_COMPAT_REVISION) {
        throw uhd::runtime_error("JESD core on FPGA is too old for this version of UHD "
                                 "(FPGA current revision: "
                                 + std::to_string(jesd_current_revision)
                                 + ", UHD oldest compatible revision: "
                                 + std::to_string(OLDEST_COMPAT_REVISION)
                                 + "). Please update the FPGA image.");
    }
    if (CURRENT_REVISION < jesd_old_compat_revision) {
        throw uhd::runtime_error(
            "JESD core on FPGA is too new for this version of UHD (FPGA oldest "
            "compatible revision: "
            + std::to_string(jesd_old_compat_revision) + ", UHD current revision: "
            + std::to_string(CURRENT_REVISION) + "). Please update UHD.");
    }
}

void b300_jesd_core::reset()
{
    _gt_reset("tx", true);
    _gt_reset("rx", true);
    _gt_pll_lock_control(true);
    enable_lmfc(false);
}

void b300_jesd_core::init_deframer(bool bypass_descrambler)
{
    _poke32(MGT_RECEIVER_CONTROL, 0x2);
    _poke32(MGT_RX_DESCRAMBLER_CONTROL, bypass_descrambler ? 0x1 : 0x0);
    _gt_reset("rx", false);
    _poke32(MGT_RECEIVER_CONTROL, 0x0);
}

void b300_jesd_core::init_framer(bool bypass_scrambler)
{
    _poke32(MGT_TRANSMITTER_CONTROL, (1 << 13) | (1 << 1));
    _gt_reset("tx", false);
    uint32_t reg_val = ((_tx_driver_swing & 0x0F) << 16) | ((_tx_precursor & 0x1F) << 8)
                       | ((_tx_postcursor & 0x1F) << 0);
    _poke32(MGT_TX_TRANSCEIVER_CONTROL, reg_val);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    _poke32(MGT_TX_SCRAMBLER_CONTROL, bypass_scrambler ? 0x01 : 0x10);
    uint32_t rb = _peek32(MGT_TRANSMITTER_CONTROL);
    if ((rb & 0x100) != 0x100) {
        throw uhd::runtime_error("TX Framer is not idle after reset");
    }
    _poke32(MGT_TRANSMITTER_CONTROL, 1 << 12);
    _poke32(MGT_TRANSMITTER_CONTROL, 1 << 0);
}

bool b300_jesd_core::get_framer_status()
{
    uint32_t rb = _peek32(MGT_TRANSMITTER_CONTROL);
    return (rb & 0xFF0) == 0x6C0;
}

bool b300_jesd_core::get_deframer_status(bool ignore_sysref)
{
    uint32_t rb       = _peek32(MGT_RECEIVER_CONTROL);
    bool cgs_pass     = (rb & (1 << 2)) > 0;
    bool ila_pass     = (rb & (1 << 3)) > 0;
    bool sys_ref_pass = ((rb & (1 << 5)) > 0) || ignore_sysref;
    bool mgt_pass     = (rb & (1 << 21)) == 0;
    return cgs_pass && ila_pass && sys_ref_pass && mgt_pass;
}

void b300_jesd_core::init()
{
    _gt_pll_power_control();
    _gt_reset("tx", true);
    _gt_reset("rx", true);
    _gt_pll_lock_control(false);
    // Enable SYSREF sampler before link training.
    enable_lmfc(true);
}

void b300_jesd_core::enable_lmfc(bool enable)
{
    uint32_t disable_bit = enable ? 0 : 1;
    uint32_t reg_val     = ((_lmfc_divider - 1) << 23) | (_rx_sysref_delay << 16)
                       | (_tx_sysref_delay << 8) | (disable_bit << 6);
    _poke32(SYSREF_CAPTURE_CONTROL, reg_val);
}

void b300_jesd_core::reset_lmfc()
{
    // LMFC counter reset located in bit 7 of the SYSREF register, it needs to be set
    // and then cleared to reset the LMFC counters.
    uint32_t current_val = _peek32(SYSREF_CAPTURE_CONTROL);
    _poke32(SYSREF_CAPTURE_CONTROL, current_val | 0x80);
    _poke32(SYSREF_CAPTURE_CONTROL, current_val & ~0x80);
}

void b300_jesd_core::send_sysref_pulse()
{
    _poke32(LMK_SYNC_CONTROL, 1 << 30);
}

void b300_jesd_core::set_pattern_gen(const std::string& mode)
{
    static const std::map<std::string, uint32_t> TXPRBSSEL = {{"OFF", 0b000},
        {"PRBS-7", 0b001},
        {"PRBS-15", 0b010},
        {"PRBS-23", 0b011},
        {"PRBS-31", 0b100},
        {"PCIE", 0b101},
        {"SQR-2UI", 0b110},
        {"SQR-xUI", 0b111}};

    auto it = TXPRBSSEL.find(mode);
    if (it == TXPRBSSEL.end())
        throw std::invalid_argument("Invalid pattern mode");
    _poke32(JESD_MGT_TEST_CONTROL, it->second);
}

void b300_jesd_core::adjust_tx_phy(
    uint32_t tx_driver_swing, uint32_t tx_precursor, uint32_t tx_postcursor)
{
    if (_tx_driver_swing == tx_driver_swing && _tx_precursor == tx_precursor
        && _tx_postcursor == tx_postcursor) {
        return; // No change in PHY settings
    }
    _tx_driver_swing = tx_driver_swing;
    _tx_precursor    = tx_precursor;
    _tx_postcursor   = tx_postcursor;
    uint32_t reg_val = ((_tx_driver_swing & 0x0F) << 16) | ((_tx_precursor & 0x1F) << 8)
                       | ((_tx_postcursor & 0x1F) << 0);
    _poke32(MGT_TX_TRANSCEIVER_CONTROL, reg_val);
}

void b300_jesd_core::set_drp_target(const jesd_drp_target_t drp_target, size_t dev_num)
{
    int MAX_MGTS     = 4;
    int drp_ch_sel   = (drp_target == MGT) ? dev_num : dev_num + MAX_MGTS;
    uint32_t reg_val = (1 << drp_ch_sel) | (1 << 16);
    _poke32(JESD_MGT_DRP_CONTROL, reg_val);
}

void b300_jesd_core::disable_drp_target()
{
    _poke32(JESD_MGT_DRP_CONTROL, 0x0);
}

uint32_t b300_jesd_core::drp_access(bool rd, uint32_t addr, uint32_t wr_data)
{
    if ((_peek32(JESD_MGT_DRP_CONTROL) & (1 << 20)) != 0) {
        throw uhd::runtime_error(
            "MGT/QPLL DRP Port is reporting busy during an attempted access.");
    }
    uint32_t core_offset = 0x2800 + (addr << 2);
    if (rd) {
        return _peek32(core_offset);
    } else {
        _poke32(core_offset, wr_data);
        if (_peek32(core_offset) != wr_data) {
            throw uhd::runtime_error("DRP read after write failed to match!");
        }
        return 0;
    }
}

void b300_jesd_core::_gt_reset(const std::string& tx_or_rx, bool reset_only)
{
    uint32_t mgt_reg = (tx_or_rx == "tx") ? MGT_TX_RESET_CONTROL : MGT_RX_RESET_CONTROL;
    _poke32(mgt_reg, 0x10);
    if (!reset_only) {
        _poke32(mgt_reg, 0x20);
        uint32_t rb = 0;
        for (size_t i = 0; i < 20; ++i) {
            rb = _peek32(mgt_reg);
            if ((rb & 0xFFFF0000) == 0x00030000)
                return;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        throw uhd::runtime_error("Timeout in GT Reset");
    }
}

void b300_jesd_core::_gt_pll_power_control()
{
    uint32_t reg_val    = 0xFFFF000F;
    uint32_t reg_val_on = 0x0;
    for (size_t x = 0; x < _num_qplls; ++x)
        reg_val_on |= (1 << x);
    for (size_t y = 16; y < 16 + _num_cplls; ++y)
        reg_val_on |= (1 << y);
    reg_val ^= reg_val_on;
    _poke32(MGT_PLL_POWER_DOWN_CONTROL, reg_val);
}

void b300_jesd_core::_gt_pll_lock_control(bool reset_only)
{
    uint32_t reg_val = 0x1111;
    _poke32(MGT_QPLL_CONTROL, reg_val);
    if (!reset_only && _num_qplls > 0) {
        uint32_t reg_val_on = 0x0;
        for (size_t nibble = 0; nibble < _num_qplls; ++nibble)
            reg_val_on |= (1 << (nibble * 4));
        reg_val ^= reg_val_on;
        _poke32(MGT_QPLL_CONTROL, reg_val);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        // Clear all QPLL sticky bits
        _poke32(MGT_QPLL_CONTROL, 1 << 16);
        uint32_t rb      = _peek32(MGT_QPLL_CONTROL);
        uint32_t rb_mask = 0x0, locked_val = 0x0;
        for (size_t nibble = 0; nibble < _num_qplls; ++nibble) {
            if ((rb & (0xF << (nibble * 4))) != (uint32_t(0x2) << (nibble * 4))) {
                UHD_LOG_WARNING("B300", "GT QPLL " << nibble << " failed to lock!");
            }
            locked_val |= (0x2 << (nibble * 4));
            rb_mask |= (0xF << (nibble * 4));
        }
        if ((rb & rb_mask) != locked_val) {
            throw uhd::runtime_error("One or more GT QPLLs failed to lock!");
        }
    }
    reg_val = 0x1111;
    _poke32(MGT_CPLL_CONTROL1, reg_val);
    if (!reset_only && _num_cplls > 0) {
        uint32_t reg_val_on = 0x0;
        for (size_t nibble = 0; nibble < _num_cplls; ++nibble)
            reg_val_on |= (1 << (nibble * 4));
        reg_val ^= reg_val_on;
        _poke32(MGT_CPLL_CONTROL1, reg_val);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        // Clear all CPLL sticky bits
        _poke32(MGT_CPLL_CONTROL3, 1 << 16);
        uint32_t rb      = _peek32(MGT_CPLL_CONTROL1);
        uint32_t rb_mask = 0x0, locked_val = 0x0;
        for (size_t nibble = 0; nibble < _num_cplls; ++nibble) {
            if ((rb & (0xF << (nibble * 4))) != (uint32_t(0x2) << (nibble * 4))) {
                UHD_LOG_WARNING("B300", "GT CPLL " << nibble << " failed to lock!");
            }
            locked_val |= (0x2 << (nibble * 4));
            rb_mask |= (0xF << (nibble * 4));
        }
        if ((rb & rb_mask) != locked_val) {
            throw uhd::runtime_error("One or more GT CPLLs failed to lock!");
        }
    }
}
