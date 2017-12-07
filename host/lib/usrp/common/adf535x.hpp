//
// Copyright 2015, 2017 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0
//

#ifndef INCLUDED_ADF535X_HPP
#define INCLUDED_ADF535X_HPP

#include "adf5355_regs.hpp"
#include "adf5356_regs.hpp"
#include <uhd/utils/math.hpp>
#include <uhd/utils/log.hpp>
#include <boost/function.hpp>
#include <boost/math/common_factor_rt.hpp> //gcd
#include <utility>
#include <vector>
#include <algorithm>
#include <stdint.h>
#include <boost/format.hpp>
#include <uhd/utils/safe_call.hpp>

class adf535x_iface
{
public:
    typedef boost::shared_ptr<adf535x_iface> sptr;
    typedef boost::function<void(std::vector<uint32_t>)> write_fn_t;

    static sptr make_adf5355(write_fn_t write);
    static sptr make_adf5356(write_fn_t write);

    virtual ~adf535x_iface() {};

    enum output_t { RF_OUTPUT_A, RF_OUTPUT_B };

    enum feedback_sel_t { FB_SEL_FUNDAMENTAL, FB_SEL_DIVIDED };

    enum output_power_t { OUTPUT_POWER_M4DBM, OUTPUT_POWER_M1DBM, OUTPUT_POWER_2DBM, OUTPUT_POWER_5DBM };

    enum muxout_t { MUXOUT_3STATE, MUXOUT_DVDD, MUXOUT_DGND, MUXOUT_RDIV, MUXOUT_NDIV, MUXOUT_ALD, MUXOUT_DLD };

    virtual void set_reference_freq(double fref, bool force = false) = 0;

    virtual void set_pfd_freq(double pfd_freq) = 0;

    virtual void set_feedback_select(feedback_sel_t fb_sel) = 0;

    virtual void set_output_power(output_power_t power) = 0;

    virtual void set_output_enable(output_t output, bool enable) = 0;

    virtual void set_muxout_mode(muxout_t mode) = 0;

    virtual double set_frequency(double target_freq, double freq_resolution, bool flush = false) = 0;

    virtual void commit() = 0;
};

using namespace uhd;

namespace {
  const double ADF535X_DOUBLER_MAX_REF_FREQ    = 60e6;
  const double ADF535X_MAX_FREQ_PFD            = 125e6;
//const double ADF535X_PRESCALER_THRESH        = 7e9;

  const double ADF535X_MIN_VCO_FREQ            = 3.4e9;
//const double ADF535X_MAX_VCO_FREQ            = 6.8e9;
  const double ADF535X_MAX_OUT_FREQ            = 6.8e9;
  const double ADF535X_MIN_OUT_FREQ            = (3.4e9 / 64);
//const double ADF535X_MAX_OUTB_FREQ           = (6.8e9 * 2);
//const double ADF535X_MIN_OUTB_FREQ           = (3.4e9 * 2);

  const double ADF535X_PHASE_RESYNC_TIME       = 400e-6;

  const uint32_t ADF535X_MOD1           = 16777216;
  const uint32_t ADF535X_MAX_MOD2       = 16383;
  const uint32_t ADF535X_MAX_FRAC2      = 16383;
//const uint16_t ADF535X_MIN_INT_PRESCALER_89 = 75;
}

template <typename adf535x_regs_t>
class adf535x_impl : public adf535x_iface
{
public:
  explicit adf535x_impl(write_fn_t write_fn) :
          _write_fn(write_fn),
          _regs(),
          _rewrite_regs(true),
          _wait_time_us(0),
          _ref_freq(0.0),
          _pfd_freq(0.0),
          _fb_after_divider(false)
  {

    _regs.vco_band_div = 3;
    _regs.timeout = 11;
    _regs.auto_level_timeout = 30;
    _regs.synth_lock_timeout = 12;

    _regs.adc_clock_divider = 16;
    _regs.adc_conversion = adf535x_regs_t::ADC_CONVERSION_ENABLED;
    _regs.adc_enable = adf535x_regs_t::ADC_ENABLE_ENABLED;

    // TODO Needs to be enabled for phase resync
    _regs.phase_resync = adf535x_regs_t::PHASE_RESYNC_DISABLED;

    // TODO Default should be divided, but there seems to be a bug preventing that. Needs rechecking
    _regs.feedback_select = adf535x_regs_t::FEEDBACK_SELECT_FUNDAMENTAL;

    // TODO 0 is an invalid value for this field. Setting to 1 seemed to break phase sync, needs retesting.
    _regs.phase_resync_clk_div = 0;
  }

