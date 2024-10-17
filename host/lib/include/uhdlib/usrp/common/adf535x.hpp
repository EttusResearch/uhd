//
// Copyright 2015, 2017 Ettus Research, A National Instruments Company
// Copyright 2019 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include "adf5355_regs.hpp"
#include "adf5356_regs.hpp"
#include <uhd/types/ranges.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/math.hpp>
#include <uhd/utils/safe_call.hpp>
#include <uhdlib/utils/narrow.hpp>
#include <stdint.h>
#include <boost/format.hpp>
#include <algorithm>
#include <functional>
#include <iomanip>
#include <numeric>
#include <utility>
#include <vector>

class adf535x_iface
{
public:
    typedef std::shared_ptr<adf535x_iface> sptr;
    typedef std::function<void(std::vector<uint32_t>)> write_fn_t;
    typedef std::function<void(uint32_t)> wait_fn_t;

    static sptr make_adf5355(write_fn_t write, wait_fn_t wait);
    static sptr make_adf5356(write_fn_t write, wait_fn_t wait);

    virtual ~adf535x_iface() = default;

    enum output_t { RF_OUTPUT_A, RF_OUTPUT_B };

    enum feedback_sel_t { FB_SEL_FUNDAMENTAL, FB_SEL_DIVIDED };

    enum output_power_t {
        OUTPUT_POWER_M4DBM,
        OUTPUT_POWER_M1DBM,
        OUTPUT_POWER_2DBM,
        OUTPUT_POWER_5DBM
    };

    enum muxout_t {
        MUXOUT_3STATE,
        MUXOUT_DVDD,
        MUXOUT_DGND,
        MUXOUT_RDIV,
        MUXOUT_NDIV,
        MUXOUT_ALD,
        MUXOUT_DLD
    };

    virtual void set_reference_freq(const double fref, const bool force = false) = 0;

    virtual void set_pfd_freq(const double pfd_freq) = 0;

    virtual void set_feedback_select(const feedback_sel_t fb_sel) = 0;

    virtual void set_output_power(const output_power_t power) = 0;

    virtual void set_output_enable(const output_t output, const bool enable) = 0;

    virtual void set_muxout_mode(const muxout_t mode) = 0;

    virtual double set_frequency(
        const double target_freq, const uint32_t mod2 = 2, const bool flush = false) = 0;

    virtual double set_charge_pump_current(
        const double target_current, const bool flush = false) = 0;

    virtual uhd::meta_range_t get_charge_pump_current_range() = 0;

    virtual void commit() = 0;
};

using namespace uhd;

namespace {
const double ADF535X_DOUBLER_MAX_REF_FREQ = 60e6;
const double ADF535X_MAX_FREQ_PFD         = 125e6;
// const double ADF535X_PRESCALER_THRESH        = 7e9;

const double ADF535X_MIN_VCO_FREQ = 3.4e9;
// const double ADF535X_MAX_VCO_FREQ            = 6.8e9;
const double ADF535X_MAX_OUT_FREQ = 6.8e9;
const double ADF535X_MIN_OUT_FREQ = (3.4e9 / 64);
// const double ADF535X_MAX_OUTB_FREQ           = (6.8e9 * 2);
// const double ADF535X_MIN_OUTB_FREQ           = (3.4e9 * 2);

const double ADF535X_PHASE_RESYNC_TIME = 400e-6;

const uint32_t ADF535X_MOD1      = 16777216;
const uint32_t ADF5355_MAX_MOD2  = 16383;
const uint32_t ADF5355_MAX_FRAC2 = 16383;
const uint32_t ADF5356_MAX_MOD2  = 268435455;
const uint32_t ADF5356_MAX_FRAC2 = 268435455;
// const uint16_t ADF535X_MIN_INT_PRESCALER_89 = 75;
} // namespace

