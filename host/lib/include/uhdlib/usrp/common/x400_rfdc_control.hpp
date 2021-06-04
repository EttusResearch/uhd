//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/types/memmap_iface.hpp>
#include <uhd/types/time_spec.hpp>
#include <cstdint>
#include <memory>
#include <vector>
#include <string>

namespace uhd { namespace rfnoc { namespace x400 {

//! Control class for the RFDC components of a single daughterboard
//
// This class controls the NCOs and other RFDC settings. The corresponding FPGA
// module is rfdc_timing_control.v.
class rfdc_control
{
public:
    using sptr = std::shared_ptr<rfdc_control>;

    struct regmap
    {
        //! Address of the NCO reset register
        static constexpr uint32_t NCO_RESET = 0;
        //! Bit position of reset-start bit (w)
        static constexpr uint32_t NCO_RESET_START_MSB = 0;
        //! Bit position of reset-done bit (r)
        static constexpr uint32_t NCO_RESET_DONE_MSB = 1;
        //! Address of the gearbox reset register
        static constexpr uint32_t GEARBOX_RESET = 4;
        //! Bit position of ADC gearbox reset
        static constexpr uint32_t ADC_RESET_MSB = 0;
        //! Bit position of DAC gearbox reset
        static constexpr uint32_t DAC_RESET_MSB = 1;
    };

    //! Identify the NCOs/ADCs/DACs available to this radio control
    enum class rfdc_type { RX0, RX1, TX0, TX1 };

    rfdc_control(uhd::memmap32_iface_timed&& iface, const std::string& log_id);

    //! Reset the listed NCOs
    //
    // All NCOs that are listed in \p ncos are reset synchronously.
    //
    // \param ncos A list of NCOs that shall be reset at the given time
    // \param time The time at which the reset shall occur
    void reset_ncos(const std::vector<rfdc_type>& ncos, const uhd::time_spec_t& time);

    //! Reset the listed gearboxes
    //
    // All gearboxes that are listed in \p gearboxes are reset synchronously.
    //
    // \param gearboxes A list of gearboxes that shall be reset at the given time
    // \param time The time at which the reset shall occur. Note: If \p time is
    //             set to ASAP, the resets will still occur synchronously, but
    //             at a non-deterministic time. This will suffice for synchronizing
    //             gearboxes on a single device.
    void reset_gearboxes(
        const std::vector<rfdc_type>& gearboxes, const uhd::time_spec_t& time);

    //! Return true if the NCO is out of reset
    bool get_nco_reset_done();

    //! Set an NCO to a specific frequency
    //
    // \param nco Which NCO to re-tune
    // \param freq the new frequency to tune it to (in Hz)
    double set_nco_freq(const rfdc_type nco, const double freq);

private:
    //! Peek/poke interface
    memmap32_iface_timed _iface;

    //! Prefix for log messages
    const std::string _log_id;
};

}}} // namespace uhd::rfnoc::x400