  ~adf535x_impl()
  {
    UHD_SAFE_CALL(
      _regs.power_down = adf535x_regs_t::POWER_DOWN_ENABLED;
      commit();
    )
  }

  void set_feedback_select(const feedback_sel_t fb_sel)
  {
    _fb_after_divider = (fb_sel == FB_SEL_DIVIDED);

    if (_fb_after_divider) {
      _regs.feedback_select = adf535x_regs_t::FEEDBACK_SELECT_DIVIDED;
    } else {
      _regs.feedback_select = adf535x_regs_t::FEEDBACK_SELECT_FUNDAMENTAL;
    }
  }

  void set_pfd_freq(const double pfd_freq)
  {
    if (pfd_freq > ADF535X_MAX_FREQ_PFD) {
      UHD_MSG(error) << boost::format("ADF535x: %f MHz is above the maximum PFD frequency of %f MHz\n")
                % (pfd_freq/1e6) % (ADF535X_MAX_FREQ_PFD/1e6);
      return;
    }
    _pfd_freq = pfd_freq;

    set_reference_freq(_ref_freq);
  }

  void set_reference_freq(const double fref, const bool force = false)
  {
    //Skip the body if the reference frequency does not change
    if (uhd::math::frequencies_are_equal(fref, _ref_freq) and (not force))
      return;

    _ref_freq = fref;

    //-----------------------------------------------------------
    //Set reference settings
    int ref_div_factor = static_cast<int>(std::floor(_ref_freq / _pfd_freq));

    //Reference doubler for 50% duty cycle
    const bool doubler_en = (_ref_freq <= ADF535X_DOUBLER_MAX_REF_FREQ);
    if (doubler_en) {
      ref_div_factor *= 2;
    }

    //Reference divide-by-2 for 50% duty cycle
    // if R even, move one divide by 2 to to regs.reference_divide_by_2
    const bool div2_en = (ref_div_factor % 2 == 0);
    if (div2_en) {
      ref_div_factor /= 2;
    }

    _regs.reference_divide_by_2 = div2_en ?
                                  adf535x_regs_t::REFERENCE_DIVIDE_BY_2_ENABLED :
                                  adf535x_regs_t::REFERENCE_DIVIDE_BY_2_DISABLED;
    _regs.reference_doubler = doubler_en ?
                              adf535x_regs_t::REFERENCE_DOUBLER_ENABLED :
                              adf535x_regs_t::REFERENCE_DOUBLER_DISABLED;
    _regs.r_counter_10_bit = ref_div_factor;
    UHD_ASSERT_THROW((_regs.r_counter_10_bit & ((uint16_t)~0x3FF)) == 0);

    //-----------------------------------------------------------
    //Set timeouts (code from ADI driver)
    _regs.timeout = std::max(1, std::min(int(ceil(_pfd_freq / (20e3 * 30))), 1023));

    UHD_ASSERT_THROW((_regs.timeout & ((uint16_t)~0x3FF)) == 0);
    _regs.synth_lock_timeout =
            static_cast<uint8_t>(ceil((_pfd_freq * 2) / (100e3 * _regs.timeout)));
    UHD_ASSERT_THROW((_regs.synth_lock_timeout & ((uint16_t)~0x1F)) == 0);
    _regs.auto_level_timeout =
            static_cast<uint8_t>(ceil((_pfd_freq * 5) / (100e3 * _regs.timeout)));

    //-----------------------------------------------------------
    //Set VCO band divider
    _regs.vco_band_div =
            static_cast<uint8_t>(ceil(_pfd_freq / 2.4e6));

    //-----------------------------------------------------------
    //Set ADC delay (code from ADI driver)
    _regs.adc_enable = adf535x_regs_t::ADC_ENABLE_ENABLED;
    _regs.adc_conversion = adf535x_regs_t::ADC_CONVERSION_ENABLED;
    _regs.adc_clock_divider = std::max(1, std::min(int(ceil(((_pfd_freq / 100e3) - 2) / 4)),255));

    _wait_time_us = static_cast<uint32_t>(
            ceil(16e6 / (_pfd_freq / ((4 * _regs.adc_clock_divider) + 2))));

    //-----------------------------------------------------------
    //Phase resync
    // TODO Renable here, in initialization, or through separate set_phase_resync(bool enable) function
    _regs.phase_resync = adf535x_regs_t::PHASE_RESYNC_DISABLED;

    _regs.phase_adjust = adf535x_regs_t::PHASE_ADJUST_DISABLED;
    _regs.sd_load_reset = adf535x_regs_t::SD_LOAD_RESET_ON_REG0_UPDATE;
    _regs.phase_resync_clk_div = static_cast<uint16_t>(
            floor(ADF535X_PHASE_RESYNC_TIME * _pfd_freq));

    _rewrite_regs = true;
  }