template <typename adf535x_regs_t>
class adf535x_impl : public adf535x_iface
{
public:
    explicit adf535x_impl(write_fn_t write_fn, wait_fn_t wait_fn)
        : _write_fn(std::move(write_fn))
        , _wait_fn(std::move(wait_fn))
        , _regs()
        , _rewrite_regs(true)
        , _wait_time_us(0)
        , _ref_freq(0.0)
        , _pfd_freq(0.0)
        , _fb_after_divider(true)
    {
        _regs.vco_band_div       = 3;
        _regs.timeout            = 11;
        _regs.auto_level_timeout = 30;
        _regs.synth_lock_timeout = 12;

        _regs.adc_clock_divider = 16;
        _regs.adc_conversion    = adf535x_regs_t::ADC_CONVERSION_ENABLED;
        _regs.adc_enable        = adf535x_regs_t::ADC_ENABLE_ENABLED;

        // Start with phase resync disabled and enable when reference clock is set
        _regs.phase_resync = adf535x_regs_t::PHASE_RESYNC_DISABLED;

        set_feedback_select(FB_SEL_DIVIDED);
    }

    ~adf535x_impl() override
    {
        UHD_SAFE_CALL(_regs.power_down = adf535x_regs_t::POWER_DOWN_ENABLED; commit();)
    }

    void set_feedback_select(const feedback_sel_t fb_sel) override
    {
        _fb_after_divider = (fb_sel == FB_SEL_DIVIDED);
    }

    void set_pfd_freq(const double pfd_freq) override
    {
        if (pfd_freq > ADF535X_MAX_FREQ_PFD) {
            UHD_LOGGER_ERROR("ADF535x")
                << boost::format("%f MHz is above the maximum PFD frequency of %f MHz\n")
                       % (pfd_freq / 1e6) % (ADF535X_MAX_FREQ_PFD / 1e6);
            return;
        }
        _pfd_freq = pfd_freq;

        set_reference_freq(_ref_freq);
    }

    void set_reference_freq(const double fref, const bool force = false) override
    {
        // Skip the body if the reference frequency does not change
        if (uhd::math::frequencies_are_equal(fref, _ref_freq) and (not force))
            return;

        _ref_freq = fref;

        //-----------------------------------------------------------
        // Set reference settings
        int ref_div_factor = static_cast<int>(std::floor(_ref_freq / _pfd_freq));

        // Reference doubler for 50% duty cycle
        const bool doubler_en = (_ref_freq <= ADF535X_DOUBLER_MAX_REF_FREQ);
        if (doubler_en) {
            ref_div_factor *= 2;
        }

        // Reference divide-by-2 for 50% duty cycle
        // if R even, move one divide by 2 to regs.reference_divide_by_2
        const bool div2_en = (ref_div_factor % 2 == 0);
        if (div2_en) {
            ref_div_factor /= 2;
        }

        _regs.reference_divide_by_2 =
            div2_en ? adf535x_regs_t::REFERENCE_DIVIDE_BY_2_ENABLED
                    : adf535x_regs_t::REFERENCE_DIVIDE_BY_2_DISABLED;
        _regs.reference_doubler = doubler_en ? adf535x_regs_t::REFERENCE_DOUBLER_ENABLED
                                             : adf535x_regs_t::REFERENCE_DOUBLER_DISABLED;
        _regs.r_counter_10_bit  = ref_div_factor;
        UHD_ASSERT_THROW((_regs.r_counter_10_bit & ((uint16_t)~0x3FF)) == 0);

        //-----------------------------------------------------------
        // Set timeouts (code from ADI driver)
        _regs.timeout = std::max(1, std::min(int(ceil(_pfd_freq / (20e3 * 30))), 1023));

        UHD_ASSERT_THROW((_regs.timeout & ((uint16_t)~0x3FF)) == 0);
        _regs.synth_lock_timeout =
            static_cast<uint8_t>(ceil((_pfd_freq * 2) / (100e3 * _regs.timeout)));
        UHD_ASSERT_THROW((_regs.synth_lock_timeout & ((uint16_t)~0x1F)) == 0);
        _regs.auto_level_timeout =
            static_cast<uint8_t>(ceil((_pfd_freq * 5) / (100e3 * _regs.timeout)));

        //-----------------------------------------------------------
        // Set VCO band divider
        _regs.vco_band_div = _set_vco_band_div(_pfd_freq);

        //-----------------------------------------------------------
        // Set ADC delay (code from ADI driver)
        _regs.adc_enable     = adf535x_regs_t::ADC_ENABLE_ENABLED;
        _regs.adc_conversion = adf535x_regs_t::ADC_CONVERSION_ENABLED;
        _regs.adc_clock_divider =
            std::max(1, std::min(int(ceil(((_pfd_freq / 100e3) - 2) / 4)), 255));

        _wait_time_us = static_cast<uint32_t>(
            ceil(16e6 / (_pfd_freq / ((4 * _regs.adc_clock_divider) + 2))));

        //-----------------------------------------------------------
        // Phase resync
        _regs.phase_resync = adf535x_regs_t::PHASE_RESYNC_ENABLED;

        _regs.phase_adjust  = adf535x_regs_t::PHASE_ADJUST_DISABLED;
        _regs.sd_load_reset = adf535x_regs_t::SD_LOAD_RESET_ON_REG0_UPDATE;
        _regs.phase_resync_clk_div =
            static_cast<uint16_t>(floor(ADF535X_PHASE_RESYNC_TIME * _pfd_freq));

        _rewrite_regs = true;
    }

