//
// Copyright 2020 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//


#include "lmx2572_regs.hpp"
#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/math.hpp>
#include <uhdlib/usrp/common/lmx2572.hpp>
#include <uhdlib/utils/interpolation.hpp>
#include <uhdlib/utils/narrow.hpp>
#include <cmath>
#include <limits>
#include <map>

namespace {
// LOG ID
constexpr char LOG_ID[] = "LMX2572";
// Highest LO / output frequency
constexpr double MAX_OUT_FREQ = 6.4e9; // Hz
// Lowest LO / output frequency
constexpr double MIN_OUT_FREQ = 12.5e6; // Hz
// Target loop bandwidth
constexpr double TARGET_LOOP_BANDWIDTH = 75e3; // Hz
// Loop Filter gain setting resistor
constexpr double LOOP_GAIN_SETTING_RESISTANCE = 150; // ohm
// Delay after powerup. TI recommends a 10 ms delay after clearing powerdown
// (not documented in the datasheet).
const uhd::time_spec_t POWERUP_DELAY = uhd::time_spec_t(10e-3);
// Conservative estimate for PLL to lock which includes VCO calibration and PLL settling
constexpr double PLL_LOCK_TIME = 200e-6; // s

// Valid input/reference frequencies (fOSC)
//
// NOTE: These frequencies are valid for X400/ZBX. If we need to use this
// driver elsewhere, this part needs to be refactored.
const std::set<double> VALID_FOSC{61.44e6, 64e6, 62.5e6, 50e6};


}; // namespace

//! Control interface for an LMX2572 synthesizer
class lmx2572_impl : public lmx2572_iface
{
public:
    enum class muxout_state_t { LOCKDETECT, SDO };

    explicit lmx2572_impl(
        write_fn_t&& poke_fn, read_fn_t&& peek_fn, sleep_fn_t&& sleep_fn)
        : _poke16(std::move(poke_fn))
        , _peek16(std::move(peek_fn))
        , _sleep(std::move(sleep_fn))
        , _regs()
    {
        _regs.save_state();
    }

    void commit() override
    {
        UHD_LOG_TRACE(LOG_ID, "Storing register cache to LMX2572...");
        const auto changed_addrs = _regs.get_changed_addrs<uint8_t>();
        for (const auto addr : changed_addrs) {
            // We write R0 last, for double-buffering
            if (addr == 0) {
                continue;
            }
            _poke16(addr, _regs.get_reg(addr));
        }
        _poke16(0, _regs.get_reg(0));
        _regs.save_state();
        UHD_LOG_TRACE(LOG_ID,
            "Storing cache complete: Updated " << changed_addrs.size() << " registers.");
    }

    bool get_enabled() override
    {
        // Chip is either in normal operation mode or power down mode
        return _regs.powerdown == lmx2572_regs_t::powerdown_t::POWERDOWN_NORMAL_OPERATION;
    }

    void set_enabled(const bool enabled) override
    {
        const bool prev_enabled = get_enabled();

        _regs.powerdown = enabled
                              ? lmx2572_regs_t::powerdown_t::POWERDOWN_NORMAL_OPERATION
                              : lmx2572_regs_t::powerdown_t::POWERDOWN_POWER_DOWN;
        _poke16(0, _regs.get_reg(0));

        if (enabled && !prev_enabled) {
            _sleep(POWERUP_DELAY);
        }
    }

    void reset() override
    {
        // Power-on Programming Sequence described in the datasheet,
        // Section 7.5.1.1
        _regs       = lmx2572_regs_t{};
        _regs.reset = lmx2572_regs_t::reset_t::RESET_RESET;
        _poke16(0, _regs.get_reg(0));
        // Reset bit is self-clearing, it does not need to be poked twice. We
        // manually reset it in the SW cache so we don't accidentally reset again
        _regs.reset = lmx2572_regs_t::reset_t::RESET_NORMAL_OPERATION;
        // Also enable register readback so we can read back the magic number
        // register
        _enable_register_readback(true);
        // If the LO was previously powered down, the reset above will power it up. On
        // power-up, we need to wait for the power up delay recommended by TI.
        _sleep(POWERUP_DELAY);
        // Check we can read back the last register, which always returns a
        // magic constant:
        const auto magic125 = _peek16(125);
        if (magic125 != 0x2288) {
            UHD_LOG_ERROR(LOG_ID,
                "Unable to communicate with LMX2572! Expected R125==0x2288, got: "
                    << std::hex << magic125 << std::dec);
            throw uhd::runtime_error("Unable to communicate to LMX2572!");
        }
        UHD_LOG_TRACE(LOG_ID, "Communication with LMX2572 successful.");
        // Now set _regs into a sensible state
        _set_defaults();
        // Now write the regs in reverse order, skipping RO regs
        const auto ro_regs = _regs.get_ro_regs();
        // Write R0 last for the double buffering
        for (int addr = _regs.get_num_regs() - 2; addr >= 0; addr--) {
            if (ro_regs.count(addr)) {
                continue;
            }
            _poke16(uhd::narrow_cast<uint8_t>(addr), _regs.get_reg(addr));
        }
        _regs.save_state();
    }

    bool get_lock_status() override
    {
        // Disable register readback which implicitly enables lock detect mode
        _enable_register_readback(false);
        // If the PLL is locked we expect to read 0xFFFF from any read
        return _peek16(0) == 0xFFFF;
    }

    uint16_t peek16(const uint8_t addr)
    {
        _enable_register_readback(true);
        return _peek16(addr);
    }

    void set_sync_mode(const bool enable) override
    {
        _sync_mode = enable;
    }

    //! Returns the enabled/disabled state of the phase synchronization
    bool get_sync_mode() override
    {
        return _sync_mode;
    }

