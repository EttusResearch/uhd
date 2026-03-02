//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/usrp/dboard/hbx/hbx_admv1320_ctrl.hpp>

namespace uhd { namespace usrp { namespace hbx {

hbx_admv1320_ctrl::hbx_admv1320_ctrl(size_t start_address,
    hbx_cpld_ctrl::poke_fn_type&& poke_fn,
    hbx_cpld_ctrl::peek_fn_type&& peek_fn,
    hbx_cpld_ctrl::mixer_callback_t&& init_cb,
    const std::string& log_id)
    : hbx_cpld_ctrl::spi_transactor(
        start_address, std::move(poke_fn), std::move(peek_fn), true)
    , _init_cb(std::move(init_cb))
    , _log_id(log_id)
{
    // Prepare in the callback to the CPLD regs
    _init_cb(hbx_cpld_ctrl::INIT);

    // Create the ADMV object. This will set SDO active in its ctor
    _admv = admv1320_iface::make(
        [this](uint32_t addr, uint16_t data) { this->spi_write(addr, data); },
        [this](uint32_t addr) -> uint16_t { return this->spi_read(addr); });

    // Callback to CPLD for finalization.
    _init_cb(hbx_cpld_ctrl::FINALIZE);

    // General settings for all bands that we are using

    // We're not using gain lookup tables, so we bypass them.
    _admv->enable_gain_table_bypass(true);

    set_lpf_source(admv1320_iface::filter_sel_t::SPI);
    // Highest cut-off
    set_lpf_cutoff_freq(0);
    set_hpf_source(admv1320_iface::filter_sel_t::SPI);
    // Lowest cut-off
    set_hpf_cutoff_freq(63);
};

void hbx_admv1320_ctrl::set_rf_band(const uint8_t band)
{
    UHD_LOG_TRACE(_log_id, "Setting RF band to " << static_cast<uint16_t>(band));
    // Only do something if we are in band 2 or 3 of ADMV, otherwise nothing.
    if (band > 1) {
        _admv->set_rf_band(static_cast<admv1320_iface::rf_band_t>(band));
        _admv->commit();
    }
};

void hbx_admv1320_ctrl::set_input_mode(const admv1320_iface::input_mode_t mode)
{
    UHD_LOG_TRACE(_log_id, "Setting input mode to " << static_cast<int>(mode));
    _admv->set_input_mode(mode);
    _admv->commit();
};

uint8_t hbx_admv1320_ctrl::set_dsa(const admv1320_iface::dsa_t dsa, const uint8_t value)
{
    UHD_LOG_TRACE(_log_id,
        "Setting DSA " << static_cast<uint16_t>(dsa) << " to "
                       << static_cast<uint16_t>(value));
    auto ret_val = _admv->set_dsa(dsa, value);
    _admv->commit();
    return ret_val;
};

uint8_t hbx_admv1320_ctrl::get_dsa(const admv1320_iface::dsa_t dsa)
{
    auto ret_val = _admv->get_dsa(dsa);
    UHD_LOG_TRACE(_log_id,
        "Getting DSA " << static_cast<uint16_t>(dsa) << " value "
                       << static_cast<uint16_t>(ret_val));
    return ret_val;
};

uint8_t hbx_admv1320_ctrl::set_compound_dsa(const uint8_t value)
{
    UHD_ASSERT_THROW(value <= 2 * ADMV_DSA_MAX_ATTENUATION);
    // Ensure to round up one of the values in case the original value was odd:
    const uint8_t dsa2 = value / 2;
    const uint8_t dsa1 = value - dsa2;
    _comp_dsa          = dsa1 + dsa2;
    UHD_LOG_TRACE(_log_id,
        "Compound DSA " << _comp_dsa << " distributed to DSA1 "
                        << static_cast<uint16_t>(dsa1) << ", DSA2 "
                        << static_cast<uint16_t>(dsa2) << ".");
    _admv->set_dsa(admv1320_iface::dsa_t::DSA1, dsa1);
    _admv->set_dsa(admv1320_iface::dsa_t::DSA2, dsa2);
    _admv->commit();
    return _comp_dsa;
};

uint8_t hbx_admv1320_ctrl::get_compound_dsa()
{
    _comp_dsa = _admv->get_dsa(admv1320_iface::dsa_t::DSA1)
                + _admv->get_dsa(admv1320_iface::dsa_t::DSA2);
    return _comp_dsa;
};

void hbx_admv1320_ctrl::set_lo_x3_filter(const double freq)
{
    UHD_LOG_TRACE(_log_id, "Setting LO X3 filter to " << freq);
    _admv->set_lo_x3_filter(freq);
    _admv->commit();
};

void hbx_admv1320_ctrl::set_lo_sideband(const admv1320_iface::lo_sideband_t sideband)
{
    UHD_LOG_TRACE(_log_id, "Setting LO sideband to " << static_cast<int>(sideband));
    _admv->set_lo_sideband(sideband);
    _admv->commit();
};

void hbx_admv1320_ctrl::set_chip_powerdown(const bool enable)
{
    UHD_LOG_TRACE(_log_id, "Setting chip powerdown to " << enable);
    _admv->set_chip_powerdown(enable);
    _admv->commit();
};

void hbx_admv1320_ctrl::set_rf_hs_powerdown(const bool enable)
{
    UHD_LOG_TRACE(_log_id, "Setting RF HS powerdown to " << enable);
    _admv->set_rf_hs_powerdown(enable);
    _admv->commit();
};

void hbx_admv1320_ctrl::set_lpf_source(const admv1320_iface::filter_sel_t select)
{
    _admv->select_lpf_source(select);
    _admv->commit();
}

void hbx_admv1320_ctrl::set_lpf_cutoff_freq(const uint8_t value)
{
    _admv->set_lpf_cutoff_freq(value);
    _admv->commit();
}

void hbx_admv1320_ctrl::set_hpf_source(const admv1320_iface::filter_sel_t select)
{
    _admv->select_hpf_source(select);
    _admv->commit();
}

void hbx_admv1320_ctrl::set_hpf_cutoff_freq(const uint8_t value)
{
    _admv->set_hpf_cutoff_freq(value);
    _admv->commit();
}

void hbx_admv1320_ctrl::configure(const hbx_tune_map_item_t config, const double freq)
{
    set_rf_band(config.rf_band);
    if (config.rf_band == 2) {
        set_input_mode(admv1320_iface::input_mode_t::BYPASS);
        set_lo_x3_filter(freq);
    } else if (config.rf_band == 3) {
        set_input_mode(admv1320_iface::input_mode_t::BASEBAND);
        set_lo_x3_filter(freq);
    } // else do nothing
}

}}} // namespace uhd::usrp::hbx
