//
// Copyright 2018, 2017 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "lmx2592_regs.hpp"
#include <uhdlib/usrp/common/lmx2592.hpp>
#include <uhdlib/utils/narrow.hpp>
#include <boost/math/common_factor_rt.hpp>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>

using namespace uhd;

namespace {
constexpr double LMX2592_DOUBLER_MAX_REF_FREQ = 60e6;
constexpr double LMX2592_MAX_FREQ_PFD = 125e6;

constexpr double LMX2592_MIN_REF_FREQ = 5e6;
constexpr double LMX2592_MAX_REF_FREQ = 1400e6;

constexpr double LMX2592_MAX_OUT_FREQ = 9.8e9;
constexpr double LMX2592_MIN_OUT_FREQ = 20e6;

constexpr double LMX2592_MIN_VCO_FREQ = 3.55e9;
constexpr double LMX2592_MAX_VCO_FREQ = 7.1e9;

constexpr double LMX2592_MAX_DOUBLER_INPUT_FREQ = 200e6;
constexpr double LMX2592_MAX_MULT_OUT_FREQ = 250e6;
constexpr double LMX2592_MAX_MULT_INPUT_FREQ = 70e6;
constexpr double LMX2592_MAX_POSTR_DIV_OUT_FREQ = 125e6;

constexpr int MAX_N_DIVIDER = 4095;

constexpr int MAX_MASH_ORDER = 4;
constexpr std::array<int, MAX_MASH_ORDER + 1> LMX2592_MIN_N_DIV = {
    9, 11, 16, 18, 30
}; // includes int-N

constexpr int NUM_DIVIDERS = 14;
constexpr std::array<int, NUM_DIVIDERS> LMX2592_CHDIV_DIVIDERS = { 1,  2,  3,  4,  6,  8,   12,
                                                                   16, 24, 32, 64, 96, 128, 192 };
const std::array<double, NUM_DIVIDERS> LMX2592_CHDIV_MIN_FREQ = {
    3550e6,   1775e6,   1183.33e6, 887.5e6, 591.67e6, 443.75e6, 295.83e6,
    221.88e6, 147.92e6, 110.94e6,  55.47e6, 36.98e6,  27.73e6,  20e6
};
constexpr std::array<double, NUM_DIVIDERS> LMX2592_CHDIV_MAX_FREQ = {
    6000e6,   3550.0e6, 2366.67e6, 1775.00e6, 1183.33, 887.50e6, 591.67e6,
    443.75e6, 295.83e6, 221.88e6,  110.94e6,  73.96e6, 55.47e6,  36.98
};
constexpr int NUM_CHDIV_STAGES = 3;
constexpr std::array<std::array<int, NUM_CHDIV_STAGES>, NUM_DIVIDERS> LMX2592_CHDIV_SEGS = {
    { { 1, 1, 1 },
      { 2, 1, 1 },
      { 3, 1, 1 },
      { 2, 2, 1 },
      { 3, 2, 1 },
      { 2, 4, 1 },
      { 2, 6, 1 },
      { 2, 8, 1 },
      { 3, 8, 1 },
      { 2, 8, 2 },
      { 2, 8, 4 },
      { 2, 8, 6 },
      { 2, 8, 8 },
      { 3, 8, 8 } }
};

constexpr int SPI_ADDR_SHIFT = 16;
constexpr int SPI_ADDR_MASK = 0x7f;
constexpr int SPI_READ_FLAG = 1 << 23;
}