    double set_frequency(const double target_freq,
        const uint32_t mod2 = 2,
        const bool flush    = false) override
    {
        return _set_frequency(target_freq, mod2, flush);
    }

    double set_charge_pump_current(const double current, const bool flush) override
    {
        const auto cp_range = get_charge_pump_current_range();

        const auto coerced_current = cp_range.clip(current, true);
        const int current_step =
            uhd::narrow_cast<int>(std::round((coerced_current / cp_range.step()) - 1));

        UHD_ASSERT_THROW(current_step >= 0 and current_step < 16);
        _regs.charge_pump_current =
            static_cast<typename adf535x_regs_t::charge_pump_current_t>(current_step);

        if (flush) {
            commit();
        }

        if (std::abs(current - coerced_current) > 0.01e-6) {
            UHD_LOG_WARNING("ADF535x",
                "Requested charge pump current was coerced! Requested: " << std::setw(
                    4) << current << " A  Actual: " << coerced_current << " A");
        }

        return coerced_current;
    }

    uhd::meta_range_t get_charge_pump_current_range() override
    {
        return _get_charge_pump_current_range();
    }

    void set_output_power(const output_power_t power) override
    {
        typename adf535x_regs_t::output_power_t setting;
        switch (power) {
            case OUTPUT_POWER_M4DBM:
                setting = adf535x_regs_t::OUTPUT_POWER_M4DBM;
                break;
            case OUTPUT_POWER_M1DBM:
                setting = adf535x_regs_t::OUTPUT_POWER_M1DBM;
                break;
            case OUTPUT_POWER_2DBM:
                setting = adf535x_regs_t::OUTPUT_POWER_2DBM;
                break;
            case OUTPUT_POWER_5DBM:
                setting = adf535x_regs_t::OUTPUT_POWER_5DBM;
                break;
            default:
                UHD_THROW_INVALID_CODE_PATH();
        }
        if (_regs.output_power != setting)
            _rewrite_regs = true;
        _regs.output_power = setting;
    }

