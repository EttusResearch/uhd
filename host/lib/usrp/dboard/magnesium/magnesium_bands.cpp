//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

// The band plan

#include "magnesium_radio_ctrl_impl.hpp"
#include "magnesium_constants.hpp"
#include <uhd/utils/math.hpp>

/*
 * Magnesium Rev C frequency bands:
 *
 * RX IF frequency is 2.4418 GHz. Have 80 MHz of bandwidth for loband.
 * TX IF frequency is 1.8-2.1 GHz (1.95 GHz is best).
 *
 * For RX:
 *     Band      SW2-AB SW3-ABC SW4-ABC SW5-ABCD SW6-ABC SW7-AB SW8-AB MIX
 *     WB        RF1 01 OFF 111 NA  --- NA  ---- RF3 001 RF2 01 RF2 01 0
 *     LB        RF2 10 RF5 100 NA  --- RF3 0010 RF1 100 RF1 10 RF1 10 1
 *     440-530   RF2 10 RF2 001 NA  --- RF1 1000 RF1 100 RF2 01 RF2 01 0
 *     650-1000  RF2 10 RF6 101 NA  --- RF4 0001 RF1 100 RF2 01 RF2 01 0
 *     1100-1575 RF2 10 RF4 011 NA  --- RF2 0100 RF1 100 RF2 01 RF2 01 0
 *     1600-2250 RF2 10 RF3 010 RF2 010 NA  ---- RF2 010 RF2 01 RF2 01 0
 *     2100-2850 RF2 10 RF1 000 RF1 100 NA  ---- RF2 010 RF2 01 RF2 01 0
 *     2700+     RF3 11 OFF 111 RF3 001 NA  ---- RF2 010 RF2 01 RF2 01 0
 *
 * For TX:
 *     Band      SW5-AB SW4-AB SW3-X SW2-ABCD SW1-AB SWTRX-AB MIX
 *     WB        RF1 10 RF2 01 RF1 0 NA  ---- SHD 00 RF4   11 0
 *     LB        RF2 01 RF1 10 RF2 1 RF3 0010 RF3 11 RF1   00 1
 *     <800      RF1 10 RF2 01 RF2 1 RF3 0010 RF3 11 RF1   00 0
 *     800-1700  RF1 10 RF2 01 RF2 1 RF2 0100 RF2 10 RF1   00 0
 *     1700-3400 RF1 10 RF2 01 RF2 1 RF1 1000 RF1 01 RF1   00 0
 *     3400-6400 RF1 10 RF2 01 RF2 1 RF4 0001 SHD 00 RF2   10 0
 */

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::rfnoc;
using namespace uhd::math::fp_compare;

namespace {
    /* Note on the RX filter bank:
     *
     * The RX path has 7 bands, which we call BAND0 through BAND7. BAND0 is the
     * lowest frequency band (it goes through F44, the 490 MHz low pass filter,
     * on the first channel). BAND7 is the highest frequency band, it goes
     * through the 2.7 GHz high pass filter (F43 on the first channel).
     *
     * For all frequencies, there are gain values where we bypass the filter
     * bank. In this case, the band setting does not apply (does not have any
     * meaning).
     *
     * The lowband, when not disabling the filter bank, always goes through
     * BAND0, but there are non-lowband frequencies which can also go through
     * BAND0.
     *
     * The following constants define lower cutoff frequencies for each band.
     * BAND0 does not have a lower cutoff frequency, it is implied by
     * MAGNESIUM_MIN_FREQ. MAGNESIUM_RX_BAND1_MIN_FREQ is the cutover frequency
     * for switching from BAND0 to BAND1, and so on.
     *
     * Bands 1-6 have both high- and low-pass filters (effectively band
     * passes). Frequencies need to be chosen to allow as much of the full
     * bandwidth through unattenuated.
     */
    constexpr double MAGNESIUM_RX_BAND1_MIN_FREQ = 430e6;
    constexpr double MAGNESIUM_RX_BAND2_MIN_FREQ = 600e6;
    constexpr double MAGNESIUM_RX_BAND3_MIN_FREQ = 1050e6;
    constexpr double MAGNESIUM_RX_BAND4_MIN_FREQ = 1600e6;
    constexpr double MAGNESIUM_RX_BAND5_MIN_FREQ = 2100e6;
    constexpr double MAGNESIUM_RX_BAND6_MIN_FREQ = 2700e6;