class lmx2592_impl : public lmx2592_iface {
public:
    explicit lmx2592_impl(write_spi_t write_fn, read_spi_t read_fn)
        : _write_fn([write_fn](const uint8_t addr, const uint16_t data) {
              const uint32_t spi_transaction =
                  0 | ((addr & SPI_ADDR_MASK) << SPI_ADDR_SHIFT) | data;
              write_fn(spi_transaction);
          }),
          _read_fn([read_fn](const uint8_t addr) {
              const uint32_t spi_transaction =
                  SPI_READ_FLAG | ((addr & SPI_ADDR_MASK) << SPI_ADDR_SHIFT);
              return read_fn(spi_transaction);
          }),
          _regs(),
          _rewrite_regs(true) {
        UHD_LOG_TRACE("LMX2592", "Initializing Synthesizer");

        // Soft Reset
        _regs.reset = 1;
        UHD_LOG_TRACE("LMX2592", "Resetting LMX");
        _write_fn(_regs.ADDR_R0, _regs.get_reg(_regs.ADDR_R0));

        // The bit is cleared on the synth during the reset
        _regs.reset = 0;

        // Enable SPI Readback
        _regs.muxout_sel = lmx2592_regs_t::muxout_sel_t::MUXOUT_SEL_READBACK;
        UHD_LOG_TRACE("LMX2592", "Enabling SPI Readback");
        _write_fn(_regs.ADDR_R0, _regs.get_reg(_regs.ADDR_R0));

        // Test Write/Read
        const auto random_number = // Derived from current time
            static_cast<uint16_t>(
                std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) & 0x0FFF);
        _write_fn(_regs.ADDR_R40, random_number);
        const auto readback = _read_fn(_regs.ADDR_R40);
        if (readback == random_number) {
            UHD_LOG_TRACE("LMX2592", "Register loopback test passed");
        } else {
            throw runtime_error(
                str(boost::format(
                        "LMX2592 register loopback test failed. Expected 0x%04X, Read 0x%04X") %
                    random_number % readback));
        }

        // Set register values where driver defaults differ from the datasheet values
        _regs.acal_enable = 0;
        _regs.fcal_enable = 0;
        _regs.cal_clk_div = 0;
        _regs.vco_idac_ovr = 1;
        _regs.cp_idn = 12;
        _regs.cp_iup = 12;
        _regs.vco_idac = 350;
        _regs.mash_ditherer = 1;
        _regs.outa_mux = lmx2592_regs_t::outa_mux_t::OUTA_MUX_VCO;
        _regs.fcal_fast = 1;

        // Write default register values, ensures register copy is synchronized
        _rewrite_regs = true;
        commit();

