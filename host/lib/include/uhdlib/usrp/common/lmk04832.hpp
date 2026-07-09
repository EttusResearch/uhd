//
// Copyright 2025 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/types/serial.hpp>
#include <uhd/utils/log.hpp>
#include <functional>
#include <memory>
#include <vector>

static constexpr double LMK04832_VCO0_RANGE_MIN = 2440e6;
static constexpr double LMK04832_VCO0_RANGE_MAX = 2580e6;
static constexpr double LMK04832_VCO1_RANGE_MIN = 2945e6;
static constexpr double LMK04832_VCO1_RANGE_MAX = 3255e6;

//! Control interface for an LMK04832 clock chip
class lmk04832_iface
{
public:
    using sptr = std::shared_ptr<lmk04832_iface>;

    virtual ~lmk04832_iface() = default;

    enum lmk04832_pll_sel { PLL1, PLL2, BOTH };

    //! Write functor: Take address / data pair, craft SPI transaction
    using write_fn_t = std::function<void(uint16_t, uint8_t)>;

    //! Read functor: Return value given address
    using read_fn_t = std::function<uint8_t(uint16_t)>;

    static sptr make(write_fn_t&& poke8, read_fn_t&& peek8, std::string log_prefix);

    //! Apply a series of pokes.
    virtual void pokes8(
        const std::vector<std::pair<uint16_t, uint8_t>>& addr_val_pair) = 0;
    //! Read back the chip ID.
    virtual uint8_t get_chip_id() = 0;
    //! Read back the product ID.
    virtual uint16_t get_product_id() = 0;
    //! Returns True if the chip ID and product ID matches what we expect, False
    //! otherwise.
    virtual bool verify_chip_id() = 0;
    //! Enable 4-wire SPI readback from the CLKin_SEL0 pin.
    virtual void enable_4wire_spi() = 0;
    //! Clear the LD_LOST flags for the specified PLL(s).
    virtual void clear_ldl(lmk04832_pll_sel pll = lmk04832_pll_sel::BOTH) = 0;
    //! Returns True if the specified PLLs are locked, False otherwise.
    //! Note: By default, this checks the current status only, it does not check
    //! the sticky lock-detect bits (i.e., we only check RB_PLL1_LD and/or
    //! RB_PLL2_LD).
    //!
    //! From the datasheet (Section 8.6.2.9.3), register 0x183 bits:
    //! 3: RB_PLL1_LD_LOST. This is set when PLL1 DLD edge falls. Does not set
    //!    if cleared while PLL1 DLD.
    //! 2: RB_PLL1_LD. Read back 0: PLL1 DLD is low. Read back 1: PLL1 DLD is
    //!    high.
    //! 1: RB_PLL2_LD_LOST. This is set when PLL2 DLD edge falls. Does not set
    //!    if cleared while PLL2 DLD.
    //! 0: RB_PLL2_LD. Read back 0: PLL2 DLD is low. Read back 1: PLL2 DLD is
    //!    high.
    //!
    //! The *_LD_LOST bits get cleared by writing to register 0x182 (see
    //! clear_ldl()). When sticky==true, implementations may clear any set
    //! *_LD_LOST bits as part of this call.
    virtual bool check_plls_locked(
        lmk04832_pll_sel pll = lmk04832_pll_sel::BOTH, bool sticky = false) = 0;
    //! Waits for the PLL(s) to lock. Returns False if the PLL(s) do not lock before the
    //! timeout (in ms).
    virtual bool wait_for_pll_lock(
        const lmk04832_pll_sel pll = lmk04832_pll_sel::BOTH, uint32_t timeout = 2000) = 0;
    //! Performs a soft reset of the LMK04832 by setting or unsetting the reset register.
    virtual void soft_reset(bool value = true) = 0;
    //! Initialize PLL1 R Divider sync.
    virtual void init_pll1_r_divider_sync() = 0;
    //! Deinitialize PLL1 R Divider sync.
    virtual bool deinit_pll1_r_divider_sync() = 0;
    //! From the prescaler value, returns the register value combined with the other
    //! register fields.
    virtual uint8_t pll2_pre_to_reg(uint8_t prescaler,
        uint8_t osc_field = 0x01,
        uint8_t xtal_en   = 0x0,
        uint8_t ref_2x_en = 0x0) = 0;
};