    void set_output_enable(const output_t output, const bool enable) override
    {
        switch (output) {
            case RF_OUTPUT_A:
                _regs.rf_out_a_enabled = enable
                                             ? adf535x_regs_t::RF_OUT_A_ENABLED_ENABLED
                                             : adf535x_regs_t::RF_OUT_A_ENABLED_DISABLED;
                break;
            case RF_OUTPUT_B:
                _regs.rf_out_b_enabled = enable
                                             ? adf535x_regs_t::RF_OUT_B_ENABLED_ENABLED
                                             : adf535x_regs_t::RF_OUT_B_ENABLED_DISABLED;
                break;
        }
    }

    void set_muxout_mode(const muxout_t mode) override
    {
        switch (mode) {
            case MUXOUT_3STATE:
                _regs.muxout = adf535x_regs_t::MUXOUT_3STATE;
                break;
            case MUXOUT_DVDD:
                _regs.muxout = adf535x_regs_t::MUXOUT_DVDD;
                break;
            case MUXOUT_DGND:
                _regs.muxout = adf535x_regs_t::MUXOUT_DGND;
                break;
            case MUXOUT_RDIV:
                _regs.muxout = adf535x_regs_t::MUXOUT_RDIV;
                break;
            case MUXOUT_NDIV:
                _regs.muxout = adf535x_regs_t::MUXOUT_NDIV;
                break;
            case MUXOUT_ALD:
                _regs.muxout = adf535x_regs_t::MUXOUT_ANALOG_LD;
                break;
            case MUXOUT_DLD:
                _regs.muxout = adf535x_regs_t::MUXOUT_DLD;
                break;
            default:
                UHD_THROW_INVALID_CODE_PATH();
        }
    }

    void commit() override
    {
        _commit();
    }

protected:
    uint8_t _set_vco_band_div(double);
    double _set_frequency(double, uint32_t, bool);
    uhd::meta_range_t _get_charge_pump_current_range();
    void _commit();

private: // Members
    typedef std::vector<uint32_t> addr_vtr_t;

    write_fn_t _write_fn;
    wait_fn_t _wait_fn;
    adf535x_regs_t _regs;
    bool _rewrite_regs;
    uint32_t _wait_time_us;
    double _ref_freq;
    double _pfd_freq;
    bool _fb_after_divider;
};

// ADF5355 Functions
template <>
inline uint8_t adf535x_impl<adf5355_regs_t>::_set_vco_band_div(double pfd_freq)
{
    return static_cast<uint8_t>(ceil(pfd_freq / 2.4e6));
}

