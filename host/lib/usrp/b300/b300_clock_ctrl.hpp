//
// Copyright 2025 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include "b300_regs.hpp"
#include <uhd/types/serial.hpp>
#include <uhd/utils/noncopyable.hpp>
#include <uhdlib/usrp/common/lmk05318.hpp>
#include <functional>
#include <memory>

namespace uhd { namespace usrp { namespace b300 {

enum vcxo_sel_t { VCXO_122p88MHz = 0, VCXO_125MHz = 1 };
enum clkin_sel_t { CLKin0 = 0, CLKin1 = 1, CLKin2 = 2 };

// Allowable external reference clock frequencies
static const std::vector<double> EXTERNAL_FREQ_OPTIONS{10e6, 122.88e6, 125e6};

class b300_clock_ctrl : uhd::noncopyable
{
public:
    using sptr                 = std::shared_ptr<b300_clock_ctrl>;
    virtual ~b300_clock_ctrl() = default;

    using poke_fn_t = std::function<void(uint32_t, uint32_t)>;

    static sptr make(uhd::usrp::b300::bar0_regmap_t::sptr bar0_regmap,
        uhd::spi_iface::sptr spi,
        double ext_clock_rate,
        uint16_t board_rev);

    // Core configuration API
    virtual void init()                                        = 0;
    virtual void reset_lmk04832(bool value, bool hard = false) = 0;
    virtual void set_vcxo(vcxo_sel_t vcxo_sel)                 = 0;
    virtual void config_lmk04832(double master_clock_rate)     = 0;
    virtual void set_lmk04832_clock_in(clkin_sel_t clkin_sel)  = 0;
    virtual void config_lmk05318()                             = 0;
    virtual void power_down_lmk04832_sysref()                  = 0;
    virtual void config_lmk04832_for_sync()                    = 0;
    virtual void finish_lmk04832_sync()                        = 0;
    virtual void set_ext_clk_rate(const double ext_clk_rate)   = 0;

    // Query API
    virtual bool get_ref_locked() const                                             = 0;
    virtual bool get_ref_stable() const                                             = 0;
    virtual bool get_dpll_locked(lmk05318_iface::dpll_lock_check_t check =
                                     lmk05318_iface::dpll_lock_check_t::BOTH) const = 0;
    virtual bool wait_for_ref_locked(uint32_t timeout_ms) const                     = 0;

    virtual double get_master_clock_rate(void)        = 0;
    virtual bool validate_lmk05318_priref(void) const = 0;
};

}}} // namespace uhd::usrp::b300