  double set_frequency(const double target_freq, const double freq_resolution, const bool flush = false)
  {
    return _set_frequency(target_freq, freq_resolution, flush);
  }

  void set_output_power(const output_power_t power)
  {
    typename adf535x_regs_t::output_power_t setting;
    switch (power) {
      case OUTPUT_POWER_M4DBM: setting = adf535x_regs_t::OUTPUT_POWER_M4DBM; break;
      case OUTPUT_POWER_M1DBM: setting = adf535x_regs_t::OUTPUT_POWER_M1DBM; break;
      case OUTPUT_POWER_2DBM:  setting = adf535x_regs_t::OUTPUT_POWER_2DBM; break;
      case OUTPUT_POWER_5DBM:  setting = adf535x_regs_t::OUTPUT_POWER_5DBM; break;
      default: UHD_THROW_INVALID_CODE_PATH();
    }
    if (_regs.output_power != setting) _rewrite_regs = true;
    _regs.output_power = setting;
  }

  void set_output_enable(const output_t output, const bool enable)
  {
    switch (output) {
      case RF_OUTPUT_A: _regs.rf_out_a_enabled = enable ? adf535x_regs_t::RF_OUT_A_ENABLED_ENABLED :
                                                 adf535x_regs_t::RF_OUT_A_ENABLED_DISABLED;
        break;
      case RF_OUTPUT_B: _regs.rf_out_b_enabled = enable ? adf535x_regs_t::RF_OUT_B_ENABLED_ENABLED :
                                                 adf535x_regs_t::RF_OUT_B_ENABLED_DISABLED;
        break;
    }
  }

  void set_muxout_mode(const muxout_t mode)
  {
    switch (mode) {
      case MUXOUT_3STATE: _regs.muxout = adf535x_regs_t::MUXOUT_3STATE; break;
      case MUXOUT_DVDD:   _regs.muxout = adf535x_regs_t::MUXOUT_DVDD; break;
      case MUXOUT_DGND:   _regs.muxout = adf535x_regs_t::MUXOUT_DGND; break;
      case MUXOUT_RDIV:   _regs.muxout = adf535x_regs_t::MUXOUT_RDIV; break;
      case MUXOUT_NDIV:   _regs.muxout = adf535x_regs_t::MUXOUT_NDIV; break;
      case MUXOUT_ALD:    _regs.muxout = adf535x_regs_t::MUXOUT_ANALOG_LD; break;
      case MUXOUT_DLD:    _regs.muxout = adf535x_regs_t::MUXOUT_DLD; break;
      default: UHD_THROW_INVALID_CODE_PATH();
    }
  }

  void commit()
  {
    _commit();
  }

protected:
  double _set_frequency(double, double, bool);
  void _commit();

private: //Members
  typedef std::vector<uint32_t> addr_vtr_t;

  write_fn_t      _write_fn;
  adf535x_regs_t  _regs;
  bool            _rewrite_regs;
  uint32_t        _wait_time_us;
  double          _ref_freq;
  double          _pfd_freq;
  double          _fb_after_divider;
};

