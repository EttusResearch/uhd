//
// Copyright 2018, 2017 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "lmx2592_regs.hpp"
#include <uhdlib/usrp/common/lmx2592.hpp>
#include <uhdlib/utils/narrow.hpp>
#include <array>
#include <chrono>
#include <iomanip>

using namespace uhd;

namespace {
// clang-format off
// Some constexprs are unused, but kept for reference. In this case, we keep
// them as comments so compilers don't throw warnings.
//constexpr double LMX2592_DOUBLER_MAX_REF_FREQ = 60e6;
//constexpr double LMX2592_MAX_FREQ_PFD = 125e6;

constexpr double LMX2592_MIN_REF_FREQ = 5e6;
constexpr double LMX2592_MAX_REF_FREQ = 1400e6;

constexpr double LMX2592_MAX_OUT_FREQ = 9.8e9;
constexpr double LMX2592_MIN_OUT_FREQ = 20e6;

//constexpr double LMX2592_MIN_VCO_FREQ = 3.55e9;
constexpr double LMX2592_MAX_VCO_FREQ = 7.1e9;

constexpr double LMX2592_MAX_DOUBLER_INPUT_FREQ = 200e6;
constexpr double LMX2592_MAX_MULT_OUT_FREQ = 250e6;
constexpr double LMX2592_MAX_MULT_INPUT_FREQ = 70e6;
constexpr double LMX2592_MAX_POSTR_DIV_OUT_FREQ = 125e6;

constexpr double DEFAULT_LMX2592_SPUR_DODGING_THRESHOLD = 2e6; // Hz

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
// Unused, but kept for reference
//constexpr std::array<double, NUM_DIVIDERS> LMX2592_CHDIV_MAX_FREQ = {
    //6000e6,   3550.0e6, 2366.67e6, 1775.00e6, 1183.33e6, 887.50e6, 591.67e6,
    //443.75e6, 295.83e6, 221.88e6,  110.94e6,  73.96e6, 55.47e6,  36.98e6
//};
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
// clang-format on

enum intermediate_frequency_t {
    FVCO,
    FLO,
    FRF_IN,
};

const char* log_intermediate_frequency(intermediate_frequency_t inter)
{
    switch (inter) {
        case FRF_IN:
            return "FRF_IN";
        case FVCO:
            return "FVCO";
        case FLO:
            return "FLO";
        default:
            return "???";
    }
}

// simple comparator that uses absolute value
inline bool abs_less_than_compare(const double a, const double b)
{
    return std::abs(a) < std::abs(b);
}

typedef std::pair<double, intermediate_frequency_t> offset_t;

// comparator that uses absolute value on the first value of an offset_t
inline bool offset_abs_less_than_compare(const offset_t a, const offset_t b)
{
    return std::abs(a.first) < std::abs(b.first);
}

} // namespace

class lmx2592_impl : public lmx2592_iface
{
public:
    explicit lmx2592_impl(write_spi_t write_fn, read_spi_t read_fn)
        : _write_fn([write_fn](const uint8_t addr, const uint16_t data) {
            const uint32_t spi_transaction =
                0 | ((addr & SPI_ADDR_MASK) << SPI_ADDR_SHIFT) | data;
            write_fn(spi_transaction);
        })
        , _read_fn([read_fn](const uint8_t addr) {
            const uint32_t spi_transaction = SPI_READ_FLAG
                                             | ((addr & SPI_ADDR_MASK) << SPI_ADDR_SHIFT);
            return read_fn(spi_transaction);
        })
        , _regs()
        , _rewrite_regs(true)
    {
        UHD_LOG_TRACE("LMX2592", "Initializing Synthesizer");

        // Soft Reset
        _regs.reset = 1;
        UHD_LOG_TRACE("LMX2592", "Resetting LMX");
        _write_fn(_regs.ADDR_R0, _regs.get_reg(_regs.ADDR_R0));

        // The bit is cleared on the synth during the reset
        _regs.reset = 0;

        // Set register values where driver defaults differ from the datasheet values
        _regs.acal_enable   = 0;
        _regs.fcal_enable   = 0;
        _regs.cal_clk_div   = 0;
        _regs.vco_idac_ovr  = 1;
        _regs.cp_idn        = 12;
        _regs.cp_iup        = 12;
        _regs.vco_idac      = 350;
        _regs.mash_ditherer = 1;
        _regs.outa_mux      = lmx2592_regs_t::outa_mux_t::OUTA_MUX_VCO;
        _regs.fcal_fast     = 1;

        // Write default register values, ensures register copy is synchronized
        _rewrite_regs = true;
        commit();

        _regs.fcal_enable = 1;
        commit();
    }