    void set_output_enable_all(const bool enable) override
    {
        set_output_enable(RF_OUTPUT_A, enable);
        set_output_enable(RF_OUTPUT_B, enable);
    }

    void set_output_enable(const output_t output, const bool enable) override
    {
        if (output == RF_OUTPUT_A) {
            _regs.outa_pd = enable ? lmx2572_regs_t::outa_pd_t::OUTA_PD_NORMAL_OPERATION
                                   : lmx2572_regs_t::outa_pd_t::OUTA_PD_POWER_DOWN;
            return;
        }
        if (output == RF_OUTPUT_B) {
            _regs.outb_pd = enable ? lmx2572_regs_t::outb_pd_t::OUTB_PD_NORMAL_OPERATION
                                   : lmx2572_regs_t::outb_pd_t::OUTB_PD_POWER_DOWN;
            return;
        }
        UHD_THROW_INVALID_CODE_PATH();
    }

    void set_output_power(const output_t output, const uint8_t power) override
    {
        if (output == RF_OUTPUT_A) {
            _regs.outa_pwr = power;
            return;
        }
        if (output == RF_OUTPUT_B) {
            _regs.outb_pwr = power;
            return;
        }
        UHD_THROW_INVALID_CODE_PATH();
    }

    void set_mux_input(const output_t output, const mux_in_t input) override
    {
        switch (output) {
            case RF_OUTPUT_A: {
                switch (input) {
                    case mux_in_t::DIVIDER:
                        _regs.outa_mux =
                            lmx2572_regs_t::outa_mux_t::OUTA_MUX_CHANNEL_DIVIDER;
                        return;
                    case mux_in_t::VCO:
                        _regs.outa_mux = lmx2572_regs_t::outa_mux_t::OUTA_MUX_VCO;
                        return;
                    case mux_in_t::HIGH_IMPEDANCE:
                        _regs.outa_mux =
                            lmx2572_regs_t::outa_mux_t::OUTA_MUX_HIGH_IMPEDANCE;
                        return;
                    default:
                        break;
                }
                break;
            }
            case RF_OUTPUT_B: {
                switch (input) {
                    case mux_in_t::DIVIDER:
                        _regs.outb_mux =
                            lmx2572_regs_t::outb_mux_t::OUTB_MUX_CHANNEL_DIVIDER;
                        return;
                    case mux_in_t::VCO:
                        _regs.outb_mux = lmx2572_regs_t::outb_mux_t::OUTB_MUX_VCO;
                        return;
                    case mux_in_t::HIGH_IMPEDANCE:
                        _regs.outb_mux =
                            lmx2572_regs_t::outb_mux_t::OUTB_MUX_HIGH_IMPEDANCE;
                        return;
                    case mux_in_t::SYSREF:
                        _regs.outb_mux = lmx2572_regs_t::outb_mux_t::OUTB_MUX_SYSREF;
                        return;
                    default:
                        break;
                }
                break;
            }
            default:
                break;
        }
        UHD_THROW_INVALID_CODE_PATH();
    }

