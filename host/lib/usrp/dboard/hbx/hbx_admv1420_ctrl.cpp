//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/usrp/dboard/hbx/hbx_admv1420_ctrl.hpp>

namespace uhd { namespace usrp { namespace hbx {

hbx_admv1420_ctrl::hbx_admv1420_ctrl(size_t start_address,
    hbx_cpld_ctrl::poke_fn_type&& poke_fn,
    hbx_cpld_ctrl::peek_fn_type&& peek_fn,
    hbx_cpld_ctrl::mixer_callback_t&& init_cb,
    const std::string& unique_id)
    : hbx_cpld_ctrl::spi_transactor(
        start_address, std::move(poke_fn), std::move(peek_fn), true)
    , _init_cb(std::move(init_cb))
    , _log_id(unique_id + "::HBX_ADMV1420")
{
    _admv = admv1420_iface::make(
        [this](uint16_t addr, uint16_t data) { this->spi_write(addr, data); },
        [this](uint16_t addr) -> uint16_t { return this->spi_read(addr); },
        unique_id);

    _init_cb(hbx_cpld_ctrl::init_t::INIT);
    // SPI settings
    _admv->set_sdo_level(admv1420_iface::sdo_level_t::V1_8);
    // Power down unused bands/LNA
    _admv->set_powerdown(
        {admv1420_iface::pd_comp_t::DSA1, admv1420_iface::pd_comp_t::RF_LNA_B0}, true);
    _admv->set_powerdown(
        {admv1420_iface::pd_comp_t::RF_LNA_B1, admv1420_iface::pd_comp_t::RF_LNA_B2},
        false);
    _admv->commit();
    // Set internal Vcm close to 1.25 V
    _admv->set_vcm(admv1420_iface::internal_voltage_t::V1_25);
    // Set Vcm to external (not internal)
    _admv->set_bb_amp_output_common_mode_int(false);
    // Disable powerdown on BB amp
    _admv->set_powerdown(
        {admv1420_iface::pd_comp_t::BB_AMP_I, admv1420_iface::pd_comp_t::BB_AMP_Q},
        false);
    // Gain and filter table settings
    _admv->enable_filter_table(false);
    _admv->enable_gain_table(false);
    _admv->set_mix_gate_bias_adj_mode(admv1420_iface::mix_gate_bias_adj_mode_t::AUTO);
    _admv->commit();
    _admv->finalize_init();
    // Pull the enable flag in the CPLD
    _init_cb(hbx_cpld_ctrl::init_t::FINALIZE);
    // We only use lo sideband in HBX band 2
    _admv->set_lo_sideband(admv1420_iface::lo_sideband_t::LSB);

    // General settings for all bands that we are using
    _admv->select_lpf_source(admv1420_iface::filter_sel_t::SPI);
    _admv->set_lpf_cutoff_freq(0);
    _admv->select_hpf_source(admv1420_iface::filter_sel_t::SPI);
    _admv->set_hpf_cutoff_freq(63);
};

void hbx_admv1420_ctrl::set_rf_band(const admv1420_iface::rf_band_t band)
{
    UHD_LOG_TRACE(_log_id, "Setting RF band to " << static_cast<uint16_t>(band));
    _admv->set_rf_band(band);
    _admv->commit();
};

uint8_t hbx_admv1420_ctrl::set_dsa(const admv1420_iface::dsa_t dsa, uint8_t value)
{
    UHD_LOG_TRACE(_log_id,
        "Setting DSA " << static_cast<uint16_t>(dsa) << " to "
                       << static_cast<uint16_t>(value) << "dB");
    auto ret_val = _admv->set_dsa(dsa, value);
    _admv->commit();
    return ret_val;
};

uint8_t hbx_admv1420_ctrl::get_dsa(const admv1420_iface::dsa_t dsa)
{
    auto ret_val = _admv->get_dsa(dsa);
    UHD_LOG_TRACE(_log_id,
        "Getting DSA " << static_cast<uint16_t>(dsa) << " value "
                       << static_cast<uint16_t>(ret_val) << "dB");
    return ret_val;
};

void hbx_admv1420_ctrl::set_lo_sideband(const admv1420_iface::lo_sideband_t sideband)
{
    UHD_LOG_TRACE(_log_id, "Setting LO sideband to " << static_cast<uint16_t>(sideband));
    _admv->set_lo_sideband(sideband);
    _admv->commit();
};

uint8_t hbx_admv1420_ctrl::set_compound_dsa(const uint8_t value)
{
    uint8_t dsa1 = ADMV_DSA_MAX_ATTENUATION;
    uint8_t dsa2 = ADMV1420_DSA2_MAX_ATTENUATION;
    uint8_t dsa3 = ADMV_DSA_MAX_ATTENUATION;
    uint8_t dsa4 = ADMV_DSA_MAX_ATTENUATION;
    uint8_t dsa5 = ADMV_DSA_MAX_ATTENUATION;

    if (_band == hbx_band_t::BAND_1) {
        // In band 1, DSAs 3-5 are used and we distribute the overall attenuation
        // evenly:
        dsa3      = static_cast<uint8_t>(ceil(value / 3.0));
        dsa4      = static_cast<uint8_t>(ceil((value - dsa3) / 2.0));
        dsa5      = value - dsa3 - dsa4;
        _comp_dsa = dsa3 + dsa4 + dsa5;
        UHD_LOG_TRACE(_log_id,
            "Compound DSA " << static_cast<uint16_t>(value) << " distributed to DSA3 "
                            << static_cast<uint16_t>(dsa3) << ", DSA4 "
                            << static_cast<uint16_t>(dsa4) << ", DSA5 "
                            << static_cast<uint16_t>(dsa5) << ".");
    } else if (_band == hbx_band_t::BAND_2) {
        // In band 2, DSAs 2 and 3 are used only. DSA 2 can only do 0 or 6 dB attenuation.
        dsa2 = (value >= ADMV1420_DSA2_MAX_ATTENUATION) ? ADMV1420_DSA2_MAX_ATTENUATION
                                                        : 0;
        dsa3 = value - dsa2;
        _comp_dsa = dsa2 + dsa3;
        UHD_LOG_TRACE(_log_id,
            "Compound DSA " << static_cast<uint16_t>(value) << " distributed to DSA2 "
                            << static_cast<uint16_t>(dsa2) << ", DSA3 "
                            << static_cast<uint16_t>(dsa3) << ".");
    } else {
        UHD_THROW_INVALID_CODE_PATH();
    }
    _admv->set_dsa(admv1420_iface::dsa_t::DSA1, dsa1);
    _admv->set_dsa(admv1420_iface::dsa_t::DSA2, dsa2);
    _admv->set_dsa(admv1420_iface::dsa_t::DSA3, dsa3);
    _admv->set_dsa(admv1420_iface::dsa_t::DSA4, dsa4);
    _admv->set_dsa(admv1420_iface::dsa_t::DSA5, dsa5);
    _admv->commit();
    return _comp_dsa;
}

uint8_t hbx_admv1420_ctrl::get_compound_dsa()
{
    // As individual gains may have chaned in the meantime we need to
    // re-calculate the compound DSA value
    uint8_t dsa2 = get_dsa(admv1420_iface::dsa_t::DSA2);
    uint8_t dsa3 = get_dsa(admv1420_iface::dsa_t::DSA3);
    uint8_t dsa4 = get_dsa(admv1420_iface::dsa_t::DSA4);
    uint8_t dsa5 = get_dsa(admv1420_iface::dsa_t::DSA5);
    if (_band == hbx_band_t::BAND_1) {
        // In band 1, DSAs 3-5 are used
        _comp_dsa = dsa3 + dsa4 + dsa5;
        UHD_LOG_TRACE(_log_id,
            "Compound DSA " << _comp_dsa << " in band " << static_cast<uint16_t>(_band)
                            << " distributed to DSA3 " << static_cast<uint16_t>(dsa3)
                            << ", DSA4 " << static_cast<uint16_t>(dsa4) << ", DSA5 "
                            << static_cast<uint16_t>(dsa5) << ".");
    } else if (_band == hbx_band_t::BAND_2) {
        // In band 2, DSAs 2 and 3 are used only. DSA 2 can only do 0 or 6 dB attenuation.
        _comp_dsa = dsa2 + dsa3;
        UHD_LOG_TRACE(_log_id,
            "Compound DSA " << _comp_dsa << " in band " << static_cast<uint16_t>(_band)
                            << " distributed to DSA2 " << static_cast<uint16_t>(dsa2)
                            << ", DSA3 " << static_cast<uint16_t>(dsa3) << ".");
    } else {
        UHD_THROW_INVALID_CODE_PATH();
    }
    return _comp_dsa;
}

uint8_t hbx_admv1420_ctrl::get_max_compound_dsa()
{
    if (_band == hbx_band_t::BAND_1) {
        // In band 1, DSAs 3-5 are used
        return ADMV_DSA_MAX_ATTENUATION * 3;
    } else if (_band == hbx_band_t::BAND_2) {
        // In band 2, DSAs 2 and 3 are used only. DSA 2 can only do 0 or 6 dB attenuation.
        return ADMV1420_DSA2_MAX_ATTENUATION + ADMV_DSA_MAX_ATTENUATION;
    } else {
        UHD_THROW_INVALID_CODE_PATH();
    }
}

void hbx_admv1420_ctrl::configure(const hbx_band_t hbx_band,
    const admv1420_iface::rf_band_t admv_band,
    const double freq)
{
    _band = hbx_band;
    UHD_LOG_TRACE(_log_id,
        "Setting ADMV1420 RF band = "
            << static_cast<uint16_t>(admv_band)
            << ", HBX band = " << static_cast<uint16_t>(hbx_band) << ", freq = " << freq);
    switch (_band) {
        case hbx_band_t::BAND_0:
            // ADMV1420 should not be exercised at all in rf band 0
            // Gain control takes care to set DSAs to full attenuation in that case.
            break;
        case hbx_band_t::BAND_1:
            _admv->set_if_bb_switch_ctrl(admv1420_iface::bb_switch_t::IF);
            _admv->set_rf_band(admv_band);
            if (admv_band == admv1420_iface::rf_band_t::BAND_1) {
                _admv->set_if_band(admv1420_iface::if_band_t::BAND_0);
            } else if (admv_band == admv1420_iface::rf_band_t::BAND_2) {
                _admv->set_if_band(admv1420_iface::if_band_t::BAND_1);
            } else {
                // For ADMV RF band 0 and 3, the state of the IF band does not matter, so
                // we choose band 0 to make it deterministic.
                _admv->set_if_band(admv1420_iface::if_band_t::BAND_0);
            }
            set_lo_x3_filter(freq);
            break;
        case hbx_band_t::BAND_2:
            _admv->set_if_bb_switch_ctrl(admv1420_iface::bb_switch_t::BASEBAND);
            _admv->set_rf_band(admv_band);
            set_lo_x3_filter(freq);
            break;
        default:
            UHD_THROW_INVALID_CODE_PATH();
    }
}

void hbx_admv1420_ctrl::set_lo_x3_filter(const double freq)
{
    _admv->set_lo_x3_filter(freq);
    _admv->commit();
}

}}} // namespace uhd::usrp::hbx