    ~lmx2592_impl() override
    {
        UHD_SAFE_CALL(_regs.powerdown = 1; commit();)
    }

    double set_frequency(const double target_freq,
        const bool spur_dodging = false,
        const double spur_dodging_threshold =
            DEFAULT_LMX2592_SPUR_DODGING_THRESHOLD) override
    {
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
        const auto core_vco_freq   = target_vco_freq / vco_multiplier;

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
        _regs.mult =
            narrow_cast<uint8_t>(std::floor(LMX2592_MAX_MULT_OUT_FREQ / input_freq));
        input_freq *= _regs.mult;

        // Post R divider
        _regs.pll_r =
            narrow_cast<uint8_t>(std::ceil(input_freq / LMX2592_MAX_POSTR_DIV_OUT_FREQ));

        // Default to divide by 2, will be increased later if N exceeds its limit
        int prescaler   = 2;
        _regs.pll_n_pre = lmx2592_regs_t::pll_n_pre_t::PLL_N_PRE_DIVIDE_BY_2;

        const int min_n_divider = LMX2592_MIN_N_DIV[_regs.mash_order];
        double pfd_freq         = input_freq / _regs.pll_r;
        while (pfd_freq * (prescaler * min_n_divider) / vco_multiplier > core_vco_freq) {
            _regs.pll_r++;
            pfd_freq = input_freq / _regs.pll_r;
        }

        _set_fcal_adj_values(pfd_freq);

        // Calculate N and frac
        const auto N_dot_F = target_vco_freq / (pfd_freq * prescaler);
        auto N             = static_cast<uint16_t>(std::floor(N_dot_F));
        if (N > MAX_N_DIVIDER) {
            _regs.pll_n_pre = lmx2592_regs_t::pll_n_pre_t::PLL_N_PRE_DIVIDE_BY_4;
            N /= 2;
        }
        const auto frac = N_dot_F - N;

        // Increase VCO step size to threshold to avoid primary fractional spurs
        const double min_vco_step_size = spur_dodging ? spur_dodging_threshold : 1;
        // Calculate Fden
        const auto initial_fden =
            static_cast<uint32_t>(std::floor(pfd_freq * prescaler / min_vco_step_size));
        const auto fden = (spur_dodging) ? _find_fden(initial_fden) : initial_fden;
        // Calculate Fnum
        const auto initial_fnum = static_cast<uint32_t>(std::round(frac * fden));
        const auto fnum         = (spur_dodging) ? _find_fnum(N,
                              initial_fnum,
                              fden,
                              prescaler,
                              pfd_freq,
                              output_divider,
                              spur_dodging_threshold)
                                         : initial_fnum;

        // Calculate mash_seed
        // if spur_dodging is true, mash_seed is the first odd value less than fden
        // else mash_seed is int(fden / 2);
        const uint32_t mash_seed = (spur_dodging) ? _find_mash_seed(fden)
                                                  : static_cast<uint32_t>(fden / 2);

        // Calculate actual Fcore_vco, Fvco, F_lo frequencies
        const auto actual_fvco = pfd_freq * prescaler * (N + double(fnum) / double(fden));
        const auto actual_fcore_vco = actual_fvco / vco_multiplier;
        const auto actual_f_lo      = actual_fcore_vco * vco_multiplier / output_divider;

        // Write to registers
        _regs.pll_n         = N;
        _regs.pll_num_lsb   = narrow_cast<uint16_t>(fnum);
        _regs.pll_num_msb   = narrow_cast<uint16_t>(fnum >> 16);
        _regs.pll_den_lsb   = narrow_cast<uint16_t>(fden);
        _regs.pll_den_msb   = narrow_cast<uint16_t>(fden >> 16);
        _regs.mash_seed_lsb = narrow_cast<uint16_t>(mash_seed);
        _regs.mash_seed_msb = narrow_cast<uint16_t>(mash_seed >> 16);

        UHD_LOGGER_TRACE("LMX2592") << "Tuned to " << actual_f_lo;

        // Toggle fcal field to start calibration
        _regs.fcal_enable = 0;
        commit();

        _regs.fcal_enable = 1;
        commit();

        return actual_f_lo;
    }