    double set_frequency(
        const double target_freq, const double fOSC, const bool spur_dodging) override
    {
        // Sanity check
        if (target_freq > MAX_OUT_FREQ || target_freq < MIN_OUT_FREQ) {
            UHD_LOG_ERROR(LOG_ID,
                "Invalid LMX2572 target frequency! Must be in ["
                    << (MIN_OUT_FREQ / 1e6) << " MHz, " << (MAX_OUT_FREQ / 1e6)
                    << " MHz]!");
            throw uhd::value_error("Invalid LMX2572 target frequency!");
        }
        UHD_ASSERT_THROW(VALID_FOSC.count(fOSC));
        // Create an integer version of fOSC for some of the following
        // calculations
        const uint64_t fOSC_int = static_cast<uint64_t>(fOSC);

        // 1. Set up output/channel divider value and the output mux
        const uint16_t out_D = _set_output_divider(target_freq);
        const double fVCO    = target_freq * out_D;
        UHD_ASSERT_THROW(3200e6 <= fVCO && fVCO <= 6400e6);

        // 2. Configure the reference dividers/multipliers
        _set_pll_div_and_mult(target_freq, fVCO, fOSC_int);

        // Calculate phase detector frequency
        // See datasheet (Section 7.3.2):
        // Equation (1): fPD = fOSC × OSC_2X × MULT / (PLL_R_PRE × PLL_R)
        const double fPD =
            fOSC * (_regs.osc_2x + 1) * _regs.mult / (_regs.pll_r_pre * _regs.pll_r);

        // pre-3.  Identify SYNC category.
        // Based on the category, we need to set VCO_PHASE_SYNC_EN appropriately
        // and update our p-multiplier.
        // Note: In the line below, we use target_freq and not actual_freq. That
        // is OK, because we know that _get_sync_cat() only does a check to see
        // if target_freq is an integer multiple of fOSC. If that's the case,
        // then rounding/coercion errors won't happen between target_freq and
        // actual_freq because we can always exactly produce frequencies that
        // are integer multiples of fOSC. This way, we don't have a circular
        // dependency (because actual_freq depends on p indirectly).
        const int p =
            _set_phase_sync(_get_sync_cat(_regs.mult, fOSC, target_freq, out_D));
        // P is introduced in Section 7.3.12 - calculate P with adaptation of
        // Equation (3). It also comes up again in 8.1.6, although it's not
        // called P there any more. There, it is the factor between N' and N.
        // P == 2 whenever we're in a sync category where we need to program the
        // N-divider with half the "normal" values. In TICS PRO, this value is
        // described as "Calculated Included Channel Divide".

        // 3. Calculate N, PLL_NUM and PLL_DEN
        const double delta_fVCO = spur_dodging ? 2e6 : 1.0;
        // In the next statement, we:
        // - Estimate PLL_DEN by PLL_DEN = ceil(fPD * p / delta_fVCO)
        // - This value can exceed the limits of uint32_t, so we clamp it between
        //   1 and 0xFFFFFFFF (the denominator can also not be zero, so we need
        //   to catch rounding errors)
        // - Finally, convert to uint32_t
        const uint32_t PLL_DEN = static_cast<uint32_t>(std::max(1.0,
            std::min(std::ceil(fPD * p / delta_fVCO),
                double(std::numeric_limits<uint32_t>::max()))));
        UHD_ASSERT_THROW(PLL_DEN > 0);
        // This is where we do the N=N'/2 division from Section 8.1.6:
        const double N_real    = fVCO / (fPD * p);
        const uint32_t N       = static_cast<uint32_t>(std::floor(N_real));
        const uint32_t PLL_NUM = std::round((N_real - double(N)) * PLL_DEN);

        // See datasheet (Section 7.3.4):
        // Equation (2): fVCO = fPD * [PLL_N + (PLL_NUM / PLL_DEN)] * p
        // Note that p here is the "extra divider in SYNC mode" that is in the
        // text, but not listed in Eq. (2) in this section.
        const double fVCO_actual =
            fPD * p * (N + (static_cast<double>(PLL_NUM) / PLL_DEN));
        UHD_ASSERT_THROW(3200e6 <= fVCO_actual && fVCO_actual <= 6400e6);
        const double actual_freq = fVCO_actual / out_D;
        // clang-format off
        UHD_LOG_TRACE(LOG_ID,
            "Calculating settings for fTARGET=" << (target_freq / 1e6)
                << " MHz, fOSC=" << (fOSC / 1e6)
                << " MHz: Target fVCO=" << (fVCO / 1e6)
                << " MHz, actual fVCO=" << (fVCO_actual / 1e6)
                << " MHz. R_pre=" << _regs.pll_r_pre
                << " OSC2X=" << _regs.osc_2x
                << " MULT=" << std::to_string(_regs.mult)
                << " PLL_R=" << std::to_string(_regs.pll_r)
                << " P=" << p
                << " N=" << N
                << " PLL_DEN=" << PLL_DEN
                << " PLL_NUM=" << PLL_NUM
                << " CHDIV=" << out_D);
        // clang-format on

        // 4. Set frequency dependent registers
        _compute_and_set_mult_hi(fOSC);
        _set_pll_n(N); // Note: N-divider values already account for
        _set_pll_num(PLL_NUM); // N-divider adaptations at this point. No more
        _set_pll_den(PLL_DEN); // divide-by-2 necessary.
        _set_fcal_hpfd_adj(fPD);
        _set_fcal_lpfd_adj(fPD);
        _set_pfd_dly(fVCO_actual);
        _set_mash_seed(spur_dodging, PLL_NUM, fPD);

        if (get_sync_mode()) {
            // From R69 register field description (Table 77):
            // The delay should be at least 4 times the PLL lock time. The
            // delay is expressed in state machine clock periods where
            // state_machine_clock_period = 2^(CAL_CLK_DIV) / fOSC and
            // CAL_CLK_DIV is one of {0, 1}
            const double period = ((_regs.cal_clk_div == 0) ? 1 : 2) / fOSC;
            const uint32_t mash_rst_count =
                static_cast<uint32_t>(std::ceil(4 * PLL_LOCK_TIME / period));
            _set_mash_rst_count(mash_rst_count);
        }

        // 5. Calculate charge pump gain
        _compute_and_set_charge_pump_gain(fVCO_actual, N_real);

        // 6. Calculate VCO calibration values
        _compute_and_set_vco_cal(fVCO_actual);

        // 7. Set amplitude on enabled outputs
        if (_get_output_enabled(RF_OUTPUT_A)) {
            _find_and_set_lo_power(actual_freq, RF_OUTPUT_A);
        }
        if (_get_output_enabled(RF_OUTPUT_B)) {
            _find_and_set_lo_power(actual_freq, RF_OUTPUT_B);
        }

        return actual_freq;
    }

private:
    /**************************************************************************
     * Attributes
     *************************************************************************/
    write_fn_t _poke16;
    read_fn_t _peek16;
    sleep_fn_t _sleep;
    lmx2572_regs_t _regs = lmx2572_regs_t();
    bool _sync_mode      = false;

    /**************************************************************************
     * Private Methods
     *************************************************************************/
    //! Identify sync category according to Section 8.1.6 of the datasheet. This
    // function implements the flowchart (Fig. 170).
    sync_cat _get_sync_cat(
        const uint8_t M, const double fOSC, const double fOUT, const uint16_t CHDIV)
    {
        if (!get_sync_mode()) {
            return NONE;
        }
        // Right-hand path of the flowchart:
        if (M > 1) {
            if (CHDIV > 2) {
                return CAT4;
            }
            if (std::fmod(fOUT, fOSC * M) != 0) {
                return CAT4;
            }
            // In the flow chart, there's a third condition (PLL_NUM == 0) but
            // that is implied in the previous condition. Here's the proof:
            // 1) Because of the previous condition, we know that
            //    f_OUT = f_OSC * M * K, where K is an integer.
            //    We also know that that the doubler is disabled, so we can
            //    ignore it here.
            // 2) PLL_NUM must be zero when f_VCO / f_PD is an integer value:
            //
            //        f_VCO
            //    N = -----
            //        f_PD
            //
            // 3) We can insert
            //        f_VCO = f_OUT * D
            //        and
            //        f_PD = f_OSC * M / R where R is an integer (R-divider)
            //        which yields:
            //
            //        f_OUT * D * R
            //    N = -------------
            //        f_OSC * M
            //
            // 4) Now we can insert 1), which yields
            //
            //    N = K * D * R
            //
            //    D is either 1 or 2, and K and R are integers. Therefore, N
            //    is an integer too, and PLL_NUM == 0.                      _
            //                                                             |_|
            //
            // Note: We could simply calculate N here, but that would require
            // knowing K and R, which we can avoid with this simple comment.
        }
        // Left-hand path of the flowchart:
        if (M == 1 && std::fmod(fOUT, fOSC) != 0) {
            return CAT3;
        }
        if (M == 1 && CHDIV > 2) {
            return CAT2;
        }
        if (CHDIV == 2) {
            return CAT1B;
        }
        return CAT1A;
    }

