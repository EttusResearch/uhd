//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "rhodium_constants.hpp"
#include "rhodium_radio_control.hpp"
#include <uhd/utils/math.hpp>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::rfnoc;
using namespace uhd::math::fp_compare;

namespace {

/* Note on the RX filter bank:
 *
 * The RX path has 8 bands, which we call BAND0 through BAND7. BAND0 is the
 * lowest frequency band. BAND7 is the highest frequency band.
 *
 * BAND0 is also identical with the "low band", i.e., the frequency range in
 * which we shift the signal up to an IF before mixing it back down to DC.
 *
 * The following constants define lower cutoff frequencies for each band.
 * RHODIUM_RX_BAND1_MIN_FREQ is the cutover frequency for switching from
 * BAND0 to BAND1, and so on.
 *
 * Bands 1-7 have both high- and low-pass filters (effectively band
 * passes). Band 0 has only low-pass filters. Frequencies have been
 * chosen to allow as much of the full bandwidth through unattenuated.
 *
 * Switch selection logic for these bands can be found in
 * rhodium_radio_control_impl::_update_rx_freq_switches()
 */
constexpr double RHODIUM_RX_BAND0_MIN_FREQ = RHODIUM_MIN_FREQ;
constexpr double RHODIUM_RX_BAND1_MIN_FREQ = 450e6;
constexpr double RHODIUM_RX_BAND2_MIN_FREQ = 760e6;
constexpr double RHODIUM_RX_BAND3_MIN_FREQ = 1100e6;
constexpr double RHODIUM_RX_BAND4_MIN_FREQ = 1410e6;
constexpr double RHODIUM_RX_BAND5_MIN_FREQ = 2050e6;
constexpr double RHODIUM_RX_BAND6_MIN_FREQ = 3000e6;
constexpr double RHODIUM_RX_BAND7_MIN_FREQ = 4500e6;

/* Note on the TX filter bank:
 *
 * The TX path has 8 bands, which we call BAND0 through BAND7. BAND0 is the
 * lowest frequency band. BAND7 is the highest frequency band.
 *
 * BAND0 is also identical with the "low band", i.e., the frequency range in
 * which we shift the signal up to an IF before mixing it back down to DC.
 *
 * The following constants define lower cutoff frequencies for each band.
 * RHODIUM_TX_BAND1_MIN_FREQ is the cutover frequency for switching from
 * BAND0 to BAND1, and so on.
 *
 * All filters on the TX filter bank are low pass filters (no high pass
 * filters). Frequencies have been chosen to allow as much of the full
 * bandwidth through unattenuated.
 *
 * Switch selection logic for these bands can be found in
 * rhodium_radio_control_impl::_update_tx_freq_switches()
 */
constexpr double RHODIUM_TX_BAND0_MIN_FREQ = RHODIUM_MIN_FREQ;
constexpr double RHODIUM_TX_BAND1_MIN_FREQ = 450e6;
constexpr double RHODIUM_TX_BAND2_MIN_FREQ = 650e6;
constexpr double RHODIUM_TX_BAND3_MIN_FREQ = 1000e6;
constexpr double RHODIUM_TX_BAND4_MIN_FREQ = 1350e6;
constexpr double RHODIUM_TX_BAND5_MIN_FREQ = 1900e6;
constexpr double RHODIUM_TX_BAND6_MIN_FREQ = 3000e6;
constexpr double RHODIUM_TX_BAND7_MIN_FREQ = 4100e6;
} // namespace

rhodium_radio_control_impl::rx_band rhodium_radio_control_impl::_map_freq_to_rx_band(
    const double freq)
{
    const auto freq_compare = freq_compare_epsilon(freq);

    if (freq_compare < RHODIUM_RX_BAND0_MIN_FREQ) {
        return rx_band::RX_BAND_INVALID;
    } else if (freq_compare < RHODIUM_RX_BAND1_MIN_FREQ) {
        return rx_band::RX_BAND_0;
    } else if (freq_compare < RHODIUM_RX_BAND2_MIN_FREQ) {
        return rx_band::RX_BAND_1;
    } else if (freq_compare < RHODIUM_RX_BAND3_MIN_FREQ) {
        return rx_band::RX_BAND_2;
    } else if (freq_compare < RHODIUM_RX_BAND4_MIN_FREQ) {
        return rx_band::RX_BAND_3;
    } else if (freq_compare < RHODIUM_RX_BAND5_MIN_FREQ) {
        return rx_band::RX_BAND_4;
    } else if (freq_compare < RHODIUM_RX_BAND6_MIN_FREQ) {
        return rx_band::RX_BAND_5;
    } else if (freq_compare < RHODIUM_RX_BAND7_MIN_FREQ) {
        return rx_band::RX_BAND_6;
    } else if (freq_compare <= RHODIUM_MAX_FREQ) {
        return rx_band::RX_BAND_7;
    } else {
        return rx_band::RX_BAND_INVALID;
    }
}

rhodium_radio_control_impl::tx_band rhodium_radio_control_impl::_map_freq_to_tx_band(
    const double freq)
{
    const auto freq_compare = freq_compare_epsilon(freq);

    if (freq_compare < RHODIUM_TX_BAND0_MIN_FREQ) {
        return tx_band::TX_BAND_INVALID;
    } else if (freq_compare < RHODIUM_TX_BAND1_MIN_FREQ) {
        return tx_band::TX_BAND_0;
    } else if (freq_compare < RHODIUM_TX_BAND2_MIN_FREQ) {
        return tx_band::TX_BAND_1;
    } else if (freq_compare < RHODIUM_TX_BAND3_MIN_FREQ) {
        return tx_band::TX_BAND_2;
    } else if (freq_compare < RHODIUM_TX_BAND4_MIN_FREQ) {
        return tx_band::TX_BAND_3;
    } else if (freq_compare < RHODIUM_TX_BAND5_MIN_FREQ) {
        return tx_band::TX_BAND_4;
    } else if (freq_compare < RHODIUM_TX_BAND6_MIN_FREQ) {
        return tx_band::TX_BAND_5;
    } else if (freq_compare < RHODIUM_TX_BAND7_MIN_FREQ) {
        return tx_band::TX_BAND_6;
    } else if (freq_compare <= RHODIUM_MAX_FREQ) {
        return tx_band::TX_BAND_7;
    } else {
        return tx_band::TX_BAND_INVALID;
    }
}

bool rhodium_radio_control_impl::_is_rx_lowband(const double freq)
{
    return _map_freq_to_rx_band(freq) == rx_band::RX_BAND_0;
}

bool rhodium_radio_control_impl::_is_tx_lowband(const double freq)
{
    return _map_freq_to_tx_band(freq) == tx_band::TX_BAND_0;
}