template <>
inline double adf535x_impl<adf5355_regs_t>::_set_frequency(
    double target_freq, uint32_t mod2, bool flush)
{
    if (target_freq > ADF535X_MAX_OUT_FREQ or target_freq < ADF535X_MIN_OUT_FREQ) {
        throw uhd::runtime_error("requested frequency out of range.");
    }
    if (mod2 < 2 or mod2 > ADF5355_MAX_MOD2) {
        throw uhd::runtime_error("requested mod2 out of range.");
    }

    /* Calculate target VCOout frequency */
    // Increase RF divider until acceptable VCO frequency
    double target_vco_freq = target_freq;
    uint32_t rf_divider    = 1;
    while (target_vco_freq < ADF535X_MIN_VCO_FREQ && rf_divider < 64) {
        target_vco_freq *= 2;
        rf_divider *= 2;
    }

    switch (rf_divider) {
        case 1:
            _regs.rf_divider_select = adf5355_regs_t::RF_DIVIDER_SELECT_DIV1;
            break;
        case 2:
            _regs.rf_divider_select = adf5355_regs_t::RF_DIVIDER_SELECT_DIV2;
            break;
        case 4:
            _regs.rf_divider_select = adf5355_regs_t::RF_DIVIDER_SELECT_DIV4;
            break;
        case 8:
            _regs.rf_divider_select = adf5355_regs_t::RF_DIVIDER_SELECT_DIV8;
            break;
        case 16:
            _regs.rf_divider_select = adf5355_regs_t::RF_DIVIDER_SELECT_DIV16;
            break;
        case 32:
            _regs.rf_divider_select = adf5355_regs_t::RF_DIVIDER_SELECT_DIV32;
            break;
        case 64:
            _regs.rf_divider_select = adf5355_regs_t::RF_DIVIDER_SELECT_DIV64;
            break;
        default:
            UHD_THROW_INVALID_CODE_PATH();
    }

    // Compute fractional PLL params
    double prescaler_input_freq = target_vco_freq;
    if (_fb_after_divider) {
        prescaler_input_freq /= rf_divider;
    }

    const double N       = prescaler_input_freq / _pfd_freq;
    const auto INT       = static_cast<uint16_t>(floor(N));
    const auto FRAC1     = static_cast<uint32_t>(floor((N - INT) * ADF535X_MOD1));
    const double residue = (N - INT) * ADF535X_MOD1 - FRAC1;

    // The data sheet recommends reducing FRAC2 and MOD2 to the lowest possible values
    const uint16_t frac2 = static_cast<uint16_t>(
        std::min(ceil(residue * mod2), static_cast<double>(ADF5355_MAX_FRAC2)));
    const auto gcd = std::gcd(static_cast<int>(frac2), static_cast<int>(mod2));
    // FRAC2 and MOD2 are 14-bit numbers
    const uint16_t FRAC2 = (frac2 == 0) ? 0 : frac2 / gcd;
    const uint16_t MOD2  = (frac2 == 0) ? 2 : mod2 / gcd;

    const double coerced_vco_freq =
        _pfd_freq * (_fb_after_divider ? rf_divider : 1)
        * (double(INT)
            + ((double(FRAC1) + (double(FRAC2) / double(MOD2))) / double(ADF535X_MOD1)));

    const double coerced_out_freq = coerced_vco_freq / rf_divider;

    UHD_LOG_TRACE("ADF5355",
        boost::format("ADF5355 Frequencies (MHz): Requested=%f "
                      "Actual=%f TargetVCO=%f ActualVCO=%f")
            % (target_freq / 1e6) % (coerced_out_freq / 1e6) % (target_vco_freq / 1e6)
            % (coerced_vco_freq / 1e6));
    UHD_LOG_TRACE("ADF5355",
        boost::format("ADF5355 Settings: N=%f INT=%d FRAC1=%u MOD2=%d FRAC2=%u") % N % INT
            % FRAC1 % MOD2 % FRAC2);

    /* Update registers */
    if ((rf_divider == 1) or not _fb_after_divider) {
        _regs.feedback_select = adf5355_regs_t::FEEDBACK_SELECT_FUNDAMENTAL;
    } else {
        _regs.feedback_select = adf5355_regs_t::FEEDBACK_SELECT_DIVIDED;
    }
    _regs.int_16_bit   = INT;
    _regs.frac1_24_bit = FRAC1;
    _regs.frac2_14_bit = FRAC2;
    _regs.mod2_14_bit  = MOD2;
    _regs.phase_24_bit = 0;

    if (flush)
        commit();
    return coerced_out_freq;
}

template <>
inline uhd::meta_range_t adf535x_impl<adf5355_regs_t>::_get_charge_pump_current_range()
{
    return uhd::meta_range_t(.3125e-6, 5e-6, .3125e-6);
}