    //! Enable/disable register readback mode enabled
    // SPI MISO is multiplexed to lock detect and register readback. Reading
    // any register when the mux is set to lock detect will return just the
    // lock detect signal, so ensure we're in readback mode if reads desired
    void _enable_register_readback(const bool enable)
    {
        auto desired_state =
            enable ? lmx2572_regs_t::muxout_ld_sel_t::MUXOUT_LD_SEL_REGISTER_READBACK
                   : lmx2572_regs_t::muxout_ld_sel_t::MUXOUT_LD_SEL_LOCK_DETECT;
        if (_regs.muxout_ld_sel != desired_state) {
            _regs.muxout_ld_sel = desired_state;
            _poke16(0, _regs.get_reg(0));
        }
    }

    //! Sets the output divider registers
    //
    // Configures both the output divider and the output mux. If the divider is
    // used, the mux input is set to CHDIV, otherwise, it's set to VCO.
    uint16_t _set_output_divider(const double freq)
    {
        // clang-format off
        // Map the desired output / LO frequency to output divider settings
        const std::map<
            double,
            std::tuple<uint16_t, lmx2572_regs_t::chdiv_t>
            > out_div_map {
            //   freq    outD chdiv
                {25e6,  {256, lmx2572_regs_t::chdiv_t::CHDIV_DIVIDE_BY_256}},
                {50e6,  {128, lmx2572_regs_t::chdiv_t::CHDIV_DIVIDE_BY_128}},
                {100e6, {64,  lmx2572_regs_t::chdiv_t::CHDIV_DIVIDE_BY_64 }},
                {200e6, {32,  lmx2572_regs_t::chdiv_t::CHDIV_DIVIDE_BY_32 }},
                {400e6, {16,  lmx2572_regs_t::chdiv_t::CHDIV_DIVIDE_BY_16 }},
                {800e6, {8,   lmx2572_regs_t::chdiv_t::CHDIV_DIVIDE_BY_8  }},
                {1.6e9, {4,   lmx2572_regs_t::chdiv_t::CHDIV_DIVIDE_BY_4  }},
                {3.2e9, {2,   lmx2572_regs_t::chdiv_t::CHDIV_DIVIDE_BY_2  }},
                // CHDIV isn't used for out_divider == 1 so use DIVIDE_BY_2
                // We use +1 as an epsilon value here. Upon entering this function
                // we already know that that freq <= 6.4e9. We increase the
                // boundary here so that upper_bound() will not fail on the
                // corner case freq == 6.4e9.
                {6.4e9+1, {1,   lmx2572_regs_t::chdiv_t::CHDIV_DIVIDE_BY_2  }}
        };
        // clang-format on

        uint16_t out_D;
        lmx2572_regs_t::chdiv_t chdiv;
        auto out_div_it = out_div_map.upper_bound(freq);
        UHD_ASSERT_THROW(out_div_it != out_div_map.end());
        std::tie(out_D, chdiv) = out_div_it->second;
        _regs.chdiv            = lmx2572_regs_t::chdiv_t(chdiv);
        // If we're using the output divider, map it to the corresponding output
        // mux. Otherwise, connect the VCO directly to the mux.
        const mux_in_t input = (out_D > 1) ? mux_in_t::DIVIDER : mux_in_t::VCO;
        if (_get_output_enabled(RF_OUTPUT_A)) {
            set_mux_input(RF_OUTPUT_A, input);
        }
        if (_get_output_enabled(RF_OUTPUT_B)) {
            set_mux_input(RF_OUTPUT_B, input);
        }

        return out_D;
    }

    //! Returns the output enabled status of output
    bool _get_output_enabled(const output_t output)
    {
        if (output == RF_OUTPUT_A) {
            return _regs.outa_pd == lmx2572_regs_t::outa_pd_t::OUTA_PD_NORMAL_OPERATION;
        } else {
            return _regs.outb_pd == lmx2572_regs_t::outb_pd_t::OUTB_PD_NORMAL_OPERATION;
        }
    }

    //! Sets the MASH_RST_COUNT registers
    void _set_mash_rst_count(const uint32_t mash_rst_count)
    {
        _regs.mash_rst_count_upper = uhd::narrow_cast<uint16_t>(mash_rst_count >> 16);
        _regs.mash_rst_count_lower = uhd::narrow_cast<uint16_t>(mash_rst_count);
    }

    //! Calculate and set the mult_hi register
    //
    // Sets the MULT_HI bit (needs to be high if the multiplier output frequency
    // is larger than 100 MHz).
    //
    //  \param ref_frequency The OSCin signal's frequency.
    void _compute_and_set_mult_hi(const double fOSC)
    {
        const double fMULTout =
            (fOSC * (int(_regs.osc_2x) + 1) * _regs.mult) / _regs.pll_r_pre;
        _regs.mult_hi = (_regs.mult > 1 && fMULTout > 100e6)
                            ? lmx2572_regs_t::mult_hi_t::MULT_HI_GREATER_THAN_100M
                            : lmx2572_regs_t::mult_hi_t::MULT_HI_LESS_THAN_EQUAL_TO_100M;
    }

