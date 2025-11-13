//
// Copyright 2025 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

//! Control interface for an LMK05318 clock chip
class lmk05318_iface
{
public:
    using sptr = std::shared_ptr<lmk05318_iface>;

    virtual ~lmk05318_iface() = default;

    enum eeprom_method_t { REGISTER_COMMIT, DIRECT_WRITE };
    enum dpll_lock_check_t { FREQ_LOCK, PHASE_LOCK, BOTH };

    //! Write functor: Take address / data pair, craft SPI transaction
    using write_fn_t = std::function<void(uint16_t, uint8_t)>;

    //! Read functor: Return value given address
    using read_fn_t = std::function<uint8_t(uint16_t)>;

    static sptr make(write_fn_t&& poke8, read_fn_t&& peek8);

    //! Apply a series of pokes.
    virtual void pokes8(
        const std::vector<std::pair<uint16_t, uint8_t>>& addr_val_pair) = 0;
    //! Write to a register with optional mask protection
    virtual void poke8(uint16_t addr, uint8_t val, bool overwrite_mask = false) = 0;
    //! Read from a register
    virtual uint8_t peek8(uint16_t addr) = 0;
    //! Read back the vendor ID.
    virtual uint16_t get_vendor_id() = 0;
    //! Read back the product ID.
    virtual uint8_t get_product_id() = 0;
    //! Returns True if the vendor ID and product ID matches what we expect.
    virtual bool verify_chip_id() = 0;
    //! Performs a soft reset of the LMK05318 by setting or unsetting the reset register.
    virtual void soft_reset(bool value = true) = 0;
    //! Program the current register config to LMK eeprom
    virtual void write_cfg_regs_to_eeprom(eeprom_method_t method) = 0;
    //! Read register config from eeprom and store it into registers
    virtual void write_eeprom_to_cfg_regs() = 0;
    //! Returns the number of eeprom programming cycles
    virtual uint8_t get_eeprom_prog_cycles() = 0;
    //! Returns True if the DPLL has not lost its lock.
    virtual bool get_dpll_lock(dpll_lock_check_t check = BOTH) = 0;
};