template <>
inline void adf535x_impl<adf5355_regs_t>::_commit()
{
    const size_t ONE_REG = 1;

    if (_rewrite_regs) {
        // For a full state sync write registers in reverse order 12 - 0
        addr_vtr_t regs;
        for (uint8_t addr = 12; addr > 0; addr--) {
            regs.push_back(_regs.get_reg(addr));
        }
        _write_fn(regs);
        _wait_fn(_wait_time_us);
        _write_fn(addr_vtr_t(ONE_REG, _regs.get_reg(0)));
        _rewrite_regs = false;
    } else {
        // Frequency update sequence from data sheet
        _write_fn(addr_vtr_t(ONE_REG, _regs.get_reg(6)));
        _regs.counter_reset = adf5355_regs_t::COUNTER_RESET_ENABLED;
        _write_fn(addr_vtr_t(ONE_REG, _regs.get_reg(4)));
        _write_fn(addr_vtr_t(ONE_REG, _regs.get_reg(2)));
        _write_fn(addr_vtr_t(ONE_REG, _regs.get_reg(1)));
        _regs.autocal_en = adf5355_regs_t::AUTOCAL_EN_DISABLED;
        _write_fn(addr_vtr_t(ONE_REG, _regs.get_reg(0)));
        _regs.counter_reset = adf5355_regs_t::COUNTER_RESET_DISABLED;
        _write_fn(addr_vtr_t(ONE_REG, _regs.get_reg(4)));
        _regs.autocal_en = adf5355_regs_t::AUTOCAL_EN_ENABLED;
        _write_fn(addr_vtr_t(ONE_REG, _regs.get_reg(0)));
    }
}

// ADF5356 Functions
template <>
inline uint8_t adf535x_impl<adf5356_regs_t>::_set_vco_band_div(double pfd_freq)
{
    return static_cast<uint8_t>(ceil(pfd_freq / 1.6e6));
}

template <>
inline double adf535x_impl<adf5356_regs_t>::_set_frequency(
    double target_freq, uint32_t mod2, bool flush)
{
    if (target_freq > ADF535X_MAX_OUT_FREQ or target_freq < ADF535X_MIN_OUT_FREQ) {
        throw uhd::runtime_error("requested frequency out of range.");
    }
    if (mod2 < 2 or mod2 > ADF5355_MAX_MOD2) {
        throw uhd::runtime_error("requested mod2 out of range.");
    }

    /* Calculate target VCOout frequency */
    // Increase RF divider until acceptable VCO frequency
    double target_vco_freq = target_freq;
    uint32_t rf_divider    = 1;
    while (target_vco_freq < ADF535X_MIN_VCO_FREQ && rf_divider < 64) {
        target_vco_freq *= 2;
        rf_divider *= 2;
    }

    switch (rf_divider) {
        case 1:
            _regs.rf_divider_select = adf5356_regs_t::RF_DIVIDER_SELECT_DIV1;
            break;
        case 2:
            _regs.rf_divider_select = adf5356_regs_t::RF_DIVIDER_SELECT_DIV2;
            break;
        case 4:
            _regs.rf_divider_select = adf5356_regs_t::RF_DIVIDER_SELECT_DIV4;
            break;
        case 8:
            _regs.rf_divider_select = adf5356_regs_t::RF_DIVIDER_SELECT_DIV8;
            break;
        case 16:
            _regs.rf_divider_select = adf5356_regs_t::RF_DIVIDER_SELECT_DIV16;
            break;
        case 32:
            _regs.rf_divider_select = adf5356_regs_t::RF_DIVIDER_SELECT_DIV32;
            break;
        case 64:
            _regs.rf_divider_select = adf5356_regs_t::RF_DIVIDER_SELECT_DIV64;
            break;
        default:
            UHD_THROW_INVALID_CODE_PATH();
    }

    // Compute fractional PLL params
    double prescaler_input_freq = target_vco_freq;
    if (_fb_after_divider) {
        prescaler_input_freq /= rf_divider;
    }

    const double N       = prescaler_input_freq / _pfd_freq;
    const auto INT       = static_cast<uint16_t>(floor(N));
    const auto FRAC1     = static_cast<uint32_t>(floor((N - INT) * ADF535X_MOD1));
    const double residue = (N - INT) * ADF535X_MOD1 - FRAC1;

    // The data sheet recommends reducing FRAC2 and MOD2 to the lowest possible values
    const uint32_t frac2 = static_cast<uint32_t>(
        std::min(ceil(residue * mod2), static_cast<double>(ADF5356_MAX_FRAC2)));
    const auto gcd = std::gcd(static_cast<int>(frac2), static_cast<int>(mod2));
    // FRAC2 and MOD2 are 28-bit numbers
    const uint32_t FRAC2 = (frac2 == 0) ? 0 : frac2 / gcd;
    const uint32_t MOD2  = (frac2 == 0) ? 2 : mod2 / gcd;

    const double coerced_vco_freq =
        _pfd_freq * (_fb_after_divider ? rf_divider : 1)
        * (double(INT)
            + ((double(FRAC1) + (double(FRAC2) / double(MOD2))) / double(ADF535X_MOD1)));

    const double coerced_out_freq = coerced_vco_freq / rf_divider;

    UHD_LOG_TRACE("ADF5356",
        boost::format("ADF5356 Frequencies (MHz): Requested=%f "
                      "Actual=%f TargetVCO=%f ActualVCO=%f")
            % (target_freq / 1e6) % (coerced_out_freq / 1e6) % (target_vco_freq / 1e6)
            % (coerced_vco_freq / 1e6));
    UHD_LOG_TRACE("ADF5356",
        boost::format("ADF5356 Settings: N=%f INT=%d FRAC1=%u MOD2=%d FRAC2=%u") % N % INT
            % FRAC1 % MOD2 % FRAC2);

    /* Update registers */
    if ((rf_divider == 1) or not _fb_after_divider) {
        _regs.feedback_select = adf5356_regs_t::FEEDBACK_SELECT_FUNDAMENTAL;
    } else {
        _regs.feedback_select = adf5356_regs_t::FEEDBACK_SELECT_DIVIDED;
    }
    _regs.int_16_bit   = INT;
    _regs.frac1_24_bit = FRAC1;
    _regs.frac2_lsb    = narrow_cast<uint16_t>(FRAC2 & 0x3FFF);
    _regs.mod2_lsb     = narrow_cast<uint16_t>(MOD2 & 0x3FFF);
    _regs.frac2_msb    = narrow_cast<uint16_t>(FRAC2 >> 14);
    _regs.mod2_msb     = narrow_cast<uint16_t>(MOD2 >> 14);
    _regs.phase_24_bit = 0;

    _regs.negative_bleed = FRAC1 != 0 or FRAC2 != 0
                               ? adf5356_regs_t::NEGATIVE_BLEED_ENABLED
                               : adf5356_regs_t::NEGATIVE_BLEED_DISABLED;

    if (flush)
        commit();
    return coerced_out_freq;
}

