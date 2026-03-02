//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/types/direction.hpp>
#include <uhd/utils/assert_has.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/math.hpp>
#include <uhdlib/usrp/dboard/hbx/hbx_cpld_ctrl.hpp>
#include <uhdlib/usrp/dboard/hbx/hbx_expert.hpp>
#include <uhdlib/utils/interpolation.hpp>
#include <algorithm>
#include <array>

using namespace uhd;

namespace uhd { namespace usrp { namespace hbx {
namespace {

/*********************************************************************
 *   Misc/calculative helper functions
 **********************************************************************/
std::string _get_trx_string(const direction_t dir)
{
    if (dir == RX_DIRECTION) {
        return "rx";
    } else if (dir == TX_DIRECTION) {
        return "tx";
    } else {
        UHD_THROW_INVALID_CODE_PATH();
    }
}
uint8_t _get_external_lo_pwr(const double freq, const bool lo_export)
{
    if (lo_export) {
        for (const auto& item : lo_export_power_map) {
            if (freq >= item.first.start() && freq <= item.first.stop()) {
                return item.second;
            }
        }
        UHD_THROW_INVALID_CODE_PATH();
    } else {
        return 0;
    }
}

struct lo_gain_settings_t
{
    double desired_lo_int_power;
    double desired_lo_gain;
};

// Helper function for LO gain settings for both RX and TX. Returns an lo_gain_settings_t
// struct with the internal LO power and LO gain settings according to the corresponding
// frequency and gain maps.
lo_gain_settings_t _get_lo_gain_settings(const double freq,
    const bool lo_import,
    const std::vector<std::pair<uhd::range_t, uint8_t>>& lo_import_gain_map,
    const std::vector<hbx_lo_gain_map_item_t>& lo_gain_map)
{
    lo_gain_settings_t lo_gain_settings{0, 0};

    if (lo_import) {
        // Since we import an LO, the internal one may be silent. It should be disabled in
        // the LO expert already, though.
        lo_gain_settings.desired_lo_int_power = 0;
        for (const auto& item : lo_import_gain_map) {
            if (freq >= item.first.start() && freq <= item.first.stop()) {
                lo_gain_settings.desired_lo_gain = item.second;
                break;
            }
        }
        return lo_gain_settings;
    }

    for (const auto& item : lo_gain_map) {
        if (freq >= item.start_freq && freq <= item.stop_freq) {
            lo_gain_settings.desired_lo_int_power = item.lo_power;
            lo_gain_settings.desired_lo_gain      = item.lo_gain;
            break;
        }
    }

    return lo_gain_settings;
}
} // namespace

/*!---------------------------------------------------------
 * EXPERT RESOLVE FUNCTIONS
 *
 * This sections contains all expert resolve functions.
 * These methods are triggered by any of the bound accessors becoming "dirty",
 * or changing value
 * --------------------------------------------------------
 */
void hbx_scheduling_expert::resolve()
{
    // We currently have no fancy scheduling, but here is where we'd add it if
    // we need to do that (e.g., plan out SYNC pulse timing vs. NCO timing etc.)
    _frontend_time = _command_time;
}

void hbx_lo_expert::resolve()
{
    if (_lo_export.is_dirty() || _lo_import.is_dirty()) {
        // If the LO is imported and we don't export our own LO at the same time, we don't
        // need our own LO anymore and can turn it off to generate silence. Otherwise turn
        // it on.
        _lo_ctrl->set_lo_enabled(!_lo_import || _lo_export);

        _lo_ctrl->set_lo_ports(_lo_export, _lo_import);
        _cpld->set_lo_switches_and_leds(_lo_export, _lo_import, _trx);
    }

    // When ports have been set, we need to (re-)set the frequency to make sure the output
    // power is set properly (which LMX2572's set_frequency() does at the very end).
    const double clipped_lo_freq = std::max(
        LMX2572_MIN_FREQ, std::min(_desired_lo_frequency.get(), LMX2572_MAX_FREQ));
    _coerced_lo_frequency = _lo_ctrl->set_lo_freq(clipped_lo_freq);
    _desired_lo_ext_power = _get_external_lo_pwr(_coerced_lo_frequency, _lo_export);
}

void hbx_tx_gain_programming_expert::resolve()
{
    if (_admv_dsa_in.is_dirty()) {
        // In ADMV1320, the compound DSA consists of DSA1 and DSA2 which both have a
        // maximum value of ADMV_DSA_MAX_ATTENUATION. Therefore 2x this value is the upper
        // limit of the gain range.
        _admv_dsa_out =
            ADMV_DSA_MAX_ATTENUATION * 2
            - _admv1320->set_compound_dsa(ADMV_DSA_MAX_ATTENUATION * 2 - _admv_dsa_in);
        // Query the individual DSAs
        _admv_dsa1_out =
            ADMV_DSA_MAX_ATTENUATION - _admv1320->get_dsa(admv1320_iface::dsa_t::DSA1);
        _admv_dsa2_out =
            ADMV_DSA_MAX_ATTENUATION - _admv1320->get_dsa(admv1320_iface::dsa_t::DSA2);
    }
    if (_admv_dsa1_in.is_dirty()) {
        _admv_dsa1_out = ADMV_DSA_MAX_ATTENUATION
                         - _admv1320->set_dsa(admv1320_iface::dsa_t::DSA1,
                             ADMV_DSA_MAX_ATTENUATION - _admv_dsa1_in);
        _admv_dsa_out = ADMV_DSA_MAX_ATTENUATION * 2 - _admv1320->get_compound_dsa();
    }
    if (_admv_dsa2_in.is_dirty()) {
        _admv_dsa2_out = ADMV_DSA_MAX_ATTENUATION
                         - _admv1320->set_dsa(admv1320_iface::dsa_t::DSA2,
                             ADMV_DSA_MAX_ATTENUATION - _admv_dsa2_in);
        _admv_dsa_out = ADMV_DSA_MAX_ATTENUATION * 2 - _admv1320->get_compound_dsa();
    }

    // All non-ADMV1320 DSAs are ATR-enabled. To use that we need to set the values in the
    // corresponding states (TX and TRX for TX DSAs).
    if (_rf_dsa_in.is_dirty()) {
        for (const size_t idx : {ATR_ADDR_TX, ATR_ADDR_XX}) {
            _rf_dsa_out = RF_DSA_MAX_ATTENUATION
                          - _cpld->set_dsa(idx,
                              hbx_cpld_ctrl::dsa_t::TX_RF_DSA,
                              RF_DSA_MAX_ATTENUATION - _rf_dsa_in);
        }
    }
    if (_lo_dsa_in.is_dirty()) {
        for (const size_t idx : {ATR_ADDR_TX, ATR_ADDR_XX}) {
            _lo_dsa_out = LO_LF_DSA_MAX_ATTENUATION
                          - _cpld->set_dsa(idx,
                              hbx_cpld_ctrl::dsa_t::TX_LO_DSA,
                              LO_LF_DSA_MAX_ATTENUATION - _lo_dsa_in);
        }
    }
    if (_lo_pwr_int_in.is_dirty()) {
        _lo_pwr_int_out = _lo_ctrl->set_lo_output_a_power(_lo_pwr_int_in);
    }
    if (_lo_pwr_ext_in.is_dirty()) {
        _lo_pwr_ext_out = _lo_ctrl->set_lo_output_b_power(_lo_pwr_ext_in);
    }
}

void hbx_rx_gain_programming_expert::resolve()
{
    const uint8_t max_dsa = _admv1420->get_max_compound_dsa();
    if (_admv_dsa_in.is_dirty()) {
        // In ADMV1420, the compound DSA consists of different DSAs in different bands.
        // Therefore we have to query what is the current maximum value.
        _admv_dsa_out = max_dsa - _admv1420->set_compound_dsa(max_dsa - _admv_dsa_in);
        // Query the individual DSAs
        _admv_dsa2_out = ADMV1420_DSA2_MAX_ATTENUATION
                         - _admv1420->get_dsa(admv1420_iface::dsa_t::DSA2);
        _admv_dsa3_out =
            ADMV_DSA_MAX_ATTENUATION - _admv1420->get_dsa(admv1420_iface::dsa_t::DSA3);
        _admv_dsa4_out =
            ADMV_DSA_MAX_ATTENUATION - _admv1420->get_dsa(admv1420_iface::dsa_t::DSA4);
        _admv_dsa5_out =
            ADMV_DSA_MAX_ATTENUATION - _admv1420->get_dsa(admv1420_iface::dsa_t::DSA5);
    }
    if (_admv_dsa2_in.is_dirty()) {
        _admv_dsa2_out = ADMV1420_DSA2_MAX_ATTENUATION
                         - _admv1420->set_dsa(admv1420_iface::dsa_t::DSA2,
                             ADMV1420_DSA2_MAX_ATTENUATION - _admv_dsa2_in);
        _admv_dsa_out = max_dsa - _admv1420->get_compound_dsa();
    }
    if (_admv_dsa3_in.is_dirty()) {
        _admv_dsa3_out = ADMV_DSA_MAX_ATTENUATION
                         - _admv1420->set_dsa(admv1420_iface::dsa_t::DSA3,
                             ADMV_DSA_MAX_ATTENUATION - _admv_dsa3_in);
        _admv_dsa_out = max_dsa - _admv1420->get_compound_dsa();
    }
    if (_admv_dsa4_in.is_dirty()) {
        _admv_dsa4_out = ADMV_DSA_MAX_ATTENUATION
                         - _admv1420->set_dsa(admv1420_iface::dsa_t::DSA4,
                             ADMV_DSA_MAX_ATTENUATION - _admv_dsa4_in);
        _admv_dsa_out = max_dsa - _admv1420->get_compound_dsa();
    }
    if (_admv_dsa5_in.is_dirty()) {
        _admv_dsa5_out = ADMV_DSA_MAX_ATTENUATION
                         - _admv1420->set_dsa(admv1420_iface::dsa_t::DSA5,
                             ADMV_DSA_MAX_ATTENUATION - _admv_dsa5_in);
        _admv_dsa_out = max_dsa - _admv1420->get_compound_dsa();
    }

    // All non-ADMV1420 DSAs are ATR-enabled. To use that we need to set the values in the
    // corresponding states (RX and TRX for RX DSAs).
    if (_rf_dsa_in.is_dirty()) {
        for (const size_t idx : {ATR_ADDR_RX, ATR_ADDR_XX}) {
            _rf_dsa_out = RF_DSA_MAX_ATTENUATION
                          - _cpld->set_dsa(idx,
                              hbx_cpld_ctrl::dsa_t::RX_RF_DSA,
                              RF_DSA_MAX_ATTENUATION - _rf_dsa_in);
        }
    }
    if (_lo_dsa_in.is_dirty()) {
        for (const size_t idx : {ATR_ADDR_RX, ATR_ADDR_XX}) {
            _lo_dsa_out = LO_LF_DSA_MAX_ATTENUATION
                          - _cpld->set_dsa(idx,
                              hbx_cpld_ctrl::dsa_t::RX_LO_DSA,
                              LO_LF_DSA_MAX_ATTENUATION - _lo_dsa_in);
        }
    }
    if (_lo_pwr_int_in.is_dirty()) {
        _lo_pwr_int_out = _lo_ctrl->set_lo_output_a_power(_lo_pwr_int_in);
    }
    if (_lo_pwr_ext_in.is_dirty()) {
        _lo_pwr_ext_out = _lo_ctrl->set_lo_output_b_power(_lo_pwr_ext_in);
    }
    if (_lf_dsa1.is_dirty()) {
        for (const size_t idx : {ATR_ADDR_RX, ATR_ADDR_XX}) {
            _lf_dsa1_out = LO_LF_DSA_MAX_ATTENUATION
                           - _cpld->set_dsa(idx,
                               hbx_cpld_ctrl::dsa_t::RX_LF_DSA1,
                               LO_LF_DSA_MAX_ATTENUATION - _lf_dsa1);
        }
    }
    if (_lf_dsa2.is_dirty()) {
        for (const size_t idx : {ATR_ADDR_RX, ATR_ADDR_XX}) {
            _lf_dsa2_out = LO_LF_DSA_MAX_ATTENUATION
                           - _cpld->set_dsa(idx,
                               hbx_cpld_ctrl::dsa_t::RX_LF_DSA2,
                               LO_LF_DSA_MAX_ATTENUATION - _lf_dsa2);
        }
    }
}

void hbx_tx_gain_expert::resolve()
{
    // Don't touch the individual gains if we're not in default gain profile.
    if (_profile != HBX_GAIN_PROFILE_DEFAULT) {
        return;
    }
    // Whenever frequency or gain changes, we go into here.
    // Frequencies up to 6 GHz only have one DSA available for regulating the gain, for
    // that we don't need gain tables.

    // If a user passes in a gain value, we have to set the Power API tracking mode. If
    // this resolver is triggered by the power manager, it is aware of the tracking mode
    // change and will set it back to TRACK_POWER after the gain has been set.
    if (_gain_all_in.is_dirty()) {
        _power_mgr->set_tracking_mode(uhd::usrp::pwr_cal_mgr::tracking_mode::TRACK_GAIN);
    }

    // The property tree remembers the value set, even if we cannot make this in this
    // frequency band. This is documented and helps when retuning.
    _gain_all_out = _gain_all_in;
    if (_freq <= 6e9) {
        // The actual gain in this range is set by the RF DSA, which has a step width of 2
        // dB.
        // For a flatter gain response, we use the maximum values of the individual bands
        // to align with. To do this, we need to calculate the offset from the
        // overall gain range to the RF DSA gain range which is defined by the RF DSA
        // maximum attenuation.
        const auto offset = HBX_TX_GAIN_RANGE.stop() - RF_DSA_MAX_ATTENUATION;
        _rf_dsa_gain_out  = uhd::gain_range_t(0.0, RF_DSA_MAX_ATTENUATION, 2.0)
                               .clip(_gain_all_in - offset, true);
        _admv_gain_out = 0.0; // No ADMV DSA in this range, use maximum attenuation
    } else {
        // Now for everything above 6 GHz:
        // We iterate through the gain map and find the correct frequency range. If we
        // find the matching gain index, we stop there and use this configuration,
        // otherwise we'll stop after we've passed the right frequency range. But in that
        // case we'll have the highest possible gain setting.
        hbx_tx_gain_map_item_t _gain_settings = TX_GAIN_MAP.front();
        for (const auto& item : TX_GAIN_MAP) {
            if (_freq >= item.start_freq && _freq <= item.stop_freq) {
                _gain_settings = item;
                if (item.gain_idx >= _gain_all_out) {
                    break;
                }
            } else if (item.start_freq > _freq) {
                // We have passed the frequency range, so we can stop here.
                break;
            }
        }
        _rf_dsa_gain_out = _gain_settings.rf_dsa1;
        _admv_gain_out   = _gain_settings.admv_dsa;
    }

    const auto lo_gain_settings =
        _get_lo_gain_settings(_freq, _lo_import, tx_lo_import_gain_map, tx_lo_gain_map);
    _desired_lo_int_power = lo_gain_settings.desired_lo_int_power;
    _desired_lo_gain      = lo_gain_settings.desired_lo_gain;
}

void hbx_rx_gain_expert::resolve()
{
    // Don't touch the individual gains if we're not in default gain profile.
    if (_profile != HBX_GAIN_PROFILE_DEFAULT) {
        return;
    }
    // If a user passes in a gain value, we have to set the Power API tracking mode. If
    // this resolver is triggered by the power manager, it is aware of the tracking mode
    // change and will set it back to TRACK_POWER after the gain has been set.
    if (_gain_all_in.is_dirty()) {
        _power_mgr->set_tracking_mode(uhd::usrp::pwr_cal_mgr::tracking_mode::TRACK_GAIN);
    }

    // In comparison to the TX side, we have a more complex gain map here, no regularities
    // that we could follow. Therefore a lookup for all gain indexes is needed.
    hbx_rx_gain_map_item_t _gain_settings = RX_GAIN_MAP.front();
    for (const auto& item : RX_GAIN_MAP) {
        if (_freq > item.start_freq && _freq <= item.stop_freq) {
            _gain_settings = item;
            if (item.gain_idx == static_cast<std::size_t>(_gain_all_in.get())) {
                break;
            }
        } else if (item.start_freq >= _freq) {
            // This ensures we coerce to the maximum gain available for the current band.
            break;
        }
    }
    _gain_all_out = _gain_all_in;
    _lf_dsa1_out  = _gain_settings.lf_dsa1;
    _lf_dsa2_out  = _gain_settings.lf_dsa2;
    _rf_dsa_out   = _gain_settings.rf_dsa1;
    _admv_dsa_out = _gain_settings.admv_dsa;

    const auto lo_gain_settings =
        _get_lo_gain_settings(_freq, _lo_import, rx_lo_import_gain_map, rx_lo_gain_map);
    _desired_lo_int_power = lo_gain_settings.desired_lo_int_power;
    _desired_lo_gain      = lo_gain_settings.desired_lo_gain;
}

void hbx_rfdc_freq_expert::resolve()
{
    // This expert only sets the converters in Real mode as the converters in
    // complex mode are driven in base-band mode.
    _rfdc_freq_coerced = _rpcc->rfdc_set_nco_freq(_get_trx_string(_trx),
        _db_idx,
        0,
        _rfdc_freq_desired,
        static_cast<size_t>(uhd::usrp::x400::ch_mode::REAL));
}

void hbx_rx_programming_expert::resolve()
{
    if (_antenna == ANTENNA_TXRX) {
        _cpld->set_rx_antenna_switches(ATR_ADDR_0X, rx_ant_t::TERM, false);
        _cpld->set_rx_antenna_switches(ATR_ADDR_RX, rx_ant_t::TRX, false);
        _cpld->set_rx_antenna_switches(ATR_ADDR_TX, rx_ant_t::TERM, false);
        _cpld->set_rx_antenna_switches(ATR_ADDR_XX, rx_ant_t::TRX, false);
    } else if (_antenna == ANTENNA_RX) {
        _cpld->set_rx_antenna_switches(ATR_ADDR_0X, rx_ant_t::TERM, false);
        _cpld->set_rx_antenna_switches(ATR_ADDR_RX, rx_ant_t::RX, false);
        _cpld->set_rx_antenna_switches(ATR_ADDR_TX, rx_ant_t::TERM, false);
        _cpld->set_rx_antenna_switches(ATR_ADDR_XX, rx_ant_t::RX, false);
    } else if (_antenna == ANTENNA_CAL_LOOPBACK) {
        _cpld->set_rx_antenna_switches(ATR_ADDR_0X, rx_ant_t::TERM, false);
        _cpld->set_rx_antenna_switches(ATR_ADDR_RX, rx_ant_t::LOOPBACK, false);
        _cpld->set_rx_antenna_switches(ATR_ADDR_TX, rx_ant_t::TERM, false);
        _cpld->set_rx_antenna_switches(ATR_ADDR_XX, rx_ant_t::LOOPBACK, false);
    } else if (_antenna == ANTENNA_TERMINATION) {
        _cpld->set_rx_antenna_switches(ATR_ADDR_0X, rx_ant_t::TERM, false);
        _cpld->set_rx_antenna_switches(ATR_ADDR_RX, rx_ant_t::TERM, false);
        _cpld->set_rx_antenna_switches(ATR_ADDR_TX, rx_ant_t::TERM, false);
        _cpld->set_rx_antenna_switches(ATR_ADDR_XX, rx_ant_t::TERM, false);
    } else if (_antenna == ANTENNA_SYNC_INT) {
        _cpld->set_rx_antenna_switches(ATR_ADDR_0X, rx_ant_t::TERM, true);
        _cpld->set_rx_antenna_switches(ATR_ADDR_RX, rx_ant_t::TERM, true);
        _cpld->set_rx_antenna_switches(ATR_ADDR_TX, rx_ant_t::TERM, true);
        _cpld->set_rx_antenna_switches(ATR_ADDR_XX, rx_ant_t::TERM, true);
    } else {
        UHD_THROW_INVALID_CODE_PATH();
    }
    _update_leds();
}

void hbx_rx_programming_expert::_update_leds()
{
    // Like in FBX and ZBX, we default to the RX1 for all RX antenna values that are not
    // TX/RX0
    const bool rx_on_trx = _antenna == ANTENNA_TXRX;

    // In case we have the loopback path enabled, we don't want any red LEDs to be on as
    // this might incorrectly indicate we're outputting any signal. Additionally, since we
    // use the TRX state for ADC self-cal, we don't want any LED to be on.
    // nlb = no loopback
    const bool nlb = _antenna != ANTENNA_CAL_LOOPBACK;
    // clang-format off
    // G==Green, R==Red          RX1              TX/RX-G         TX/RX-R
    _cpld->set_leds(ATR_ADDR_0X, false,           false,          false    );
    _cpld->set_leds(ATR_ADDR_RX, !rx_on_trx&&nlb, rx_on_trx&&nlb, false    );
    _cpld->set_leds(ATR_ADDR_TX, false,           false,          true&&nlb);
    _cpld->set_leds(ATR_ADDR_XX, !rx_on_trx&&nlb, rx_on_trx&&nlb, true&&nlb);
    // clang-format on
}

void hbx_tx_programming_expert::resolve()
{
    if (_antenna == ANTENNA_TXRX) {
        // clang-format off
        //                             State        Loopback   TX
        _cpld->set_tx_antenna_switches(ATR_ADDR_0X, false,     false);
        _cpld->set_tx_antenna_switches(ATR_ADDR_RX, false,     false);
        _cpld->set_tx_antenna_switches(ATR_ADDR_TX, false,     true );
        _cpld->set_tx_antenna_switches(ATR_ADDR_XX, false,     true );
        // clang-format on
    } else if (_antenna == ANTENNA_CAL_LOOPBACK) {
        // clang-format off
        //                             State        Loopback   TX
        _cpld->set_tx_antenna_switches(ATR_ADDR_0X, false,     false);
        _cpld->set_tx_antenna_switches(ATR_ADDR_RX, true,      false);
        _cpld->set_tx_antenna_switches(ATR_ADDR_TX, true,      false);
        _cpld->set_tx_antenna_switches(ATR_ADDR_XX, true,      false);
        // clang-format on
    } else {
        UHD_THROW_INVALID_CODE_PATH();
    }
}

void hbx_tx_band_expert::resolve()
{
    _coerced_frequency = std::max(HBX_MIN_FREQ, std::min(_frequency.get(), HBX_MAX_FREQ));
    // Iterate through the tx_tune_map to find the appropriate configuration
    for (const auto& item : tx_tune_map) {
        if (_coerced_frequency >= item.start_tune_freq
            && _coerced_frequency <= item.stop_tune_freq) {
            // Use hbx_cpld_ctrl object to set all RF path switches
            _cpld->set_tx_rf_path_switches(item);
            // Configures the ADMV1320
            _admv1320_ctrl->configure(item, _coerced_frequency);
            if (item.rf_band == 0) {
                // Only Band 0 uses the NCO of the RFDC to tune the frequency
                _desired_rfdc_frequency = _coerced_frequency;
            } else {
                _desired_lo_frequency = _coerced_frequency / item.lo_divider;
            }

            _rpcc->set_data_path(item.rfdc_mode, "tx");

            // Exit after setting the switches
            return;
        }
    }

    // If no matching frequency range is found, raise an error
    UHD_THROW_INVALID_CODE_PATH();
}

void hbx_rx_band_expert::resolve()
{
    // Coerce the frequency to ensure it is within the valid range
    _coerced_frequency = std::max(HBX_MIN_FREQ, std::min(_frequency.get(), HBX_MAX_FREQ));

    // Iterate through the rx_tune_map to find the appropriate configuration
    for (const auto& item : rx_tune_map) {
        if (_coerced_frequency >= item.start_tune_freq
            && _coerced_frequency <= item.stop_tune_freq) {
            // Use hbx_cpld_ctrl object to set all RF path switches
            _cpld->set_rx_rf_path_switches(item);

            if (item.rf_band == 0) {
                // Only Band 0 uses the NCO of the RFDC to tune the frequency
                _desired_rfdc_frequency = _coerced_frequency;
            } else {
                // Required settings for rf band 1 and 2
                _desired_lo_frequency = _coerced_frequency / item.lo_divider;
                // Configures the ADMV1420
                _admv1420_ctrl->configure(
                    static_cast<hbx_admv1420_ctrl::hbx_band_t>(item.rf_band),
                    static_cast<admv1420_iface::rf_band_t>(item.admv_band),
                    _coerced_frequency);

                if (item.rf_band == 1) {
                    // Call set_lo_matching for only band 1
                    _ltc->set_lo_matching(_coerced_frequency);
                }
            }

            // Set the data path to Real or IQ mode.
            _rpcc->set_data_path(item.rfdc_mode, "rx");

            // We know that in band 0 we have an IQ swap, so we need to swap it back.
            _rpcc->enable_iq_swap(item.rf_band == 0, "rx", 0);

            // Exit after setting the switches
            return;
        }
    }

    // If no matching frequency range is found, raise an error
    UHD_THROW_INVALID_CODE_PATH();
}

void hbx_iq_dc_correction_expert::resolve()
{
    // If we are in our "real" path, then reset the filter, otherwise take chosen filters.
    const auto coeffs = (_freq > HBX_SWITCH_FREQ) ? _coeffs.get() : IQ_DC_DEFAULT_VALUES;

    if (_num_iq_coeffs < coeffs.coeffs.size()) {
        throw uhd::runtime_error("Number of coefficients is less than the "
                                 "length of the coefficient arrays.");
    }
    UHD_LOG_DEBUG(get_name(),
        "Applying IQ and DC compensation for "
            << (_trx == RX_DIRECTION ? "RX" : "TX") << " with " << coeffs.coeffs.size()
            << " coefficients and group delay: " << coeffs.group_delay);
    // Apply the filter coefficients to the radio.
    const uint32_t iinline_coeff = _coeff_to_fixed(coeffs.scaling_coeff);
    _poke32(_iq_offset_base + IINLINE_COEFF_REG_OFFSET, iinline_coeff);
    for (size_t i = 0; i < _num_iq_coeffs; ++i) {
        // Write the coefficients in reverse order.
        const size_t idx = _num_iq_coeffs - 1 - i;
        const uint32_t qinline_coeff =
            (idx < coeffs.coeffs.size()) ? _coeff_to_fixed(coeffs.coeffs[idx].imag()) : 0;
        _poke32(_iq_offset_base + QINLINE_COEFF_REG_OFFSET, qinline_coeff);
        const uint32_t icross_coeff =
            (idx < coeffs.coeffs.size()) ? _coeff_to_fixed(coeffs.coeffs[idx].real()) : 0;
        _poke32(_iq_offset_base + ICROSS_COEFF_REG_OFFSET, icross_coeff);
    }
    _poke32(_iq_offset_base + GROUP_DELAY_REG_OFFSET,
        static_cast<uint32_t>(coeffs.group_delay));

    // Apply the DC compensation to the radio.
    const uint32_t poke_value = _iq_to_dc_offset(coeffs.dc_offset);
    UHD_LOG_DEBUG(get_name(),
        "DC offset (I: " << coeffs.dc_offset.real() << ", Q: " << coeffs.dc_offset.imag()
                         << ") -> poke value: 0x" << std::hex << poke_value);
    _poke32(_dc_offset_base + DC_VALUE_OFFSET, poke_value);
}

uint32_t hbx_iq_dc_correction_expert::_coeff_to_fixed(const double coeff) const
{
    return static_cast<uint32_t>(coeff * (1 << COEFFS_FRAC_BITS))
           & ((1 << COEFF_WIDTH) - 1);
}

uint32_t hbx_iq_dc_correction_expert::_iq_to_dc_offset(
    const std::complex<double>& cplx_offset) const
{
    const double offset_i =
        cplx_offset.real() - _rfdc_dc_conv_offset.real() / RFDC_DC_CONV_FACTOR;
    const double offset_q =
        cplx_offset.imag() - _rfdc_dc_conv_offset.imag() / RFDC_DC_CONV_FACTOR;
    const int32_t i_sample = static_cast<int32_t>(std::lrint(offset_i * (1 << 15)));
    const int32_t q_sample = static_cast<int32_t>(std::lrint(offset_q * (1 << 15)));

    const uint32_t i_u16 = static_cast<uint32_t>(static_cast<uint16_t>(i_sample));
    const uint32_t q_u16 = static_cast<uint32_t>(static_cast<uint16_t>(q_sample));
    return (q_u16 << 16) | i_u16;
}

void hbx_sync_expert::resolve()
{
    // Local helper consts
    constexpr std::array<rfdc_control::rfdc_type, 2> ncos{
        {rfdc_control::rfdc_type::RX0, rfdc_control::rfdc_type::TX0}};

    // Now do some timing checks
    bool chan_needs_sync = _fe_time != uhd::time_spec_t::ASAP;

    // If there's no command time, no need to synchronize anything
    if (!chan_needs_sync) {
        UHD_LOG_TRACE(get_name(), "No command time: Skipping phase sync.");
        return;
    }

    std::set<rfdc_control::rfdc_type> ncos_to_sync;
    std::set<rfdc_control::rfdc_type> gearboxes_to_sync;
    ncos_to_sync.clear();
    gearboxes_to_sync.clear();
    for (const auto& nco_idx : ncos) {
        if (_nco_freqs.at(nco_idx).is_dirty()) {
            ncos_to_sync.insert(nco_idx);
        }
    }
    UHD_LOG_TRACE(get_name(), "Syncing 1 channel: " << ncos_to_sync.size() << " NCO(s).");

    // Sync the gearboxes:
    // Gearboxes are special, because they only need to be synchronized once
    // per session, assuming the command time has been set. Unfortunately we
    // have no way here to know if the timekeeper time was updated, but it is
    // well documented that in order to synchronize devices, one first has to
    // make sure the timekeepers are running in sync (by calling
    // set_time_next_pps() accordingly).
    // The logic we use here is that we will always have to update the NCO when
    // doing a synced tune, so we update all the gearboxes for the NCOs -- but
    // only if they have not yet been synchronized.
    if (!_adcs_synced || !_dacs_synced) {
        // In ncos, element 0 in each array element is RX
        const auto rx = ncos.at(0);
        if (ncos_to_sync.count(rx) && !_adcs_synced) {
            gearboxes_to_sync.insert(rx);
            // Technically, they're not synced yet but this saves us from
            // having to look up which RFDCs map to RX again later
            _adcs_synced = true;
        }
        // In ncos, element 0 in each array element is TX
        const auto tx = ncos.at(1);
        if (ncos_to_sync.count(tx) && !_dacs_synced) {
            gearboxes_to_sync.insert(tx);
            // Technically, they're not synced yet but this saves us from
            // having to look up which RFDCs map to TX again later
            _dacs_synced = true;
        }
        if (!gearboxes_to_sync.empty()) {
            UHD_LOG_TRACE(get_name(),
                "Resetting " << gearboxes_to_sync.size() << " gearboxes. ADCs synced: "
                             << (_adcs_synced ? "True" : "False")
                             << "; DACs synced: " << (_dacs_synced ? "True" : "False"));
            _rfdcc->reset_gearboxes(
                std::vector<rfdc_control::rfdc_type>(
                    gearboxes_to_sync.cbegin(), gearboxes_to_sync.cend()),
                _fe_time);
        }
    }

    if (!ncos_to_sync.empty()) {
        _rfdcc->reset_ncos(std::vector<rfdc_control::rfdc_type>(
                               ncos_to_sync.cbegin(), ncos_to_sync.cend()),
            _fe_time);
    }
}
}}} // namespace uhd::usrp::hbx
