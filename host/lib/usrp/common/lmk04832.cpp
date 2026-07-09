//
// Copyright 2025 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/utils/log.hpp>
#include <uhdlib/usrp/common/lmk04832.hpp>
#include <cassert>
#include <chrono>
#include <thread>

namespace {
constexpr char LOG_ID[]                    = "_LMK04832";
constexpr uint8_t LMK_CHIP_ID              = 6;
constexpr uint16_t LMK_PROD_ID             = 0xD163;
constexpr uint8_t LMK_RB_PLL1_LD_LOST_MASK = 0b1000;
constexpr uint8_t LMK_RB_PLL1_LD_MASK      = 0b0100;
constexpr uint8_t LMK_RB_PLL2_LD_LOST_MASK = 0b0010;
constexpr uint8_t LMK_RB_PLL2_LD_MASK      = 0b0001;
} // namespace

class lmk04832_impl : public lmk04832_iface
{
public:
    lmk04832_impl(write_fn_t&& poke8, read_fn_t&& peek8, std::string log_prefix)
        : _poke8(std::move(poke8))
        , _peek8(std::move(peek8))
        , _enable_3wire_spi(false)
        , _log_id(std::move(log_prefix) + std::string(LOG_ID))
    {
    }

    void pokes8(const std::vector<std::pair<uint16_t, uint8_t>>& addr_val_pair) override
    {
        for (const auto& pair : addr_val_pair) {
            _poke8(pair.first, pair.second);
        }
    }

    uint8_t get_chip_id() override
    {
        uint8_t chip_id = _peek8(0x03);
        UHD_LOG_TRACE(_log_id, "Chip ID Readback: " << int(chip_id));
        return chip_id;
    }

    uint16_t get_product_id() override
    {
        uint8_t prod_id_0 = _peek8(0x04);
        uint8_t prod_id_1 = _peek8(0x05);
        uint16_t prod_id  = (uint16_t(prod_id_1) << 8) | prod_id_0;
        UHD_LOG_TRACE(_log_id, "Product ID Readback: 0x" << std::hex << prod_id);
        return prod_id;
    }

    bool verify_chip_id() override
    {
        uint8_t chip_id  = get_chip_id();
        uint16_t prod_id = get_product_id();
        if (chip_id != LMK_CHIP_ID) {
            UHD_LOG_ERROR(_log_id, "Wrong Chip ID 0x" << std::hex << int(chip_id));
            return false;
        }
        if (prod_id != LMK_PROD_ID) {
            UHD_LOG_ERROR(_log_id, "Wrong Product ID 0x" << std::hex << prod_id);
            return false;
        }
        return true;
    }

    void enable_4wire_spi() override
    {
        _poke8(0x148, 0x33);
        _enable_3wire_spi = false;
    }

    void clear_ldl(lmk04832_pll_sel pll = lmk04832_pll_sel::BOTH) override
    {
        uint8_t clear_val1 =
            (pll == lmk04832_pll_sel::BOTH || pll == lmk04832_pll_sel::PLL1) ? 0b10 : 0;
        uint8_t clear_val2 =
            (pll == lmk04832_pll_sel::BOTH || pll == lmk04832_pll_sel::PLL2) ? 0b01 : 0;
        _poke8(0x182, clear_val1 | clear_val2);
        _poke8(0x182, 0);
    }

    bool check_plls_locked(
        lmk04832_pll_sel pll = lmk04832_pll_sel::BOTH, bool sticky = false) override
    {
        bool result             = true;
        bool clear_ldl_status   = false;
        uint8_t pll_lock_status = _peek8(0x183);
        bool pll1_ld            = pll_lock_status & LMK_RB_PLL1_LD_MASK;
        bool pll1_ld_lost       = pll_lock_status & LMK_RB_PLL1_LD_LOST_MASK;
        bool pll2_ld            = pll_lock_status & LMK_RB_PLL2_LD_MASK;
        bool pll2_ld_lost       = pll_lock_status & LMK_RB_PLL2_LD_LOST_MASK;

        if (pll == lmk04832_pll_sel::BOTH || pll == lmk04832_pll_sel::PLL1) {
            result           = result && pll1_ld && (!sticky || !pll1_ld_lost);
            clear_ldl_status = clear_ldl_status || (sticky && pll1_ld_lost);
        }
        if (pll == lmk04832_pll_sel::BOTH || pll == lmk04832_pll_sel::PLL2) {
            result           = result && pll2_ld && (!sticky || !pll2_ld_lost);
            clear_ldl_status = clear_ldl_status || (sticky && pll2_ld_lost);
        }

        if (!result) {
            UHD_LOG_DEBUG(_log_id,
                "Lock loss reported: (PLL1 LD: "
                    << pll1_ld << ", PLL1 LD Lost: " << pll1_ld_lost << ", PLL2 LD: "
                    << pll2_ld << ", PLL2 LD Lost: " << pll2_ld_lost << ")");
        }

        if (clear_ldl_status) {
            clear_ldl(pll);
        }
        return result;
    }