// ADF5355 Functions
template <>
inline double adf535x_impl<adf5355_regs_t>::_set_frequency(double target_freq, double freq_resolution, bool flush)
{
  if (target_freq > ADF535X_MAX_OUT_FREQ or target_freq < ADF535X_MIN_OUT_FREQ) {
    throw uhd::runtime_error("requested frequency out of range.");
  }
  if ((uint32_t) freq_resolution == 0) {
    throw uhd::runtime_error("requested resolution cannot be less than 1.");
  }

  /* Calculate target VCOout frequency */
  //Increase RF divider until acceptable VCO frequency
  double target_vco_freq = target_freq;
  uint32_t rf_divider = 1;
  while (target_vco_freq < ADF535X_MIN_VCO_FREQ && rf_divider < 64) {
    target_vco_freq *= 2;
    rf_divider *= 2;
  }

  switch (rf_divider) {
    case 1:  _regs.rf_divider_select = adf5355_regs_t::RF_DIVIDER_SELECT_DIV1;  break;
    case 2:  _regs.rf_divider_select = adf5355_regs_t::RF_DIVIDER_SELECT_DIV2;  break;
    case 4:  _regs.rf_divider_select = adf5355_regs_t::RF_DIVIDER_SELECT_DIV4;  break;
    case 8:  _regs.rf_divider_select = adf5355_regs_t::RF_DIVIDER_SELECT_DIV8;  break;
    case 16: _regs.rf_divider_select = adf5355_regs_t::RF_DIVIDER_SELECT_DIV16; break;
    case 32: _regs.rf_divider_select = adf5355_regs_t::RF_DIVIDER_SELECT_DIV32; break;
    case 64: _regs.rf_divider_select = adf5355_regs_t::RF_DIVIDER_SELECT_DIV64; break;
    default: UHD_THROW_INVALID_CODE_PATH();
  }

  //Compute fractional PLL params
  double prescaler_input_freq = target_vco_freq;
  if (_fb_after_divider) {
    prescaler_input_freq /= rf_divider;
  }

  const double N = prescaler_input_freq / _pfd_freq;
  const uint16_t INT = static_cast<uint16_t>(floor(N));
  const uint32_t FRAC1 = static_cast<uint32_t>(floor((N - INT) * ADF535X_MOD1));
  const double residue = (N - INT) * ADF535X_MOD1 - FRAC1;

  const double gcd = boost::math::gcd(static_cast<int>(_pfd_freq), static_cast<int>(freq_resolution));
  const uint16_t MOD2 = static_cast<uint16_t>(std::min(floor(_pfd_freq / gcd), static_cast<double>(ADF535X_MAX_MOD2)));
  const uint16_t FRAC2 = static_cast<uint16_t>(std::min(ceil(residue * MOD2), static_cast<double>(ADF535X_MAX_FRAC2)));

  const double coerced_vco_freq = _pfd_freq * (
    double(INT) + (
      (double(FRAC1) +
      (double(FRAC2) / double(MOD2)))
      / double(ADF535X_MOD1)
    )
  );

  const double coerced_out_freq = coerced_vco_freq / rf_divider;

  /* Update registers */
  _regs.int_16_bit = INT;
  _regs.frac1_24_bit = FRAC1;
  _regs.frac2_14_bit = FRAC2;
  _regs.mod2_14_bit = MOD2;
  _regs.phase_24_bit = 0;

  if (flush) commit();
  return coerced_out_freq;
}

template <>
inline void adf535x_impl<adf5355_regs_t>::_commit()
{
  const size_t ONE_REG = 1;

  if (_rewrite_regs) {
    //For a full state sync write registers in reverse order 12 - 0
    addr_vtr_t regs;
    for (uint8_t addr = 12; addr > 0; addr--) {
      regs.push_back(_regs.get_reg(addr));
    }
    _write_fn(regs);
    // TODO Add FPGA based delay between these writes
    _write_fn(addr_vtr_t(ONE_REG, _regs.get_reg(0)));
    _rewrite_regs = false;

  } else {
    //Frequency update sequence from data sheet
    _write_fn(addr_vtr_t(ONE_REG, _regs.get_reg(6)));
    _regs.counter_reset = adf5355_regs_t::COUNTER_RESET_ENABLED;
    _write_fn(addr_vtr_t(ONE_REG, _regs.get_reg(4)));
    _write_fn(addr_vtr_t(ONE_REG, _regs.get_reg(2)));
    _write_fn(addr_vtr_t(ONE_REG, _regs.get_reg(1)));
    _regs.autocal_en = adf5355_regs_t::AUTOCAL_EN_DISABLED;
    _write_fn(addr_vtr_t(ONE_REG, _regs.get_reg(0)));
    _regs.counter_reset = adf5355_regs_t::COUNTER_RESET_DISABLED;
    _write_fn(addr_vtr_t(ONE_REG, _regs.get_reg(4)));
    // TODO Add FPGA based delay between these writes
    _regs.autocal_en = adf5355_regs_t::AUTOCAL_EN_ENABLED;
    _write_fn(addr_vtr_t(ONE_REG, _regs.get_reg(0)));
  }
}

