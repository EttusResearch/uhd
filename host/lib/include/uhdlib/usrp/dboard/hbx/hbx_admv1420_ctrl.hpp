//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhdlib/usrp/common/admv1420.hpp>
#include <uhdlib/usrp/dboard/hbx/hbx_cpld_ctrl.hpp>
#include <functional>

namespace uhd { namespace usrp { namespace hbx {

class hbx_admv1420_ctrl final : public hbx_cpld_ctrl::spi_transactor
{
public:
    using init_callback_t = std::function<void(const bool)>;

    hbx_admv1420_ctrl(size_t start_address,
        hbx_cpld_ctrl::poke_fn_type&& poke_fn,
        hbx_cpld_ctrl::peek_fn_type&& peek_fn,
        hbx_cpld_ctrl::mixer_callback_t&& init_cb,
        const std::string& log_id);

    //! The bands according to the HBX frequency plan
    enum class hbx_band_t { BAND_0, BAND_1, BAND_2 };

    //! Set the RF band to be used.
    void set_rf_band(const admv1420_iface::rf_band_t band);

    //! Set the attentuation value for DSAs 1-5 (in dB)
    //
    // \param dsa The DSA to set
    // \param value The attenuation value to set (0-15 dB, DSA2 only 0 or 6 dB)
    // \return The value set for the DSA
    uint8_t set_dsa(const admv1420_iface::dsa_t dsa, uint8_t value);

    //! Get the attenuation value for DSAs 1-5 (in dB)
    //
    // \param dsa The DSA to get
    // \return The value set for the DSA
    uint8_t get_dsa(const admv1420_iface::dsa_t dsa);

    //! Set the lo_x3 filter according to the given frequency
    void set_lo_x3_filter(const double freq);

    //! Sets the LO sideband to be used (may do an IQ swap).
    void set_lo_sideband(const admv1420_iface::lo_sideband_t sideband);

    // Sets everything according to the HBX RX band definitions
    void configure(const hbx_band_t hbx_band,
        const admv1420_iface::rf_band_t admv_band,
        const double freq);

    //! Sets a compound DSA value
    //
    // \param value The compound DSA value to set
    // \return The values set for all DSAs 1 to 5
    uint8_t set_compound_dsa(const uint8_t value);

    //! Get the compound DSA value for the current band
    uint8_t get_compound_dsa();

    //! Get the maximum compound DSA value for the current band
    uint8_t get_max_compound_dsa();

private:
    // ADMV1420 driver instance
    admv1420_iface::sptr _admv;

    // ADMV init callback
    hbx_cpld_ctrl::mixer_callback_t _init_cb;

    // String prefix for log messages
    const std::string _log_id;

    // Variables for current settings
    // Current band
    hbx_band_t _band = hbx_band_t::BAND_1;

    // Compound DSA value
    uint16_t _comp_dsa;
};

}}} // namespace uhd::usrp::hbx