    //! Sets the mash seed value based on fPD and whether spur dodging is enabled
    void _set_mash_seed(const bool spur_dodging, const uint32_t PLL_NUM, const double pfd)
    {
        uint32_t mash_seed = 0;
        if (spur_dodging || PLL_NUM == 0) {
            // Leave mash_seed set to 0
        } else {
            const std::map<double, uint32_t> seed_map = {{25e6, 4999},
                {30.72e6, 5531},
                {31.25e6, 5591},
                {32e6, 5657},
                {50e6, 7096},
                {61.44e6, 7841},
                {62.5e6, 7907},
                {64e6, 7993}};
            mash_seed                                 = seed_map.lower_bound(pfd)->second;
        }
        _regs.mash_seed_upper = uhd::narrow_cast<uint16_t>(mash_seed >> 16);
        _regs.mash_seed_lower = uhd::narrow_cast<uint16_t>(mash_seed);
    }

    void _find_and_set_lo_power(const double freq, const output_t output)
    {
        if (freq < 3e9) {
            set_output_power(output, 25);
        } else if (3e9 <= freq && freq < 4e9) {
            constexpr double slope         = 5.0;
            constexpr double segment_range = 1e9;
            constexpr int power_base       = 25;
            const double offset            = freq - 3e9;
            const uint8_t power =
                std::round<uint8_t>(power_base + ((offset / segment_range) * slope));
            set_output_power(output, power);
        } else if (4e9 <= freq && freq < 5e9) {
            constexpr double slope         = 10.0;
            constexpr double segment_range = 1e9;
            constexpr int power_base       = 30;
            const double offset            = freq - 4e9;
            const uint8_t power =
                std::round<uint8_t>(power_base + ((offset / segment_range) * slope));
            set_output_power(output, power);
        } else if (5e9 <= freq && freq < 6.4e9) {
            constexpr double slope         = 5 / 1.4;
            constexpr double segment_range = 1.4e9;
            constexpr int power_base       = 40;
            const double offset            = freq - 5e9;
            const uint8_t power =
                std::round<uint8_t>(power_base + ((offset / segment_range) * slope));
            set_output_power(output, power);
        } else if (freq >= 6.4e9) {
            set_output_power(output, 45);
        } else {
            UHD_THROW_INVALID_CODE_PATH();
        }
    }

    //! Sets the FCAL_HPFD_ADJ value based on fPD
    void _set_fcal_hpfd_adj(const double pfd)
    {
        // These frequency constants are from the data sheet (Section 7.6.1)
        if (pfd <= 37.5e6) {
            _regs.fcal_hpfd_adj = 0x0;
        } else if (37.5e6 < pfd && pfd <= 75e6) {
            _regs.fcal_hpfd_adj = 0x1;
        } else if (75e6 < pfd && pfd <= 100e6) {
            _regs.fcal_hpfd_adj = 0x2;
        } else { // 100 MHz > pfd
            _regs.fcal_hpfd_adj = 0x3;
        }
    }

    //! Sets the FCAL_LPFD_ADJ value based on the fPD (Section 7.6.1)
    void _set_fcal_lpfd_adj(const double pfd)
    {
        // These frequency constants are from the data sheet (Section 7.6.1)
        if (pfd >= 10e6) {
            _regs.fcal_lpfd_adj = 0x0;
        } else if (10e6 > pfd && pfd >= 5e6) {
            _regs.fcal_lpfd_adj = 0x1;
        } else if (5e6 > pfd && pfd >= 2.5e6) {
            _regs.fcal_lpfd_adj = 0x2;
        } else { // pfd > 2.5MHz
            _regs.fcal_lpfd_adj = 0x3;
        }
    }

    //! Sets the PFD Delay value based on fVCO (Section 7.3.4)
    void _set_pfd_dly(const double fVCO)
    {
        UHD_ASSERT_THROW(_regs.mash_order == lmx2572_regs_t::MASH_ORDER_THIRD_ORDER);
        // Thse constants / values come from the data sheet (Table 3)
        if (3.2e9 <= fVCO && fVCO < 4e9) {
            _regs.pfd_dly_sel = 2;
        } else if (4e9 <= fVCO && fVCO < 4.9e9) {
            _regs.pfd_dly_sel = 2;
        } else if (4.9e9 <= fVCO && fVCO <= 6.4e9) {
            _regs.pfd_dly_sel = 3;
        } else {
            UHD_THROW_INVALID_CODE_PATH();
        }
    }

    //! Sets the PLL divider and multiplier values
    void _set_pll_div_and_mult(
        const double fTARGET, const double fVCO, const uint64_t fOSC_int)
    {
        // We want to avoid SYNC category 4 (device unreliable in SYNC mode) so
        // fix the pre-divider and multiplier to 1
        // See datasheet (Section 8.1.6)
        _regs.pll_r_pre = 1;
        _regs.mult      = 1;
        // Doubler fixed to disabled
        _regs.osc_2x = lmx2572_regs_t::osc_2x_t::OSC_2X_DISABLED;
        // Post-divider
        uint8_t pll_r = 0;
        // NOTE: This calculation is designed for the ZBX daughterboard. Should
        // we want to reuse this driver elsewhere, we need to factor this out
        // and make it a bit nicer.
        if (get_sync_mode()) {
            if (fTARGET < 3200e6) {
                switch (fOSC_int) {
                    case 61440000: {
                        if (3200e6 <= fVCO && fVCO < 3950e6) {
                            pll_r = 2;
                        } else if (3950e6 <= fVCO && fVCO <= 6400e6) {
                            pll_r = 1;
                        }
                        break;
                    }
                    case 64000000: {
                        if (3200e6 <= fVCO && fVCO < 4100e6) {
                            pll_r = 2;
                        } else if (4150e6 < fVCO && fVCO <= 6400e6) {
                            pll_r = 1;
                        }
                        break;
                    }
                    case 62500000: {
                        if (3200e6 <= fVCO && fVCO < 4000e6) {
                            pll_r = 2;
                        } else if (4050e6 <= fVCO && fVCO <= 6400e6) {
                            pll_r = 1;
                        }
                        break;
                    }
                    case 50000000: {
                        pll_r = 1;
                        break;
                    }
                    default:
                        UHD_THROW_INVALID_CODE_PATH();
                } // end switch
            } // end if (fTARGET < 3200e6)
            else {
                pll_r = 1;
            }
        } else {
            pll_r = 1;
        }
        UHD_ASSERT_THROW(pll_r > 0);
        _regs.pll_r = pll_r;
        // Section 7.3.2 states to not use both the double and the multiplier
        // (M), so let's check we're doing that
        UHD_ASSERT_THROW(
            _regs.mult == 1 || _regs.osc_2x == lmx2572_regs_t::osc_2x_t::OSC_2X_DISABLED);
    }

