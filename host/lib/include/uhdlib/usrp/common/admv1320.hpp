//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include "admv1320_regs.hpp"
#include <functional>
#include <memory>

//! Control interface for ADMV1320.
// Currently, mainly the HBX-specific requirements are covered here, so extensions may be
// required for other products.
// To apply the changes made by any of these methods, call `commit()` afterwards.
class admv1320_iface
{
public:
    using sptr = std::unique_ptr<admv1320_iface>;

    virtual ~admv1320_iface() = default;

    enum class input_mode_t { BYPASS, IF, BASEBAND };
    enum class lo_sideband_t { LSB, USB };
    enum class filter_sel_t { LUT_REG, SPI };
    enum class rf_band_t { BAND_0, BAND_1, BAND_2, BAND_3 };
    enum class dsa_t { DSA1 = 1, DSA2 = 2 };

    //! Write functor: Take address / data pair, craft SPI transaction
    using write_fn_t = std::function<void(uint32_t, uint16_t)>;

    //! Read functor: Return value given address
    using read_fn_t = std::function<uint16_t(uint32_t)>;

    //! Factory
    //
    // \param write SPI write function object
    // \param read SPI read function object
    // \param unique_id Unique ID string for logging purposes
    static sptr make(
        write_fn_t&& poke16, read_fn_t&& peek16, const std::string& unique_id);

    //! Save state to chip
    virtual void commit() = 0;

    //! Sets the RF band
    //
    // Bands are frequency dependent but since they are overlapping they need to be chosen
    // per application:
    // Band 0: 0 to 2 GHz
    // Band 1: 1 GHz to 5 GHz
    // Band 2: 3 GHz to 10 GHz
    // Band 3: 6 GHz to 20 GHz
    //
    // Not all bands are compatible with all settings in `set_input_mode()`.
    virtual void set_rf_band(const rf_band_t band) = 0;

    //! Enables or disables the gain table bypass mode
    virtual void enable_gain_table_bypass(const bool enable) = 0;

    //! Set DSA settings
    virtual uint8_t set_dsa(const dsa_t dsa, const uint8_t value) = 0;

    //! Get the DSA settings
    virtual uint8_t get_dsa(const dsa_t dsa) = 0;

    //! Sets the input mode
    //
    // Choices are: BYPASS, IF or BASEBAND.
    // BYPASS: Enables IF and all 4 bands, using the mixer bypass.
    // IF: Enables IF + mixer + band 3 (functionality not verified).
    // BASEBAND: Enables baseband operation + band 3.
    //
    // In the IF case the chip allows to configure the IF high or low band. This
    // functionality is currently not implemented here.
    virtual void set_input_mode(const input_mode_t mode) = 0;

    //! RF High Speed Power Down
    //
    // \param enable: If enabled, the RF High Speed is powered down
    virtual void set_rf_hs_powerdown(const bool enable) = 0;

    //! Chip Power Down
    //
    // \param enable: If enabled, the Chip is powered down
    virtual void set_chip_powerdown(const bool enable) = 0;

    //! Set LO Sideband
    //
    // Configures the LO sideband to be uesd. That may cause an I/Q swap.
    virtual void set_lo_sideband(const lo_sideband_t sideband) = 0;

    //! Set LO X3 Filter
    //
    // Chooses the LO X3 filter depending on the frequency between 8 and 28 GHz
    virtual void set_lo_x3_filter(const double freq) = 0;

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
