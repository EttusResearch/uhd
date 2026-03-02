//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhdlib/usrp/common/admv1320.hpp>
#include <uhdlib/usrp/dboard/hbx/hbx_cpld_ctrl.hpp>
#include <functional>

namespace uhd { namespace usrp { namespace hbx {

class hbx_admv1320_ctrl final : public hbx_cpld_ctrl::spi_transactor
{
public:
    using init_callback_t = std::function<void(const bool)>;

    hbx_admv1320_ctrl(size_t start_address,
        hbx_cpld_ctrl::poke_fn_type&& poke_fn,
        hbx_cpld_ctrl::peek_fn_type&& peek_fn,
        hbx_cpld_ctrl::mixer_callback_t&& init_cb,
        const std::string& log_id);

    // Set the RF band to be used.
    void set_rf_band(const uint8_t band);

    // Set the input mode (bypass, IF, baseband)
    void set_input_mode(const admv1320_iface::input_mode_t mode);

    // Set the attenuation value
    uint8_t set_dsa(const admv1320_iface::dsa_t dsa, const uint8_t value);

    // Get the attenuation value
    uint8_t get_dsa(const admv1320_iface::dsa_t dsa);

    // Set the compound DSA value
    //
    // \param value The compound DSA value to set
    // \return The values set for DSA1 and DSA2
    uint8_t set_compound_dsa(const uint8_t value);

    // Get the compound DSA value
    uint8_t get_compound_dsa();

    // Set the lo_x3 filter according to the given frequency
    void set_lo_x3_filter(const double freq);

    // Sets the LO sideband to be used (may do an IQ swap).
    void set_lo_sideband(const admv1320_iface::lo_sideband_t sideband);

    // Power down the chip if enable==true
    void set_chip_powerdown(const bool enable);

    // Power down the RF Highspeed if enable==true
    void set_rf_hs_powerdown(const bool enable);

    // Selects the Low Pass Filter state source
    void set_lpf_source(const admv1320_iface::filter_sel_t select);

    // Sets the LPF cutoff frequency
    void set_lpf_cutoff_freq(const uint8_t value);

    // Selects the Low Pass Filter state source
    void set_hpf_source(const admv1320_iface::filter_sel_t select);

    // Sets the LPF cutoff frequency
    void set_hpf_cutoff_freq(const uint8_t value);

    // Configures ADMV1320 for HBX
    void configure(const hbx_tune_map_item_t config, const double freq);


private:
    // ADMV1320 driver instance
    admv1320_iface::sptr _admv;

    // ADMV init callback
    hbx_cpld_ctrl::mixer_callback_t _init_cb;

    // String prefix for log messages
    const std::string _log_id;

    // Compound DSA value
    uint16_t _comp_dsa;
};

}}} // namespace uhd::usrp::hbx