// ADF5356 Functions
template <>
inline double adf535x_impl<adf5356_regs_t>::_set_frequency(double target_freq, double freq_resolution, bool flush)
{
  if (target_freq > ADF535X_MAX_OUT_FREQ or target_freq < ADF535X_MIN_OUT_FREQ) {
    throw uhd::runtime_error("requested frequency out of range.");
  }
  if ((uint32_t) freq_resolution == 0) {
    throw uhd::runtime_error("requested resolution cannot be less than 1.");
  }

  /* Calculate target VCOout frequency */
  //Increase RF divider until acceptable VCO frequency
  double target_vco_freq = target_freq;
  uint32_t rf_divider = 1;
  while (target_vco_freq < ADF535X_MIN_VCO_FREQ && rf_divider < 64) {
    target_vco_freq *= 2;
    rf_divider *= 2;
  }

  switch (rf_divider) {
    case 1:  _regs.rf_divider_select = adf5356_regs_t::RF_DIVIDER_SELECT_DIV1;  break;
    case 2:  _regs.rf_divider_select = adf5356_regs_t::RF_DIVIDER_SELECT_DIV2;  break;
    case 4:  _regs.rf_divider_select = adf5356_regs_t::RF_DIVIDER_SELECT_DIV4;  break;
    case 8:  _regs.rf_divider_select = adf5356_regs_t::RF_DIVIDER_SELECT_DIV8;  break;
    case 16: _regs.rf_divider_select = adf5356_regs_t::RF_DIVIDER_SELECT_DIV16; break;
    case 32: _regs.rf_divider_select = adf5356_regs_t::RF_DIVIDER_SELECT_DIV32; break;
    case 64: _regs.rf_divider_select = adf5356_regs_t::RF_DIVIDER_SELECT_DIV64; break;
    default: UHD_THROW_INVALID_CODE_PATH();
  }

  //Compute fractional PLL params
  double prescaler_input_freq = target_vco_freq;
  if (_fb_after_divider) {
    prescaler_input_freq /= rf_divider;
  }

  const double N = prescaler_input_freq / _pfd_freq;
  const uint16_t INT = static_cast<uint16_t>(floor(N));
  const uint32_t FRAC1 = static_cast<uint32_t>(floor((N - INT) * ADF535X_MOD1));
  const double residue = (N - INT) * ADF535X_MOD1 - FRAC1;

  const double gcd = boost::math::gcd(static_cast<int>(_pfd_freq), static_cast<int>(freq_resolution));
  const uint16_t MOD2 = static_cast<uint16_t>(std::min(floor(_pfd_freq / gcd), static_cast<double>(ADF535X_MAX_MOD2)));
  const uint16_t FRAC2 = static_cast<uint16_t>(std::min(ceil(residue * MOD2), static_cast<double>(ADF535X_MAX_FRAC2)));

  const double coerced_vco_freq = _pfd_freq * (
    double(INT) + (
      (double(FRAC1) +
      (double(FRAC2) / double(MOD2)))
      / double(ADF535X_MOD1)
    )
  );

  const double coerced_out_freq = coerced_vco_freq / rf_divider;

  /* Update registers */
  _regs.int_16_bit = INT;
  _regs.frac1_24_bit = FRAC1;
  _regs.frac2_msb = FRAC2;
  _regs.mod2_msb = MOD2;
  _regs.phase_24_bit = 0;

  if (flush) commit();
  return coerced_out_freq;
}

template <>
inline void adf535x_impl<adf5356_regs_t>::_commit()
{
  const size_t ONE_REG = 1;
  if (_rewrite_regs) {
    //For a full state sync write registers in reverse order 12 - 0
    addr_vtr_t regs;
    for (uint8_t addr = 13; addr > 0; addr--) {
      regs.push_back(_regs.get_reg(addr));
    }
    _write_fn(regs);
    // TODO Add FPGA based delay between these writes
    _write_fn(addr_vtr_t(ONE_REG, _regs.get_reg(0)));
    _rewrite_regs = false;

  } else {
    //Frequency update sequence from data sheet
    _write_fn(addr_vtr_t(ONE_REG, _regs.get_reg(13)));
    _write_fn(addr_vtr_t(ONE_REG, _regs.get_reg(10)));
    _write_fn(addr_vtr_t(ONE_REG, _regs.get_reg(6)));
    _write_fn(addr_vtr_t(ONE_REG, _regs.get_reg(2)));
    _write_fn(addr_vtr_t(ONE_REG, _regs.get_reg(1)));
    // TODO Add FPGA based delay between these writes
    _write_fn(addr_vtr_t(ONE_REG, _regs.get_reg(0)));
  }
}

#endif // INCLUDED_ADF535X_HPP