    //! Set the value of VCO_PHASE_SYNC_EN according to our sync category
    //
    // Assumption: outa_mux and outb_mux have already been appropriately
    // programmed for this use case.
    //
    // Also calculates the P-value (see set_frequency() for more discussion on
    // that value).
    int _set_phase_sync(const sync_cat cat)
    {
        int P = 1;
        // We always set the default value for VCO_PHASE_SYNC_EN here. Some
        // sync categories do not, in fact, require this bit to be asserted.
        // By resetting it here, we can exactly follow the datasheet in the
        // following switch statement.
        _regs.vco_phase_sync_en =
            lmx2572_regs_t::vco_phase_sync_en_t::VCO_PHASE_SYNC_EN_NORMAL_OPERATION;
        // This switch statement implements Table 137 from Section 8.1.6 of the
        // datasheet.
        switch (cat) {
            case CAT1A:
                UHD_LOG_TRACE(LOG_ID, "Sync Category: 1A");
                // Nothing required in this mode, input and output are always
                // at a deterministic phase relationship.
                break;
            case CAT1B:
                UHD_LOG_TRACE(LOG_ID, "Sync Category: 1B");
                // Set VCO_PHASE_SYNC_EN = 1
                _regs.vco_phase_sync_en = lmx2572_regs_t::vco_phase_sync_en_t::
                    VCO_PHASE_SYNC_EN_PHASE_SYNC_MODE;
                P = 2;
                break;
            case CAT2:
                UHD_LOG_TRACE(LOG_ID, "Sync Category: 2");
                // Note: We assume the existence and usage of the SYNC pin here.
                // This means there are no more steps required (Steps 3-6 are
                // skipped).
                break;
            case CAT3:
                UHD_LOG_TRACE(LOG_ID, "Sync Category: 3");
                // In this category, we assume that the SYNC signal will be
                // applied afterwards, and that timing requirements are met.
                if (_regs.outa_mux == lmx2572_regs_t::outa_mux_t::OUTA_MUX_CHANNEL_DIVIDER
                    || _regs.outb_mux
                           == lmx2572_regs_t::outb_mux_t::OUTB_MUX_CHANNEL_DIVIDER) {
                    P = 2;
                }
                _regs.vco_phase_sync_en = lmx2572_regs_t::vco_phase_sync_en_t::
                    VCO_PHASE_SYNC_EN_PHASE_SYNC_MODE;
                break;
            case CAT4:
                UHD_LOG_TRACE(LOG_ID, "Sync Category: 4");
                UHD_LOG_WARNING(LOG_ID,
                    "PLL programming does not allow reliable phase synchronization!");
                break;
            case NONE:
                // No phase sync, we're done
                break;
            default:
                UHD_THROW_INVALID_CODE_PATH();
        }
        return P;
    }

    //! Compute and set charge pump gain register
    // TODO: Charge pump settings will eventually come from a
    // lookup table in the Cal EEPROM for Charge Pump setting vs. F_CORE VCO_.
    void _compute_and_set_charge_pump_gain(const double fVCO_actual, const double N_real)
    {
        // clang-format off
        // Table 135 (VCO Gain)
        const std::map<
            double,
            std::tuple<double, double, uint8_t, uint8_t, uint8_t>
            > vco_gain_map {
            //   fmax     fmin    fmax    vco kmin kmax
                {3.65e9, {3.2e9,  3.65e9, 1,  32,  47}},
                {4.2e9,  {3.65e9, 4.2e9,  2,  35,  54}},
                {4.65e9, {4.2e9,  4.65e9, 3,  47,  64}},
                {5.2e9,  {4.65e9, 5.2e9,  4,  50,  73}},
                {5.75e9, {5.2e9,  5.75e9, 5,  61,  82}},
                {6.4e9,  {5.75e9, 6.4e9,  6,  57,  79}}
            };
        // clang-format on
        double fmin, fmax;
        int VCO_CORE;
        int KvcoMin, KvcoMax;
        auto vco_gain_it = vco_gain_map.lower_bound(fVCO_actual);
        UHD_ASSERT_THROW(vco_gain_it != vco_gain_map.end());
        std::tie(fmin, fmax, VCO_CORE, KvcoMin, KvcoMax) = vco_gain_it->second;
        double Kvco =
            uhd::math::linear_interp<double>(fVCO_actual, fmin, KvcoMin, fmax, KvcoMax);

        // Calculate the optimal charge pump current (uA)
        const double icp = 2 * uhd::math::PI * TARGET_LOOP_BANDWIDTH * N_real
                           / (Kvco * LOOP_GAIN_SETTING_RESISTANCE);

        // clang-format off
        // Table 2 (Charge Pump Gain)
        const std::map<double, uint8_t> cpg_map = {
        //   gain  cpg
            {   0,  0},
            { 625,  1},
            {1250,  2},
            {1875,  3},
            {2500,  4},
            {3125,  5},
            {3750,  6},
            {4375,  7},
            {5000, 12},
            {5625, 13},
            {6250, 14},
            {6875, 15}
        };
        // clang-format on
        const uint8_t cpg = uhd::math::at_nearest(cpg_map, icp);
        _regs.cpg         = cpg;
    }