template <>
inline uhd::meta_range_t adf535x_impl<adf5356_regs_t>::_get_charge_pump_current_range()
{
    return uhd::meta_range_t(.3e-6, 4.8e-6, .3e-6);
}

template <>
inline void adf535x_impl<adf5356_regs_t>::_commit()
{
    const size_t ONE_REG = 1;
    if (_rewrite_regs) {
        // For a full state sync write registers in reverse order 12 - 0
        addr_vtr_t regs;
        for (uint8_t addr = 13; addr > 0; addr--) {
            regs.push_back(_regs.get_reg(addr));
        }
        _write_fn(regs);
        _wait_fn(_wait_time_us);
        _write_fn(addr_vtr_t(ONE_REG, _regs.get_reg(0)));
        _rewrite_regs = false;
    } else {
        // Frequency update sequence from data sheet
        _write_fn(addr_vtr_t(ONE_REG, _regs.get_reg(13)));
        _write_fn(addr_vtr_t(ONE_REG, _regs.get_reg(10)));
        _write_fn(addr_vtr_t(ONE_REG, _regs.get_reg(6)));
        _write_fn(addr_vtr_t(ONE_REG, _regs.get_reg(2)));
        _write_fn(addr_vtr_t(ONE_REG, _regs.get_reg(1)));
        _wait_fn(_wait_time_us);
        _write_fn(addr_vtr_t(ONE_REG, _regs.get_reg(0)));
    }
}
