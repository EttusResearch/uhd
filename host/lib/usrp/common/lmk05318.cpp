//
// Copyright 2025 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// This class also has an MPM python counterpart in mpm/python/usrp_mpm/chips/lmk05318.py
// If any changes are needed in this file, consider if making the change in the python
// counterpart is also necessary.

#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/usrp/common/lmk05318.hpp>
#include <chrono>
#include <thread>

namespace {
constexpr uint16_t LMK_VENDOR_ID = 0x100B;
constexpr uint8_t LMK_PROD_ID    = 0x35;
} // namespace

class lmk05318_impl : public lmk05318_iface
{
public:
    lmk05318_impl(write_fn_t&& poke8, read_fn_t&& peek8)
        : _poke8(std::move(poke8)), _peek8(std::move(peek8))
    {
    }

    void pokes8(const std::vector<std::pair<uint16_t, uint8_t>>& addr_val_pair) final
    {
        for (const auto& pair : addr_val_pair) {
            poke8(pair.first, pair.second);
        }
    }

    void poke8(uint16_t addr, uint8_t val, bool overwrite_mask = false) final
    {
        // TI LMK UserGuide chapter 9.5.5 states that some register require bit masks
        // to be applied to bits to avoid writing to them
        // mask is in the form that a 1 means that the bit shall not be modified. In order
        // to write to the address without the mask applied, the overwrite_mask parameter
        // can be set to True.
        if (!overwrite_mask) {
            uint8_t mask    = 0;
            bool apply_mask = false;

            if (addr == 0x0C) {
                mask       = 0xA7;
                apply_mask = true;
            } else if (addr == 0x9D) {
                mask       = 0xFF;
                apply_mask = true;
            } else if (addr == 0xA4) {
                mask       = 0xFF;
                apply_mask = true;
            } else if (addr >= 0x161 && addr <= 0x1B2) {
                mask       = 0xFF;
                apply_mask = true;
            }

            if (apply_mask) {
                const uint8_t current_val = _peek8(addr);
                val                       = val & ~mask;
                val                       = val | current_val;
                UHD_LOG_TRACE("LMK05318",
                    "Attention: writing to register 0x"
                        << std::hex << addr << " with masked bits, mask 0x" << std::hex
                        << int(mask) << " was applied, resulting in value 0x" << std::hex
                        << int(val));
            }
        }
        _poke8(addr, val);
    }

    uint8_t peek8(uint16_t addr) final
    {
        return _peek8(addr);
    }

    uint16_t get_vendor_id() final
    {
        const uint8_t vendor_id_high = _peek8(0x00);
        const uint8_t vendor_id_low  = _peek8(0x01);
        const uint16_t vendor_id     = (vendor_id_high << 8) | vendor_id_low;
        return vendor_id;
    }

    uint8_t get_product_id() final
    {
        return _peek8(0x02);
    }

    bool verify_chip_id() final
    {
        const uint16_t vendor_id = get_vendor_id();
        const uint8_t prod_id    = get_product_id();
        if (vendor_id != LMK_VENDOR_ID) {
            UHD_LOG_ERROR("LMK05318", "Wrong Vendor ID 0x" << std::hex << vendor_id);
            return false;
        }
        if (prod_id != LMK_PROD_ID) {
            UHD_LOG_ERROR("LMK05318", "Wrong Product ID 0x" << std::hex << prod_id);
            return false;
        }
        return true;
    }

    void soft_reset(bool value = true) final
    {
        constexpr uint16_t reset_addr = 0x0C;
        uint8_t reset_byte;
        if (value) {
            reset_byte = 0x80;
        } else {
            reset_byte = 0x7F & _peek8(reset_addr);
        }
        poke8(reset_addr, reset_byte, true);
    }

    void write_cfg_regs_to_eeprom(eeprom_method_t method) final
    {
        if (method == eeprom_method_t::REGISTER_COMMIT) {
            UHD_LOG_TRACE("LMK05318", "write current device register content to EEPROM");

            // Store current cfg to SRAM
            poke8(0x9D, 0x40, true);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            // Unlock EEPROM
            poke8(0xA4, 0xEA, true);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            // Store SRAM into EEPROM
            poke8(0x9D, 0x03, true);

            // The actual programming takes about 230ms, poll the busy bit to see when
            // it's done
            if (!_wait_for_busy(true)) {
                UHD_LOG_ERROR("LMK05318",
                    "EEPROM does not start programming, something went wrong");
            }
            if (!_wait_for_busy(false)) {
                UHD_LOG_ERROR("LMK05318",
                    "EEPROM is still busy after programming, something went wrong");
            }

        } else if (method == eeprom_method_t::DIRECT_WRITE) {
            throw uhd::runtime_error(
                "direct LMK05318 EEPROM programming not implemented");
        } else {
            throw uhd::runtime_error("Invalid method for LMK05318 EEPROM programming");
        }

        // Lock EEPROM
        poke8(0xA4, 0x00, true);
        UHD_LOG_TRACE("LMK05318",
            "programming EEPROM done, power-cycle or hard-reset to take effect");
    }

    void write_eeprom_to_cfg_regs() final
    {
        poke8(0x9D, 0x08, true);
    }

    uint8_t get_eeprom_prog_cycles() final
    {
        // Note: the actual counter only increases after programming AND
        // power-cycle/hard-reset so multiple programming cycles without power cycle will
        // lead to wrong counter values
        return _peek8(0x9C);
    }

    bool get_dpll_lock(dpll_lock_check_t check = BOTH) final
    {
        // DPLL Status is read from register 0xE. Bit 7 is Loss of Phase Lock, Bit 6 is
        // Loss of Frequency Lock. When both bits are 0, the DPLL is locked.
        const uint8_t dpll_status  = _peek8(0xE);
        const bool phase_lock_loss = (dpll_status >> 7) & 1;
        const bool freq_lock_loss  = (dpll_status >> 6) & 1;
        switch (check) {
            case FREQ_LOCK:
                return !freq_lock_loss;
            case PHASE_LOCK:
                return !phase_lock_loss;
            case BOTH:
                return !phase_lock_loss && !freq_lock_loss;
            default:
                return false;
        }
    }

private:
    write_fn_t _poke8;
    read_fn_t _peek8;

    bool _wait_for_busy(uint8_t expected_value)
    {
        auto wait_until = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        do {
            // check if busy bit is cleared
            uint8_t busy = (_peek8(0x9D) >> 2) & 1;
            if (busy == expected_value) {
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        } while (std::chrono::steady_clock::now() < wait_until);
        return false;
    }
};

lmk05318_iface::sptr lmk05318_iface::make(write_fn_t&& poke8, read_fn_t&& peek8)
{
    return std::make_shared<lmk05318_impl>(std::move(poke8), std::move(peek8));
}