    //! Compute and set VCO calibration values
    // This method implements VCO partial assist calibration
    // See datasheet (Section 8.1.4.1)
    void _compute_and_set_vco_cal(const double fVCO_actual)
    {
        // clang-format off
        // Table 136
        const std::map<
            double,
            std::tuple<double, double, uint8_t, uint8_t, uint8_t, uint16_t, uint16_t>
            > vco_partial_assist_map{
            //   fmax     fmin    fmax    vco Cmin Cmax Amin Amax
                {3.65e9, {3.2e9,  3.65e9, 1,  131, 19,  138, 137}},
                {4.2e9,  {3.65e9, 4.2e9,  2,  143, 25,  162, 142}},
                {4.65e9, {4.2e9,  4.65e9, 3,  135, 34,  126, 114}},
                {5.2e9,  {4.65e9, 5.2e9,  4,  136, 25,  195, 172}},
                {5.75e9, {5.2e9,  5.75e9, 5,  133, 20,  190, 163}},
                {6.4e9,  {5.75e9, 6.4e9,  6,  151, 27,  256, 204}}
            };
        // clang-format on
        double fmin, fmax;
        uint8_t VCO_CORE, Cmin, Cmax;
        uint16_t Amin, Amax;
        auto vco_cal_it = vco_partial_assist_map.lower_bound(fVCO_actual);
        UHD_ASSERT_THROW(vco_cal_it != vco_partial_assist_map.end());
        std::tie(fmin, fmax, VCO_CORE, Cmin, Cmax, Amin, Amax) = vco_cal_it->second;

        uint16_t VCO_CAPCTRL_STRT =
            std::round(Cmin - (fVCO_actual - fmin) * (Cmin - Cmax) / (fmax - fmin));
        // From R78 register field description (Table 86)
        const uint16_t VCO_CAPCTRL_STRT_MAX = 183;
        VCO_CAPCTRL_STRT = std::min(VCO_CAPCTRL_STRT_MAX, VCO_CAPCTRL_STRT);

        uint16_t VCO_DACISET_STRT =
            std::round(Amin - ((fVCO_actual - fmin) * (Amin - Amax) / (fmax - fmin)));
        // From R17 register field description (Table 25), 9-bit register
        const uint16_t VCO_DACISET_STRT_MAX = 511; // 0x1FF
        VCO_DACISET_STRT = std::min(VCO_DACISET_STRT, VCO_DACISET_STRT_MAX);

        _regs.vco_sel          = VCO_CORE;
        _regs.vco_capctrl_strt = VCO_CAPCTRL_STRT;
        _regs.vco_daciset_strt = VCO_DACISET_STRT;
    }

    void _set_pll_n(const uint32_t n)
    {
        UHD_ASSERT_THROW((n & 0x7FFFF) == n);
        // The regs object masks internally, this 0x7 is just for the sake of
        // reading
        _regs.pll_n_upper_3_bits  = uhd::narrow_cast<uint16_t>((n >> 16) & 0x7);
        _regs.pll_n_lower_16_bits = uhd::narrow_cast<uint16_t>(n);
    }

    void _set_pll_num(const uint32_t num)
    {
        _regs.pll_num_upper = uhd::narrow_cast<uint16_t>(num >> 16);
        _regs.pll_num_lower = uhd::narrow_cast<uint16_t>(num);
    }

    void _set_pll_den(const uint32_t den)
    {
        _regs.pll_den_upper = uhd::narrow_cast<uint16_t>(den >> 16);
        _regs.pll_den_lower = uhd::narrow_cast<uint16_t>(den);
    }