        _regs.fcal_enable = 1;
        commit();
    }

    ~lmx2592_impl() override { UHD_SAFE_CALL(_regs.powerdown = 1; commit();) }

    double set_frequency(const double target_freq) override {

        // Enforce LMX frequency limits
        if (target_freq < LMX2592_MIN_OUT_FREQ or target_freq > LMX2592_MAX_OUT_FREQ) {
            throw runtime_error("Requested frequency is out of the supported range");
        }

        // Find the largest possible divider
        auto output_divider_index = 0;
        for (auto limit : LMX2592_CHDIV_MIN_FREQ) {
            // The second harmonic level is very bad when using the div-by-3
            // Skip and let the div-by-4 cover the range
            if (LMX2592_CHDIV_DIVIDERS[output_divider_index] == 3) {
                output_divider_index++;
                continue;
            }
            if (target_freq < limit) {
                output_divider_index++;
            } else {
                break;
            }
        }
        const auto output_divider = LMX2592_CHDIV_DIVIDERS[output_divider_index];
        _set_chdiv_values(output_divider_index);

        // Setup input signal path and PLL loop
        const int vco_multiplier = target_freq > LMX2592_MAX_VCO_FREQ ? 2 : 1;

        const auto target_vco_freq = target_freq * output_divider;
        const auto core_vco_freq = target_vco_freq / vco_multiplier;

        double input_freq = _ref_freq;

        // Input Doubler stage
        if (input_freq <= LMX2592_MAX_DOUBLER_INPUT_FREQ) {
            _regs.osc_doubler = 1;
            input_freq *= 2;
        } else {
            _regs.osc_doubler = 0;
        }

        // Pre-R divider
        _regs.pll_r_pre =
            narrow_cast<uint16_t>(std::ceil(input_freq / LMX2592_MAX_MULT_INPUT_FREQ));
        input_freq /= _regs.pll_r_pre;

        // Multiplier
        _regs.mult = narrow_cast<uint8_t>(std::floor(LMX2592_MAX_MULT_OUT_FREQ / input_freq));
        input_freq *= _regs.mult;

        // Post R divider
        _regs.pll_r = narrow_cast<uint8_t>(std::ceil(input_freq / LMX2592_MAX_POSTR_DIV_OUT_FREQ));

        // Default to divide by 2, will be increased later if N exceeds it's limit
        int prescaler = 2;
        _regs.pll_n_pre = lmx2592_regs_t::pll_n_pre_t::PLL_N_PRE_DIVIDE_BY_2;

        const int min_n_divider = LMX2592_MIN_N_DIV[_regs.mash_order];
        double pfd_freq = input_freq / _regs.pll_r;
        while (pfd_freq * (prescaler * min_n_divider) / vco_multiplier > core_vco_freq) {
            _regs.pll_r++;
            pfd_freq = input_freq / _regs.pll_r;
        }

        const auto spur_dodging_enable = false;
        const double min_vco_step_size = spur_dodging_enable ? 2e6 : 1;

        auto fden = static_cast<uint32_t>(std::floor(pfd_freq * prescaler / min_vco_step_size));
        _regs.pll_den_lsb = narrow_cast<uint16_t>(fden);
        _regs.pll_den_msb = narrow_cast<uint16_t>(fden >> 16);

        auto mash_seed = static_cast<uint32_t>(fden / 2);
        _regs.mash_seed_lsb = narrow_cast<uint16_t>(mash_seed);
        _regs.mash_seed_msb = narrow_cast<uint16_t>(mash_seed >> 16);

        // Calculate N and Fnum
        const auto N_dot_F = target_vco_freq / (pfd_freq * prescaler);
        auto N = static_cast<uint16_t>(std::floor(N_dot_F));
        if (N > MAX_N_DIVIDER) {
            _regs.pll_n_pre = lmx2592_regs_t::pll_n_pre_t::PLL_N_PRE_DIVIDE_BY_4;
            N /= 2;
        }
        const auto frac = N_dot_F - N;
        const auto fnum = static_cast<uint32_t>(std::round(frac * fden));

        _regs.pll_n = N;
        _regs.pll_num_lsb = narrow_cast<uint16_t>(fnum);
        _regs.pll_num_msb = narrow_cast<uint16_t>(fnum >> 16);

        // Calculate actual Fcore_vco, Fvco, F_lo frequencies
        const auto actual_fvco = pfd_freq * prescaler * (N + double(fnum) / double(fden));
        const auto actual_fcore_vco = actual_fvco / vco_multiplier;
        const auto actual_f_lo = actual_fcore_vco * vco_multiplier / output_divider;

        UHD_LOGGER_TRACE("LMX2592") << "Tuned to " << actual_f_lo;
        commit();

        // Run Frequency Calibration
        _regs.fcal_enable = 1;
        commit();

        UHD_LOGGER_TRACE("LMX2592")
            << "PLL lock status: " << (get_lock_status() ? "Locked" : "Unlocked");

        return actual_f_lo;
    }

    void set_mash_order(const mash_order_t mash_order) override {
        if (mash_order == mash_order_t::INT_N) {
            _regs.mash_order = lmx2592_regs_t::mash_order_t::MASH_ORDER_INT_MODE;

        } else if (mash_order == mash_order_t::FIRST) {
            _regs.mash_order = lmx2592_regs_t::mash_order_t::MASH_ORDER_FIRST;

        } else if (mash_order == mash_order_t::SECOND) {
            _regs.mash_order = lmx2592_regs_t::mash_order_t::MASH_ORDER_SECOND;

        } else if (mash_order == mash_order_t::THIRD) {
            _regs.mash_order = lmx2592_regs_t::mash_order_t::MASH_ORDER_THIRD;

        } else if (mash_order == mash_order_t::FOURTH) {
            _regs.mash_order = lmx2592_regs_t::mash_order_t::MASH_ORDER_FOURTH;
        }
    }

    void set_reference_frequency(const double ref_freq) override {
        if (ref_freq < LMX2592_MIN_REF_FREQ or ref_freq > LMX2592_MAX_REF_FREQ) {
            throw std::runtime_error("Reference frequency is out of bounds for the LMX2592");
        }

        _ref_freq = ref_freq;
    }

    void set_output_power(const output_t output, const unsigned int power) override {
        UHD_LOGGER_TRACE("LMX2592")
            << "Set output: " << (output == RF_OUTPUT_A ? "A" : "B") << " to power " << power;

        const auto MAX_POWER = 63;
        if (power > MAX_POWER) {
            UHD_LOGGER_ERROR("LMX2592")
                << "Requested power level of " << power << " exceeds maximum of " << MAX_POWER;
            return;
        }

        if (output == RF_OUTPUT_A) {
            _regs.outa_power = power;
        } else {
            _regs.outb_power = power;
        }

        commit();
    }

    void set_output_enable(const output_t output, const bool enable) override {
        UHD_LOGGER_TRACE("LMX2592") << "Set output " << (output == RF_OUTPUT_A ? "A" : "B")
                                    << " to " << (enable ? "On" : "Off");

        if (enable) {
            _regs.chdiv_dist_pd = 0;

            if (output == RF_OUTPUT_A) {
                _regs.outa_pd = 0;

            } else {
                _regs.outb_pd = 0;
            }

        } else {
            if (output == RF_OUTPUT_A) {
                _regs.outa_pd = 1;
                _regs.vco_dista_pd = 1;
                _regs.chdiv_dista_en = 0;

            } else {
                _regs.outb_pd = 1;
                _regs.vco_distb_pd = 1;
                _regs.chdiv_distb_en = 0;
            }
        }

        // If both channels are disabled
        if (_regs.outa_pd == 1 and _regs.outb_pd == 1) {
            _regs.chdiv_dist_pd = 1;
        }

        commit();
    }

    bool get_lock_status() override {
        // MUXOUT is shared between Lock Detect and SPI Readback
        _regs.muxout_sel = lmx2592_regs_t::muxout_sel_t::MUXOUT_SEL_LOCK_DETECT;
        commit();

        // The SPI MISO is now being driven by lock detect
        // If the PLL is locked we expect to read 0xFFFF from any read, else 0x0000
        const auto value_read = _read_fn(_regs.ADDR_R0);
        const auto lock_status = (value_read == 0xFFFF);

        UHD_LOG_TRACE(
            "LMX2592",
            str(boost::format("Read Lock status: 0x%04X") % static_cast<unsigned int>(value_read)));

        // Restore ability to read registers
        _regs.muxout_sel = lmx2592_regs_t::muxout_sel_t::MUXOUT_SEL_READBACK;
        commit();

        return lock_status;
    }

    void commit() override {
        UHD_LOGGER_DEBUG("LMX2592")
            << "Storing register cache " << (_rewrite_regs ? "completely" : "selectively")
            << " to LMX via SPI...";
        const auto changed_addrs =
            _rewrite_regs ? _regs.get_all_addrs() : _regs.get_changed_addrs<size_t>();

        for (const auto addr : changed_addrs) {
            _write_fn(addr, _regs.get_reg(addr));
            UHD_LOGGER_TRACE("LMX2592")
                << "Register " << std::setw(2) << static_cast<unsigned int>(addr) << ": 0x"
                << std::hex << std::uppercase << std::setw(4) << std::setfill('0')
                << static_cast<unsigned int>(_regs.get_reg(addr));
        }

        _regs.save_state();
        UHD_LOG_DEBUG("LMX2592",
                      "Writing registers complete: "
                      "Updated "
                          << changed_addrs.size()
                          << " registers.");

        _rewrite_regs = false;
    }

