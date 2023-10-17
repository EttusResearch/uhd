//
// Copyright 2020 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/types/time_spec.hpp>
#include <functional>
#include <memory>

//! Control interface for an LMX2572 synthesizer
class lmx2572_iface
{
public:
    using sptr = std::shared_ptr<lmx2572_iface>;

    virtual ~lmx2572_iface() = default;

    enum output_t { RF_OUTPUT_A, RF_OUTPUT_B };

    enum mux_in_t { DIVIDER, VCO, HIGH_IMPEDANCE, SYSREF };

    //! Category of phase sync procedure. See Section 8.1.6 ("Application for
    // SYNC") in the datasheet. Category NONE applies when no phase
    // synchronization is required.
    enum sync_cat { CAT1A, CAT1B, CAT2, CAT3, CAT4, NONE };

    //! Write functor: Take address / data pair, craft SPI transaction
    using write_fn_t = std::function<void(uint8_t, uint16_t)>;

    //! Read functor: Return value given address
    using read_fn_t = std::function<uint16_t(uint8_t)>;

    //! Sleep functor: sleep for the specified time
    using sleep_fn_t = std::function<void(const uhd::time_spec_t&)>;

    //! Factory
    //
    // \param write SPI write function object
    // \param read SPI read function object
    // \param sleep sleep function object
    static sptr make(write_fn_t&& poke16, read_fn_t&& peek16, sleep_fn_t&& sleep);

    //! Save state to chip
    virtual void commit() = 0;

    //! Get enabled status
    virtual bool get_enabled() = 0;

    //! Enable/disable
    virtual void set_enabled(const bool enabled = true) = 0;

    //! Performs a reset of the LMX2572 by using the software reset register
    virtual void reset() = 0;

    //! Returns True if the PLL is locked, False otherwise.
    virtual bool get_lock_status() = 0;

    //! Enables or disables the phase synchronization
    //
    // NOTE: This does not write anything to the device, it just sets the
    // VCO_PHASE_SYNC_EN high.
    virtual void set_sync_mode(const bool enable) = 0;

    //! Returns the enabled/disabled state of the phase synchronization
    virtual bool get_sync_mode() = 0;

    //!  Enables or disables the output on both ports
    virtual void set_output_enable_all(const bool enable) = 0;

    //! Sets output A or B (OUTA_PD or OUTB_PD)
    virtual void set_output_enable(const output_t output, const bool enable) = 0;

    //! Sets the output power
    //
    // \param output Choose which output to control
    // \param power Power control bits. Higher values mean more power, but the
    //              function that maps power control bits to power is non-linear,
    //              and it is also frequency-dependent. For more detail, check
    //              the data sheet, section 8.1.5.1. Ballpark numbers: 0 dBm is
    //              at about power==27, over 35 the increase becomes "not obvious".
    virtual void set_output_power(const output_t output, const uint8_t power) = 0;

    //! Sets the OUTA_MUX or OUTB_MUX input
    virtual void set_mux_input(const output_t output, const mux_in_t input) = 0;

    //! Set the output frequency
    //
    // A note on phase synchronization: If set_sync_mode(true) was called
    // previously, then this method will set up the PLL in a phase-sync mode.
    // However, this specific implementation assumes that the SYNC pin is
    // populated, and will be pulsed after calling this command.
    //
    // \param target_freq The target frequency
    // \param ref_freq The input reference frequency
    // \param spur_dodging Set to true to enable spur dodging
    virtual double set_frequency(
        const double target_freq, const double ref_freq, const bool spur_dodging) = 0;
};