    // NOTE: Some of these defaults are just sensible defaults, and get
    // overwritten as soon as anything interesting happens. Other defaults are
    // specific to X400/ZBX. If we want to use this driver for other dboards,
    // we should add APIs to set those other things in order not to have a
    // leaky abstraction (we'd like to contain lmx2572_regs_t within this file).
    void _set_defaults()
    {
        _regs.ramp_en = lmx2572_regs_t::ramp_en_t::RAMP_EN_NORMAL_OPERATION;
        _regs.vco_phase_sync_en =
            lmx2572_regs_t::vco_phase_sync_en_t::VCO_PHASE_SYNC_EN_NORMAL_OPERATION;
        _regs.add_hold      = 0;
        _regs.out_mute      = lmx2572_regs_t::out_mute_t::OUT_MUTE_MUTED;
        _regs.fcal_hpfd_adj = 1;
        _regs.fcal_lpfd_adj = 0;
        _regs.fcal_en       = lmx2572_regs_t::fcal_en_t::FCAL_EN_ENABLE;
        _regs.muxout_ld_sel = lmx2572_regs_t::muxout_ld_sel_t::MUXOUT_LD_SEL_LOCK_DETECT;
        _regs.reset         = lmx2572_regs_t::reset_t::RESET_NORMAL_OPERATION;
        _regs.powerdown     = lmx2572_regs_t::powerdown_t::POWERDOWN_NORMAL_OPERATION;

        _regs.cal_clk_div = 0;

        _regs.ipbuf_type = lmx2572_regs_t::ipbuf_type_t::IPBUF_TYPE_DIFFERENTIAL;
        _regs.ipbuf_term = lmx2572_regs_t::ipbuf_term_t::IPBUF_TERM_INTERNALLY_TERMINATED;

        _regs.out_force = lmx2572_regs_t::out_force_t::OUT_FORCE_USE_OUT_MUTE;

        // set_frequency() implements VCO Partial assist so set the correct modes of
        // operation and some defaults (defaults will get overwritten)
        // See datasheet (Section 8.1.4)
        _regs.vco_daciset_force =
            lmx2572_regs_t::vco_daciset_force_t::VCO_DACISET_FORCE_NORMAL_OPERATION;
        _regs.vco_capctrl_force =
            lmx2572_regs_t::vco_capctrl_force_t::VCO_CAPCTRL_FORCE_NORMAL_OPERATION;
        _regs.vco_sel_force    = lmx2572_regs_t::vco_sel_force_t::VCO_SEL_FORCE_DISABLED;
        _regs.vco_daciset_strt = 0x096;
        _regs.vco_sel          = 0x6;
        _regs.vco_capctrl_strt = 0;

        _regs.mult_hi = lmx2572_regs_t::mult_hi_t::MULT_HI_LESS_THAN_EQUAL_TO_100M;
        _regs.osc_2x  = lmx2572_regs_t::osc_2x_t::OSC_2X_DISABLED;

        _regs.mult = 1;

        _regs.pll_r = 1;

        _regs.pll_r_pre = 1;

        _regs.cpg = 7;

        _regs.pll_n_upper_3_bits  = 0;
        _regs.pll_n_lower_16_bits = 0x28;

        _regs.mash_seed_en = lmx2572_regs_t::mash_seed_en_t::MASH_SEED_EN_ENABLED;
        _regs.pfd_dly_sel  = 0x2;

        _regs.pll_den_upper = 0;
        _regs.pll_den_lower = 0;

        _regs.mash_seed_upper = 0;
        _regs.mash_seed_lower = 0;

        _regs.pll_num_upper = 0;
        _regs.pll_num_lower = 0;

        _regs.outa_pwr = 0;
        _regs.outb_pd  = lmx2572_regs_t::outb_pd_t::OUTB_PD_POWER_DOWN;
        _regs.outa_pd  = lmx2572_regs_t::outa_pd_t::OUTA_PD_POWER_DOWN;
        _regs.mash_reset_n =
            lmx2572_regs_t::mash_reset_n_t::MASH_RESET_N_NORMAL_OPERATION;
        _regs.mash_order = lmx2572_regs_t::mash_order_t::MASH_ORDER_THIRD_ORDER;

        _regs.outa_mux = lmx2572_regs_t::outa_mux_t::OUTA_MUX_VCO;
        _regs.outb_pwr = 0;

        _regs.outb_mux = lmx2572_regs_t::outb_mux_t::OUTB_MUX_VCO;

        _regs.inpin_ignore = 0;
        _regs.inpin_hyst   = lmx2572_regs_t::inpin_hyst_t::INPIN_HYST_DISABLED;
        _regs.inpin_lvl    = lmx2572_regs_t::inpin_lvl_t::INPIN_LVL_INVALID;
        _regs.inpin_fmt =
            lmx2572_regs_t::inpin_fmt_t::INPIN_FMT_SYNC_EQUALS_SYSREFREQ_EQUALS_CMOS2;

        _regs.ld_type = lmx2572_regs_t::ld_type_t::LD_TYPE_VTUNE_AND_VCOCAL;

        _regs.ld_dly = 100;
        // See Table 144 (LDO_DLY Setting)
        _regs.ldo_dly = 3;

        _regs.dblbuf_en_0 = lmx2572_regs_t::dblbuf_en_0_t::DBLBUF_EN_0_ENABLED;
        _regs.dblbuf_en_1 = lmx2572_regs_t::dblbuf_en_1_t::DBLBUF_EN_1_ENABLED;
        _regs.dblbuf_en_2 = lmx2572_regs_t::dblbuf_en_2_t::DBLBUF_EN_2_ENABLED;
        _regs.dblbuf_en_3 = lmx2572_regs_t::dblbuf_en_3_t::DBLBUF_EN_3_ENABLED;
        _regs.dblbuf_en_4 = lmx2572_regs_t::dblbuf_en_4_t::DBLBUF_EN_4_ENABLED;
        _regs.dblbuf_en_5 = lmx2572_regs_t::dblbuf_en_5_t::DBLBUF_EN_5_ENABLED;

        _regs.mash_rst_count_upper = 0;
        _regs.mash_rst_count_lower = 0;

        // Always gets set by set_frequency()
        _regs.chdiv = lmx2572_regs_t::chdiv_t::CHDIV_DIVIDE_BY_2;

        _regs.ramp_thresh_33rd = 0;
        _regs.quick_recal_en   = 0;

        _regs.ramp_thresh_upper = 0;
        _regs.ramp_thresh_lower = 0;

        _regs.ramp_limit_high_33rd  = 0;
        _regs.ramp_limit_high_upper = 0;
        _regs.ramp_limit_high_lower = 0;

        _regs.ramp_limit_low_33rd  = 0;
        _regs.ramp_limit_low_upper = 0;
        _regs.ramp_limit_low_lower = 0;

        // Per the datasheet, the following fields need to be programmed to specific
        // constants which differ from the defaults after a reset occurs
        _regs.reg29_reserved0 = 0;
        _regs.reg30_reserved0 = 0x18A6;
        _regs.reg52_reserved0 = 0x421;
        _regs.reg57_reserved0 = 0x20;
        _regs.reg78_reserved0 = 1;
    }
};

lmx2572_iface::sptr lmx2572_iface::make(lmx2572_iface::write_fn_t&& poke_fn,
    lmx2572_iface::read_fn_t&& peek_fn,
    lmx2572_iface::sleep_fn_t&& sleep_fn)
{
    return std::make_shared<lmx2572_impl>(
        std::move(poke_fn), std::move(peek_fn), std::move(sleep_fn));
}
