//
// Copyright 2025 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace uhd { namespace usrp { namespace b300 {

enum jesd_drp_target_t { MGT = 0, QPLL = 1 };

class b300_jesd_core
{
public:
    //! Write functor: Take address / data pair, craft SPI transaction
    using write_fn_t = std::function<void(uint32_t, uint32_t)>;

    //! Read functor: Return value given address
    using read_fn_t = std::function<uint32_t(uint32_t)>;

    b300_jesd_core(write_fn_t&& poke32,
        read_fn_t&& peek32,
        size_t num_qplls,
        size_t num_cplls,
        uint8_t lmfc_divider,
        uint8_t rx_sysref_delay,
        uint8_t tx_sysref_delay);

    using sptr = std::shared_ptr<b300_jesd_core>;

    void reset();
    void init_deframer(bool bypass_descrambler);
    void init_framer(bool bypass_scrambler);
    bool get_framer_status();
    bool get_deframer_status(bool ignore_sysref = false);
    void init();
    void enable_lmfc(bool enable = false);
    void reset_lmfc();
    void send_sysref_pulse();
    void set_pattern_gen(const std::string& mode);
    void adjust_tx_phy(
        uint32_t tx_driver_swing, uint32_t tx_precursor, uint32_t tx_postcursor);
    void set_drp_target(const jesd_drp_target_t drp_target, size_t dev_num);
    void disable_drp_target();
    uint32_t drp_access(bool rd = true, uint32_t addr = 0, uint32_t wr_data = 0);

private:
    void _gt_reset(const std::string& tx_or_rx, bool reset_only = false);
    void _gt_pll_power_control();
    void _gt_pll_lock_control(bool reset_only = false);
    void _gt_pma_eyescan(bool enable = false);

    write_fn_t _poke32;
    read_fn_t _peek32;
    size_t _num_qplls;
    size_t _num_cplls;
    uint8_t _lmfc_divider;
    uint8_t _rx_sysref_delay;
    uint8_t _tx_sysref_delay;
    uint32_t _tx_driver_swing = 0b1000;
    uint32_t _tx_precursor    = 0b00000;
    uint32_t _tx_postcursor   = 0b00000;
};

}}} // namespace uhd::usrp::b300