    void set_mash_order(const mash_order_t mash_order) override
    {
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

    void set_reference_frequency(const double ref_freq) override
    {
        if (ref_freq < LMX2592_MIN_REF_FREQ or ref_freq > LMX2592_MAX_REF_FREQ) {
            throw std::runtime_error(
                "Reference frequency is out of bounds for the LMX2592");
        }

        _ref_freq = ref_freq;
    }

    void set_output_power(const output_t output, const unsigned int power) override
    {
        UHD_LOGGER_TRACE("LMX2592")
            << "Set output: " << (output == RF_OUTPUT_A ? "A" : "B") << " to power "
            << power;

        const auto MAX_POWER = 63;
        if (power > MAX_POWER) {
            UHD_LOGGER_ERROR("LMX2592") << "Requested power level of " << power
                                        << " exceeds maximum of " << MAX_POWER;
            return;
        }

        if (output == RF_OUTPUT_A) {
            _regs.outa_power = power;
        } else {
            _regs.outb_power = power;
        }

        commit();
    }

    void set_output_enable(const output_t output, const bool enable) override
    {
        UHD_LOGGER_TRACE("LMX2592")
            << "Set output " << (output == RF_OUTPUT_A ? "A" : "B") << " to "
            << (enable ? "On" : "Off");

        if (enable) {
            _regs.chdiv_dist_pd = 0;

            if (output == RF_OUTPUT_A) {
                _regs.outa_pd = 0;

            } else {
                _regs.outb_pd = 0;
            }

        } else {
            if (output == RF_OUTPUT_A) {
                _regs.outa_pd        = 1;
                _regs.vco_dista_pd   = 1;
                _regs.chdiv_dista_en = 0;

            } else {
                _regs.outb_pd        = 1;
                _regs.vco_distb_pd   = 1;
                _regs.chdiv_distb_en = 0;
            }
        }

        // If both channels are disabled
        if (_regs.outa_pd == 1 and _regs.outb_pd == 1) {
            _regs.chdiv_dist_pd = 1;
        }

        commit();
    }

    bool get_lock_status() override
    {
        // SPI MISO is being driven by lock detect
        // If the PLL is locked we expect to read 0xFFFF from any read, else 0x0000
        const auto value_read  = _read_fn(_regs.ADDR_R0);
        const auto lock_status = (value_read == 0xFFFF);

        UHD_LOG_TRACE("LMX2592",
            str(boost::format("Read Lock status: 0x%04X")
                % static_cast<unsigned int>(value_read)));

        return lock_status;
    }

    void commit() override
    {
        UHD_LOGGER_DEBUG("LMX2592")
            << "Storing register cache " << (_rewrite_regs ? "completely" : "selectively")
            << " to LMX via SPI...";
        const auto changed_addrs = _rewrite_regs ? _regs.get_all_addrs()
                                                 : _regs.get_changed_addrs<size_t>();

        for (const auto addr : changed_addrs) {
            _write_fn(addr, _regs.get_reg(addr));
            UHD_LOGGER_TRACE("LMX2592")
                << "Register " << std::setw(2) << static_cast<unsigned int>(addr)
                << ": 0x" << std::hex << std::uppercase << std::setw(4)
                << std::setfill('0') << static_cast<unsigned int>(_regs.get_reg(addr));
        }

        _regs.save_state();
        UHD_LOG_DEBUG("LMX2592",
            "Writing registers complete: "
            "Updated "
                << changed_addrs.size() << " registers.");

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

    void _set_chdiv_values(const int output_divider_index)
    {
        // Configure divide segments and mux
        const auto seg1 = LMX2592_CHDIV_SEGS[output_divider_index][0];
        const auto seg2 = LMX2592_CHDIV_SEGS[output_divider_index][1];
        const auto seg3 = LMX2592_CHDIV_SEGS[output_divider_index][2];

        _regs.chdiv_seg_sel = lmx2592_regs_t::chdiv_seg_sel_t::CHDIV_SEG_SEL_POWERDOWN;

        if (seg1 > 1) {
            _regs.chdiv_seg_sel =
                lmx2592_regs_t::chdiv_seg_sel_t::CHDIV_SEG_SEL_DIV_SEG_1;
            _regs.chdiv_seg1_en = 1;
            _regs.outa_mux      = lmx2592_regs_t::outa_mux_t::OUTA_MUX_DIVIDER;
            _regs.outb_mux      = lmx2592_regs_t::outb_mux_t::OUTB_MUX_DIVIDER;
            _regs.vco_dista_pd  = 1;
            _regs.vco_distb_pd  = 1;
            _regs.chdiv_dist_pd = 0;

            if (_regs.outa_pd == 0) {
                _regs.chdiv_dista_en = 1;
            }
            if (_regs.outb_pd == 0) {
                _regs.chdiv_distb_en = 1;
            }

        } else {
            _regs.chdiv_seg1_en = 0;
            _regs.outa_mux      = lmx2592_regs_t::outa_mux_t::OUTA_MUX_VCO;
            _regs.outb_mux      = lmx2592_regs_t::outb_mux_t::OUTB_MUX_VCO;
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
            _regs.chdiv_seg_sel =
                lmx2592_regs_t::chdiv_seg_sel_t::CHDIV_SEG_SEL_DIV_SEG_1_AND_2;
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
            _regs.chdiv_seg_sel =
                lmx2592_regs_t::chdiv_seg_sel_t::CHDIV_SEG_SEL_DIV_SEG_1_2_AND_3;
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

    void _set_fcal_adj_values(const double pfd_freq)
    {
        // Adjust FCAL speed for particularly high or low PFD frequencies
        if (pfd_freq < 5e6) {
            _regs.fcal_lpfd_adj = lmx2592_regs_t::fcal_lpfd_adj_t::FCAL_LPFD_ADJ_5MHZ;
            _regs.fcal_hpfd_adj = lmx2592_regs_t::fcal_hpfd_adj_t::FCAL_HPFD_ADJ_UNUSED;
            _regs.pfd_ctl       = lmx2592_regs_t::pfd_ctl_t::PFD_CTL_DUAL_PFD;
        } else if (pfd_freq < 10e6) {
            _regs.fcal_lpfd_adj = lmx2592_regs_t::fcal_lpfd_adj_t::FCAL_LPFD_ADJ_10MHZ;
            _regs.fcal_hpfd_adj = lmx2592_regs_t::fcal_hpfd_adj_t::FCAL_HPFD_ADJ_UNUSED;
            _regs.pfd_ctl       = lmx2592_regs_t::pfd_ctl_t::PFD_CTL_DUAL_PFD;
        } else if (pfd_freq < 20e6) {
            _regs.fcal_lpfd_adj = lmx2592_regs_t::fcal_lpfd_adj_t::FCAL_LPFD_ADJ_20MHZ;
            _regs.fcal_hpfd_adj = lmx2592_regs_t::fcal_hpfd_adj_t::FCAL_HPFD_ADJ_UNUSED;
            _regs.pfd_ctl       = lmx2592_regs_t::pfd_ctl_t::PFD_CTL_DUAL_PFD;
        } else if (pfd_freq <= 100e6) {
            _regs.fcal_lpfd_adj = lmx2592_regs_t::fcal_lpfd_adj_t::FCAL_LPFD_ADJ_UNUSED;
            _regs.fcal_hpfd_adj = lmx2592_regs_t::fcal_hpfd_adj_t::FCAL_HPFD_ADJ_UNUSED;
            _regs.pfd_ctl       = lmx2592_regs_t::pfd_ctl_t::PFD_CTL_DUAL_PFD;
        } else if (pfd_freq <= 150e6) {
            _regs.fcal_lpfd_adj = lmx2592_regs_t::fcal_lpfd_adj_t::FCAL_LPFD_ADJ_UNUSED;
            _regs.fcal_hpfd_adj = lmx2592_regs_t::fcal_hpfd_adj_t::FCAL_HPFD_ADJ_100MHZ;
            _regs.pfd_ctl       = lmx2592_regs_t::pfd_ctl_t::PFD_CTL_DUAL_PFD;
        } else if (pfd_freq <= 200e6) {
            _regs.fcal_lpfd_adj = lmx2592_regs_t::fcal_lpfd_adj_t::FCAL_LPFD_ADJ_UNUSED;
            _regs.fcal_hpfd_adj = lmx2592_regs_t::fcal_hpfd_adj_t::FCAL_HPFD_ADJ_150MHZ;
            _regs.pfd_ctl       = lmx2592_regs_t::pfd_ctl_t::PFD_CTL_DUAL_PFD;
        } else {
            // Note, this case requires single-loop PFD which increases PLL noise floor
            _regs.fcal_lpfd_adj = lmx2592_regs_t::fcal_lpfd_adj_t::FCAL_LPFD_ADJ_UNUSED;
            _regs.fcal_hpfd_adj = lmx2592_regs_t::fcal_hpfd_adj_t::FCAL_HPFD_ADJ_200MHZ;
            _regs.pfd_ctl       = lmx2592_regs_t::pfd_ctl_t::PFD_CTL_SINGLE_PFD;
        }
    }

    // "k" is a derived value that indicates where sub-fractional spurs will be present
    // at a given Fden value.  A "k" value of 1 indicates there will be no spurs.
    // See the LMX2592 datasheet for more information
    // Table 48 on pg. 30, Revision F (or search for "sub-fractional spurs")
    int _get_k(const uint32_t fden) const
    {
        const auto mash = _regs.mash_order;
        if (mash == lmx2592_regs_t::mash_order_t::MASH_ORDER_INT_MODE
            or mash == lmx2592_regs_t::mash_order_t::MASH_ORDER_FIRST) {
            return 1;
        } else if (mash == lmx2592_regs_t::mash_order_t::MASH_ORDER_SECOND) {
            if (fden % 2 != 0) {
                return 1;
            } else {
                return 2;
            }
        } else if (mash == lmx2592_regs_t::mash_order_t::MASH_ORDER_THIRD) {
            if (fden % 2 != 0 and fden % 3 != 0) {
                return 1;
            } else if (fden % 2 == 0 and fden % 3 != 0) {
                return 2;
            } else if (fden % 2 != 0 and fden % 3 == 0) {
                return 3;
            } else {
                return 6;
            }
        } else if (mash == lmx2592_regs_t::mash_order_t::MASH_ORDER_FOURTH) {
            if (fden % 2 != 0 and fden % 3 != 0) {
                return 1;
            } else if (fden % 2 == 0 and fden % 3 != 0) {
                return 3;
            } else if (fden % 2 != 0 and fden % 3 == 0) {
                return 4;
            } else {
                return 12;
            }
        }
        UHD_THROW_INVALID_CODE_PATH();
    }

    // Find a value of fden such that "k" is 1 to avoid subfractional spurs
    // See the _get_k function for more details on how k is calculated
    uint32_t _find_fden(const uint32_t initial_fden) const
    {
        auto fden = initial_fden;
        // mathematically, this loop should run a maximum of 4 times
        // i.e. initial_fden = 6N + 4 and mash_order is third or fourth order
        for (int i = 0; i < 4; ++i) {
            if (_get_k(fden) == 1) {
                UHD_LOGGER_TRACE("LMX2592")
                    << "_find_fden(" << initial_fden << ") returned " << fden;
                return fden;
            }
            // decrement rather than increment, as incrementing fden would decrease
            // the step size and violate any minimum step size that has been set
            --fden;
        }
        UHD_LOGGER_WARNING("LMX2592") << "Unable to find suitable fractional value "
                                         "denominator for spur dodging on LMX2592";
        UHD_LOGGER_ERROR("LMX2592") << "Spur dodging failed";
        return initial_fden;
    }

    // returns the offset of the closest multiple of
    // spur_frequency_base to target_frequency
    // A negative offset indicates the closest multiple is at a lower frequency
    double _get_closest_spur_offset(double target_frequency, double spur_frequency_base)
    {
        // find closest multiples of spur_frequency_base to target_frequency
        const auto first_harmonic_number =
            std::floor(target_frequency / spur_frequency_base);
        const auto second_harmonic_number = first_harmonic_number + 1;
        // calculate offsets
        const auto first_spur_offset =
            (first_harmonic_number * spur_frequency_base) - target_frequency;
        const auto second_spur_offset =
            (second_harmonic_number * spur_frequency_base) - target_frequency;
        // select offset with smallest absolute value
        return std::min({first_spur_offset, second_spur_offset}, abs_less_than_compare);
    }

    // returns the closest spur offset among 4 different spurs
    // as well as which signal the spur is close to
    // 1. PFD to Frf_in spur (Integer boundary)
    // 2. PFD to Fvco spur
    // 3. Reference to Fvco spur
    // 4. Reference to Flo spur
    // A negative offset indicates the closest spur is at a lower frequency
    offset_t _get_min_offset_frequency(const uint16_t N,
        const uint32_t fnum,
        const uint32_t fden,
        const int prescaler,
        const double pfd_freq,
        const int output_divider)
    {
        // Calculate intermediate values
        const auto fref   = _ref_freq;
        const auto frf_in = pfd_freq * (N + double(fnum) / double(fden));
        const auto fvco   = frf_in * prescaler;
        const auto flo    = fvco / output_divider;

        // the minimum offset is the smallest absolute value of these 4 values
        // as calculated by the _get_closest_spur_offset function
        // However, we also need to know which IF the spur is closest to
        // in order to calculate the necessary frequency shift

        // Integer Boundary:
        const offset_t ib_spur = {_get_closest_spur_offset(frf_in, pfd_freq), FRF_IN};

        // PFD Offset Spur:
        const offset_t pfd_offset_spur = {_get_closest_spur_offset(fvco, pfd_freq), FVCO};

        // Reference to Fvco Spur:
        const offset_t fvco_spur = {_get_closest_spur_offset(fvco, fref), FVCO};

        // Reference to F_lo Spur:
        const offset_t flo_spur = {_get_closest_spur_offset(flo, fref), FLO};

        // use min with special comparator for minimal absolute value
        return std::min({ib_spur, pfd_offset_spur, fvco_spur, flo_spur},
            offset_abs_less_than_compare);
    }

    // Find a suitable fnum such that _get_min_offset_frequency reports
    // the closest spur is at least spur_dodging_threshold away.
    // To see what spurs are considered, see _get_min_offset_frequency.
    // This function uses a naive iterative approach, which could potentially
    // fail for certain configurations.  For example, it is assumed that the
    // PFD frequency will be at least 10x larger than the step size of
    // (fnum / fden). This function only considers at least 50% potential
    // values of fnum, and does not consider changes to N.
    uint32_t _find_fnum(const uint16_t N,
        const uint32_t initial_fnum,
        const uint32_t fden,
        const int prescaler,
        const double pfd_freq,
        const int output_divider,
        const double spur_dodging_threshold)
    {
        auto fnum = initial_fnum;
        auto min_offset =
            _get_min_offset_frequency(N, fnum, fden, prescaler, pfd_freq, output_divider);

        UHD_LOGGER_TRACE("LMX2592") << "closest spur is at " << min_offset.first << " to "
                                    << log_intermediate_frequency(min_offset.second);

        // shift away from the closest integer boundary i.e. towards 0.5
        const double delta_fnum_sign = ((((double)fnum) / ((double)fden)) < 0.5) ? 1 : -1;

        while (std::abs(min_offset.first) < spur_dodging_threshold) {
            double shift = spur_dodging_threshold;
            // if the spur is in the same direction as the desired shift direction...
            if (std::signbit(min_offset.first) == std::signbit(delta_fnum_sign)) {
                shift += std::abs(min_offset.first);
            } else {
                shift -= std::abs(min_offset.first);
            }

            // convert shift of IF value to shift of Frf_in
            if (min_offset.second == FVCO) {
                shift /= prescaler;
            } else if (min_offset.second == FLO) {
                shift /= prescaler;
                shift *= output_divider;
            }

            double delta_fnum_value = std::ceil((shift / pfd_freq) * fden);
            fnum += narrow_cast<int32_t>(delta_fnum_value * delta_fnum_sign);

            UHD_LOGGER_TRACE("LMX2592")
                << "adjusting fnum by " << (delta_fnum_value * delta_fnum_sign);

            // fnum is unsigned, so this also checks for underflow
            if (fnum >= fden) {
                UHD_LOGGER_WARNING("LMX2592")
                    << "Unable to find suitable fractional value numerator for spur "
                       "dodging on LMX2592";
                UHD_LOGGER_ERROR("LMX2592") << "Spur dodging failed";
                return initial_fnum;
            }

            min_offset = _get_min_offset_frequency(
                N, fnum, fden, prescaler, pfd_freq, output_divider);

            UHD_LOGGER_TRACE("LMX2592")
                << "closest spur is at " << min_offset.first << " to "
                << log_intermediate_frequency(min_offset.second);
        }
        UHD_LOGGER_TRACE("LMX2592")
            << "_find_fnum(" << initial_fnum << ") returned " << fnum;
        return fnum;
    }

    // if spur_dodging is true, mash_seed is the first odd value less than fden
    static uint32_t _find_mash_seed(const uint32_t fden)
    {
        if (fden < 2) {
            return 1;
        } else {
            return (fden - 2) | 0x1;
        }
    };
};

lmx2592_impl::sptr lmx2592_iface::make(write_spi_t write, read_spi_t read)
{
    return std::make_shared<lmx2592_impl>(write, read);
}