private: // Members
    //! Write functor: Take address / data pair, craft SPI transaction
    using write_fn_t = std::function<void(uint8_t, uint16_t)>;
    //! Read functor: Return value given address
    using read_fn_t = std::function<uint16_t(uint8_t)>;

    write_fn_t _write_fn;
    read_fn_t _read_fn;
    lmx2592_regs_t _regs;
    bool _rewrite_regs;
    double _ref_freq;

    void _set_chdiv_values(const int output_divider_index) {

        // Configure divide segments and mux
        const auto seg1 = LMX2592_CHDIV_SEGS[output_divider_index][0];
        const auto seg2 = LMX2592_CHDIV_SEGS[output_divider_index][1];
        const auto seg3 = LMX2592_CHDIV_SEGS[output_divider_index][2];

        _regs.chdiv_seg_sel = lmx2592_regs_t::chdiv_seg_sel_t::CHDIV_SEG_SEL_POWERDOWN;

        if (seg1 > 1) {
            _regs.chdiv_seg_sel = lmx2592_regs_t::chdiv_seg_sel_t::CHDIV_SEG_SEL_DIV_SEG_1;
            _regs.chdiv_seg1_en = 1;
            _regs.outa_mux = lmx2592_regs_t::outa_mux_t::OUTA_MUX_DIVIDER;
            _regs.outb_mux = lmx2592_regs_t::outb_mux_t::OUTB_MUX_DIVIDER;
            _regs.vco_dista_pd = 1;
            _regs.vco_distb_pd = 1;
            _regs.chdiv_dist_pd = 0;

            if (_regs.outa_pd == 0) {
                _regs.chdiv_dista_en = 1;
            }
            if (_regs.outb_pd == 0) {
                _regs.chdiv_distb_en = 1;
            }

        } else {
            _regs.chdiv_seg1_en = 0;
            _regs.outa_mux = lmx2592_regs_t::outa_mux_t::OUTA_MUX_VCO;
            _regs.outb_mux = lmx2592_regs_t::outb_mux_t::OUTB_MUX_VCO;
            _regs.chdiv_dist_pd = 1;

            if (_regs.outa_pd == 0) {
                _regs.vco_dista_pd = 0;
            }
            if (_regs.outb_pd == 0) {
                _regs.vco_distb_pd = 0;
            }
        }

        if (seg1 == 2) {
            _regs.chdiv_seg1 = lmx2592_regs_t::chdiv_seg1_t::CHDIV_SEG1_DIVIDE_BY_2;
        } else if (seg1 == 3) {
            _regs.chdiv_seg1 = lmx2592_regs_t::chdiv_seg1_t::CHDIV_SEG1_DIVIDE_BY_3;
        }

        if (seg2 > 1) {
            _regs.chdiv_seg2_en = 1;
            _regs.chdiv_seg_sel = lmx2592_regs_t::chdiv_seg_sel_t::CHDIV_SEG_SEL_DIV_SEG_1_AND_2;
        } else {
            _regs.chdiv_seg2_en = 0;
        }

        if (seg2 == 1) {
            _regs.chdiv_seg2 = lmx2592_regs_t::chdiv_seg2_t::CHDIV_SEG2_POWERDOWN;
        } else if (seg2 == 2) {
            _regs.chdiv_seg2 = lmx2592_regs_t::chdiv_seg2_t::CHDIV_SEG2_DIVIDE_BY_2;
        } else if (seg2 == 4) {
            _regs.chdiv_seg2 = lmx2592_regs_t::chdiv_seg2_t::CHDIV_SEG2_DIVIDE_BY_4;
        } else if (seg2 == 6) {
            _regs.chdiv_seg2 = lmx2592_regs_t::chdiv_seg2_t::CHDIV_SEG2_DIVIDE_BY_6;
        } else if (seg2 == 8) {
            _regs.chdiv_seg2 = lmx2592_regs_t::chdiv_seg2_t::CHDIV_SEG2_DIVIDE_BY_8;
        }

        if (seg3 > 1) {
            _regs.chdiv_seg3_en = 1;
            _regs.chdiv_seg_sel = lmx2592_regs_t::chdiv_seg_sel_t::CHDIV_SEG_SEL_DIV_SEG_1_2_AND_3;
        } else {
            _regs.chdiv_seg3_en = 0;
        }

        if (seg3 == 1) {
            _regs.chdiv_seg3 = lmx2592_regs_t::chdiv_seg3_t::CHDIV_SEG3_POWERDOWN;
        } else if (seg3 == 2) {
            _regs.chdiv_seg3 = lmx2592_regs_t::chdiv_seg3_t::CHDIV_SEG3_DIVIDE_BY_2;
        } else if (seg3 == 4) {
            _regs.chdiv_seg3 = lmx2592_regs_t::chdiv_seg3_t::CHDIV_SEG3_DIVIDE_BY_4;
        } else if (seg3 == 6) {
            _regs.chdiv_seg3 = lmx2592_regs_t::chdiv_seg3_t::CHDIV_SEG3_DIVIDE_BY_6;
        } else if (seg3 == 8) {
            _regs.chdiv_seg3 = lmx2592_regs_t::chdiv_seg3_t::CHDIV_SEG3_DIVIDE_BY_8;
        }
    }
};

lmx2592_impl::sptr lmx2592_iface::make(write_spi_t write, read_spi_t read) {
    return std::make_shared<lmx2592_impl>(write, read);
}
