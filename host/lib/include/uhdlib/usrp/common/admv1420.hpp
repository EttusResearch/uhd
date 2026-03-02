//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include "admv1420_regs.hpp"
#include <functional>
#include <memory>

//! Control interface for an ADMV1420
class admv1420_iface
{
public:
    using sptr = std::unique_ptr<admv1420_iface>;

    virtual ~admv1420_iface() = default;

    enum class lo_sideband_t { LSB, USB };
    enum class bb_amp_t { I, Q };
    enum class mix_gate_bias_adj_mode_t { AUTO, MANUAL };
    enum class pd_comp_t { DSA1, RF_LNA_B0, RF_LNA_B1, RF_LNA_B2, BB_AMP_I, BB_AMP_Q };
    enum class bb_switch_t { BASEBAND, IF };
    enum class filter_sel_t { LUT_REG, SPI };
    enum class sdo_level_t { V3_3, V1_8 };
    enum class dsa_t { DSA1 = 1, DSA2 = 2, DSA3 = 3, DSA4 = 4, DSA5 = 5 };
    enum class rf_band_t { BAND_0, BAND_1, BAND_2, BAND_3 };
    enum class if_band_t { BAND_0, BAND_1 };
    enum class internal_voltage_t { V1_25 };

    //! Write functor: Take address / data pair, craft SPI transaction
    using write_fn_t = std::function<void(uint32_t, uint16_t)>;

    //! Read functor: Return value given address
    using read_fn_t = std::function<uint16_t(uint32_t)>;

    //! Factory
    //
    // \param write SPI write function object
    // \param read SPI read function object
    static sptr make(write_fn_t&& poke16, read_fn_t&& peek16);

    //! Save state to chip
    virtual void commit() = 0;

    //! Set the VCM voltage
    virtual void set_vcm(internal_voltage_t voltage) = 0;

    //! Finalize the ADMV init
    //
    // This method does not require a commit() afterwards as it pokes the registers
    // directly.
    virtual void finalize_init() = 0;

    //! Sets the RF band
    //
    // Bands are frequency dependent but since they are overlapping they need to be chosen
    // per application:
    // Band 0: 0.1 GHz to 2 GHz
    // Band 1: 1 GHz to 5 GHz
    // Band 2: 2 GHz to 13 GHz
    // Band 3: 6 GHz to 20 GHz
    virtual void set_rf_band(const rf_band_t band) = 0;

    //! Sets the IF band
    //
    // IF Band 0: 1 GHz to 5 GHz
    // IF Band 1: 3 GHz to 13 GHz
    virtual void set_if_band(const if_band_t if_band) = 0;

    //! Sets the LO X3 filter
    //
    // It does this depending on the frequency
    virtual void set_lo_x3_filter(const double freq) = 0;

    //! Set LO Sideband
    //
    // Enables swapping I and Q.
    virtual void set_lo_sideband(const lo_sideband_t sideband) = 0;

    //! Enable/disable filter table
    virtual void enable_filter_table(const bool enable) = 0;

    //! Enable/disable gain table
    virtual void enable_gain_table(const bool enable) = 0;

    //! Set DSA value
    //
    // \param dsa: The DSA to set
    // \param value: The attenuation value to set (0-15 dB, DSA2 only 0 or 6 dB)
    // \return The value set for the DSA
    virtual uint8_t set_dsa(const dsa_t dsa, const uint8_t value) = 0;

    //! Get DSA value
    virtual uint8_t get_dsa(const dsa_t dsa) = 0;

    //! Sets the baseband switch control
    virtual void set_if_bb_switch_ctrl(const bb_switch_t if_bb_switch_ctrl) = 0;

    //! Enables/disables powerdown for given components.
    //
    // \param components: List of components to be enabled/disabled
    // \param enable: If enabled, the LNA band is powered down
    virtual void set_powerdown(
        const std::vector<pd_comp_t> components, const bool enable) = 0;

    //! Sets the baseband amplifier output common mode voltage select
    //
    // \param internal: false if external, true if internal
    virtual void set_bb_amp_output_common_mode_int(const bool internal) = 0;

    //! Sets the mix gate bias adjustment mode
    virtual void set_mix_gate_bias_adj_mode(const mix_gate_bias_adj_mode_t mode) = 0;

    //! Sets the absolute mixer gate bias voltage
    //
    // Only applies if mixer gate bias adjustment mode is set to manual.
    // Voltage is set in mV and hardware accepts steps of 50 mV.
    virtual double set_mix_gate_bias_voltage(const double voltage) = 0;

    //! Set SDO level (SPI config)
    //
    // \param setting: V3_3 for 3.3 V, V1_8 for 1.8 V
    virtual void set_sdo_level(const sdo_level_t setting) = 0;

    //! Selects the Low Pass Filter state source
    //
    // \param select: LUT_REG for lookup table or registers, SPI for state by SPI
    virtual void select_lpf_source(const filter_sel_t select) = 0;

    //! Set the LPF cutoff frequency
    //
    // The cutoff freq is inversively propertional to the value passed.
    virtual void set_lpf_cutoff_freq(const uint8_t value) = 0;

    //! Selects the High Pass Filter state source
    //
    // \param select: LUT_REG for lookup table or registers, SPI for state by SPI
    virtual void select_hpf_source(const filter_sel_t select) = 0;

    //! Set the HPF cutoff frequency
    //
    // The cutoff freq is inversively propertional to the value passed.
    virtual void set_hpf_cutoff_freq(const uint8_t value) = 0;
};