    /* Note on the TX filter bank:
     *
     * The TX path has 4 bands, which we call BAND0 through BAND3.
     * For all frequencies, there are gain values where we bypass the filter
     * bank. In this case, the band setting does not apply (does not have any
     * meaning).
     *
     * The lowband, when not disabling the filter bank, always goes through
     * BAND0, but there are non-lowband frequencies which can also go through
     * BAND0.
     *
     * The following constants define lower cutoff frequencies for each band.
     * BAND0 does not have a lower cutoff frequency, it is implied by
     * MAGNESIUM_MIN_FREQ. MAGNESIUM_TX_BAND1_MIN_FREQ is the cutover frequency
     * for switching from BAND0 to BAND1, and so on.
     *
     * On current Magnesium revisions, all filters on the TX filter bank are
     * low pass filters (no high pass filters).
     * Frequencies need to be chosen to allow as much of the full bandwidth
     * through unattenuated (so don't go all the way up to the cutoff frequency
     * of that filter, OK).
     */
    constexpr double MAGNESIUM_TX_BAND1_MIN_FREQ = 723.17e6;
    constexpr double MAGNESIUM_TX_BAND2_MIN_FREQ = 1623.17e6;
    constexpr double MAGNESIUM_TX_BAND3_MIN_FREQ = 3323.17e6;
}

magnesium_radio_ctrl_impl::rx_band
magnesium_radio_ctrl_impl::_map_freq_to_rx_band(const double freq) {
    magnesium_radio_ctrl_impl::rx_band band;

    if (fp_compare_epsilon<double>(freq) < MAGNESIUM_MIN_FREQ) {
        band = rx_band::INVALID_BAND;
    } else if (fp_compare_epsilon<double>(freq) < MAGNESIUM_LOWBAND_FREQ) {
        band = rx_band::LOWBAND;
    } else if (fp_compare_epsilon<double>(freq) < MAGNESIUM_RX_BAND1_MIN_FREQ) {
        band = rx_band::BAND0;
    } else if (fp_compare_epsilon<double>(freq) < MAGNESIUM_RX_BAND2_MIN_FREQ) {
        band = rx_band::BAND1;
    } else if (fp_compare_epsilon<double>(freq) < MAGNESIUM_RX_BAND3_MIN_FREQ) {
        band = rx_band::BAND2;
    } else if (fp_compare_epsilon<double>(freq) < MAGNESIUM_RX_BAND4_MIN_FREQ) {
        band = rx_band::BAND3;
    } else if (fp_compare_epsilon<double>(freq) < MAGNESIUM_RX_BAND5_MIN_FREQ) {
        band = rx_band::BAND4;
    } else if (fp_compare_epsilon<double>(freq) < MAGNESIUM_RX_BAND6_MIN_FREQ) {
        band = rx_band::BAND5;
    } else if (fp_compare_epsilon<double>(freq) <= MAGNESIUM_MAX_FREQ) {
        band = rx_band::BAND6;
    } else {
        band = rx_band::INVALID_BAND;
    }

    return band;
}

magnesium_radio_ctrl_impl::tx_band
magnesium_radio_ctrl_impl::_map_freq_to_tx_band(const double freq) {
    magnesium_radio_ctrl_impl::tx_band band;

    if (fp_compare_epsilon<double>(freq) < MAGNESIUM_MIN_FREQ) {
        band = tx_band::INVALID_BAND;
    } else if (fp_compare_epsilon<double>(freq) < MAGNESIUM_LOWBAND_FREQ) {
        band = tx_band::LOWBAND;
    } else if (fp_compare_epsilon<double>(freq) < MAGNESIUM_TX_BAND1_MIN_FREQ) {
        band = tx_band::BAND0;
    } else if (fp_compare_epsilon<double>(freq) < MAGNESIUM_TX_BAND2_MIN_FREQ) {
        band = tx_band::BAND1;
    } else if (fp_compare_epsilon<double>(freq) < MAGNESIUM_TX_BAND3_MIN_FREQ) {
        band = tx_band::BAND2;
    } else if (fp_compare_epsilon<double>(freq) <= MAGNESIUM_MAX_FREQ) {
        band = tx_band::BAND3;
    } else {
        band = tx_band::INVALID_BAND;
    }

    return band;
}