    bool wait_for_pll_lock(const lmk04832_pll_sel pll = lmk04832_pll_sel::BOTH,
        uint32_t timeout                              = 2000) override
    {
        clear_ldl();

        // Now poll lock status until timeout.
        auto end_time =
            std::chrono::steady_clock::now() + std::chrono::milliseconds(int(timeout));
        while (std::chrono::steady_clock::now() < end_time) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (check_plls_locked(pll)) {
                return true;
            }
        }
        UHD_LOG_DEBUG(
            _log_id, pll << " not reporting locked after " << timeout << " ms wait");
        return false;
    }

    void soft_reset(bool value = true) override
    {
        uint8_t reset_byte = value ? 0x80 : 0x00;
        if (!_enable_3wire_spi) {
            reset_byte |= 0x10;
        }
        _poke8(0, reset_byte);
    }

    void init_pll1_r_divider_sync() override
    {
        // Run PLL1 R Divider sync according to
        // http://www.ti.com/lit/ds/snas688c/snas688c.pdf chapter 8.3.1.1.

        // Rising edge on sync pin is done by an callback which has to return its success
        // state. If the sync callback was successful, returns PLL1 lock state as overall
        // success otherwise the method fails.

        // 1) Setup device for synchronizing PLL1 R.
        // PLL1R_SYNC_EN    (6) = 1
        // PLL1R_SYNC_SRC (5,4) = Sync pin
        // PLL2R_SYNC_EN    (3) = 0
        _poke8(0x145, 0x50);

        // Do NOT change clkin0_TYPE and Clkin[0,1]_DEMUX.
        // Both are set in initialization and remain static.

        // 2) Arm PLL1 R divider for synchronization.
        _poke8(0x177, 0x20);
        _poke8(0x177, 0);
        // 3) Wait for the writes to complete by triggering a read.
        get_chip_id();
        // Caller now needs to send a rising edge on the sync port.
    }

    bool deinit_pll1_r_divider_sync() override
    {
        // 1) reset 0x145 to safe value (no sync enable set, sync src invalidated)
        _poke8(0x145, 0);

        // 2) wait for PLL1 to lock
        return wait_for_pll_lock(lmk04832_pll_sel::PLL1);
    }

    uint8_t pll2_pre_to_reg(uint8_t prescaler,
        uint8_t osc_field = 0x01,
        uint8_t xtal_en   = 0x0,
        uint8_t ref_2x_en = 0x0) override
    {
        assert(prescaler >= 2 && prescaler <= 8);
        uint8_t reg_val = ((prescaler & 0x07) << 5) | ((osc_field & 0x3) << 2)
                          | ((xtal_en & 0x1) << 1) | ((ref_2x_en & 0x1) << 0);
        UHD_LOG_TRACE(_log_id,
            "From prescaler value " << int(prescaler) << ", writing register as 0x"
                                    << std::hex << int(reg_val));
        return reg_val;
    }

private:
    write_fn_t _poke8;
    read_fn_t _peek8;
    bool _enable_3wire_spi;
    std::string _log_id;
};

lmk04832_iface::sptr lmk04832_iface::make(
    write_fn_t&& poke8, read_fn_t&& peek8, std::string log_prefix)
{
    return std::make_shared<lmk04832_impl>(
        std::move(poke8), std::move(peek8), std::move(log_prefix));
}
