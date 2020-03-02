//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "e3xx_constants.hpp"
#include "e3xx_radio_control_impl.hpp"
#include <uhd/utils/math.hpp>

/*
 * E320 frequency bands:
 *
 * For RX:
 *     Band   Freq      BSEL-210 BSEL-543         TX/RX                RX2
 *                                         SEL_10   |   FE_SEL_210    SEL_10
 *                                                   chan 1 | chan 2
 *     -------------------------------------------------------------------------
 *     LB_B2: < 450     RF5 100  RF6 101    01       RF3 100  RF1 001   10
 *     LB_B3: 450-700   RF6 101  RF5 100    01       RF3 100  RF1 001   10
 *     LB_B4: 700-1200  RF3 010  RF4 011    01       RF3 100  RF1 001   10
 *     LB_B5: 1200-1800 RF4 011  RF3 010    01       RF3 100  RF1 001   10
 *     LB_B6: 1800-2350 RF1 000  RF2 001    01       RF3 100  RF1 001   10
 *     LB_B7: 2350-2600 RF2 001  RF1 000    01       RF3 100  RF1 001   10
 *     HB:    2600+     --- 111  --- 111    10       RF3 100  RF1 001   01
 *
 *     SEL_1 SEL_0
 *      1    0    ANT1(RX_HB)--TX/RX, ANT2(RX_LB)--RX
 *      0    1    ANT1(RX_HB)--RX, ANT2(RX_LB)--TX/RX
 *
 * For TX:
 *     Band-Freq              BSEL-210 BSEL-543       TX/RX
 *                                                 FE_SEL_210
 *                                               chan 1 | chan 2
 *     ------------------------------------------------------------
 *     LB_80   < 117.7        RF7 011  RF8 111   RF1 001   RF3 100
 *     LB_160  117.7-178.2    RF8 111  RF7 011   RF1 001   RF3 100
 *     LB_225  178.2-284.3    RF5 001  RF6 101   RF1 001   RF3 100
 *     LB_400  284.3-453.7    RF6 101  RF5 001   RF1 001   RF3 100
 *     LB_575  453.7-723.8    RF3 010  RF4 110   RF1 001   RF3 100
 *     LB_1000 723.8-1154.9   RF4 110  RF3 010   RF1 001   RF3 100
 *     LB_1700 1154.9-1842.6  RF1 000  RF2 100   RF1 001   RF3 100
 *     LB_2750 1842.6-2940.0  RF2 100  RF1 000   RF1 001   RF3 100
 *     HB_5850 > 2940.0       --- ---  --- ---   RF2 010   RF2 010
 *
 */

/*
 * E31x frequency bands:
 *
 * For RX: (chan here is fe_chan - swapped)
 *     Band   Freq         RX_BSEL-210       RXC_BSEL-10   RXB_BSEL-10         RX2 TX/RX
 *                                                                          VCRX_V1_V2
 VCTXRX_V1_V2
 *                         chan1 | chan2   chan1 | chan2  chan1 | chan2  RX ant | TXRX ant
 chan2 | chan1
 *
 ----------------------------------------------------------------------------------------------------
 *     LB_B2: < 450       RF5 100  RF6 101  J2 10   J1 01   -- 00  -- 00    01    10 J2 10
 J1 01
 *     LB_B3: 450-700     RF3 010  RF4 011  J3 11   J3 11   -- 00  -- 00    01    10 J2 10
 J1 01
 *     LB_B4: 700-1200    RF1 000  RF2 001  J1 01   J2 10   -- 00  -- 00    01    10 J2 10
 J1 01
 *     LB_B5: 1200-1800   RF2 001  RF1 000  -- 00   -- 00   J2 10  J1 01    01    10 J2 10
 J1 01
 *     LB_B6: 1800-2350   RF4 011  RF3 010  -- 00   -- 00   J3 11  J3 11    01    10 J2 10
 J1 01
 *     LB_B7: 2350-2600   RF6 101  RF5 100  -- 00   -- 00   J1 01  J2 10    01    10 J2 10
 J1 01
 *     HB:    2600+       --- 111  --- 111  -- 00   -- 00   -- 00  -- 00    10    01 J2 10
 J1 01
 *
 *
 * For TX:
 *     Band-Freq              TX_BSEL-210           TX/RX
                                                VCTXRX_V1_V2
 *                                             chan 1 | chan 2
 *     ------------------------------------------------------------
 *     LB_80   < 117.7        RF8 111          J1 01    J2 10
 *     LB_160  117.7-178.2    RF7 110          J1 01    J2 10
 *     LB_225  178.2-284.3    RF6 101          J1 01    J2 10
 *     LB_400  284.3-453.7    RF5 100          J1 01    J2 10
 *     LB_575  453.7-723.8    RF4 011          J1 01    J2 10
 *     LB_1000 723.8-1154.9   RF3 010          J1 01    J2 10
 *     LB_1700 1154.9-1842.6  RF2 001          J1 01    J2 10
 *     LB_2750 1842.6-2940.0  RF1 000          J1 01    J2 10
 *     HB_5850 > 2940.0       RF8 111          J3 11    J3 11
 *
 */

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::rfnoc;
using namespace uhd::math::fp_compare;

namespace {
/* Note on the RX filter bank:
 *
 * The RX path has 7 bands, which we call LB_B2, B3, .. HB same as
 * the schematic.
 *
 * The following constants define lower cutoff frequencies for each band.
 * LB_B2 does not have a lower cutoff frequency, it is implied by
 * AD9361_MIN_FREQ. E3XX_RX_BAND1_MIN_FREQ is the cutover frequency
 * for switching from LB_B2 to LB_B3, and so on.
 *
 * Bands 1-6 have both high- and low-pass filters (effectively band
 * passes). Frequencies need to be chosen to allow as much of the full
 * bandwidth through unattenuated.
 */
constexpr double E3XX_RX_LB_BAND3_MIN_FREQ = 450e6;
constexpr double E3XX_RX_LB_BAND4_MIN_FREQ = 700e6;
constexpr double E3XX_RX_LB_BAND5_MIN_FREQ = 1200e6;
constexpr double E3XX_RX_LB_BAND6_MIN_FREQ = 1800e6;
constexpr double E3XX_RX_LB_BAND7_MIN_FREQ = 2350e6;
constexpr double E3XX_RX_HB_MIN_FREQ       = 2600e6;

/* Note on the TX filter bank:
 *
 * The TX path has 9 bands, which we name according to the schematic.
 *
 * The following constants define lower cutoff frequencies for each band.
 * LB_80 does not have a lower cutoff frequency, it is implied by
 * AD9361_MIN_FREQ. E3XX_TX_LB_160_MIN_FREQ is the cutover frequency
 * for switching from LB_80 to LB_160, and so on.
 *
 * All filters on the TX filter bank are
 * low pass filters (no high pass filters).
 * Frequencies need to be chosen to allow as much of the full bandwidth
 * through unattenuated (so don't go all the way up to the cutoff frequency
 * of that filter).
 */
constexpr double E3XX_TX_LB_160_MIN_FREQ  = 117.7e6;
constexpr double E3XX_TX_LB_225_MIN_FREQ  = 178.2e6;
constexpr double E3XX_TX_LB_400_MIN_FREQ  = 284.3e6;
constexpr double E3XX_TX_LB_575_MIN_FREQ  = 453.7e6;
constexpr double E3XX_TX_LB_1000_MIN_FREQ = 723.8e6;
constexpr double E3XX_TX_LB_1700_MIN_FREQ = 1154.9e6;
constexpr double E3XX_TX_LB_2750_MIN_FREQ = 1842.6e6;
constexpr double E3XX_TX_HB_MIN_FREQ      = 2940.0e6;
} // namespace

e3xx_radio_control_impl::rx_band e3xx_radio_control_impl::map_freq_to_rx_band(
    const double freq)
{
    e3xx_radio_control_impl::rx_band band;

    if (fp_compare_epsilon<double>(freq) < AD9361_RX_MIN_FREQ) {
        band = rx_band::INVALID_BAND;
    } else if (fp_compare_epsilon<double>(freq) < E3XX_RX_LB_BAND3_MIN_FREQ) {
        band = rx_band::LB_B2;
    } else if (fp_compare_epsilon<double>(freq) < E3XX_RX_LB_BAND4_MIN_FREQ) {
        band = rx_band::LB_B3;
    } else if (fp_compare_epsilon<double>(freq) < E3XX_RX_LB_BAND5_MIN_FREQ) {
        band = rx_band::LB_B4;
    } else if (fp_compare_epsilon<double>(freq) < E3XX_RX_LB_BAND6_MIN_FREQ) {
        band = rx_band::LB_B5;
    } else if (fp_compare_epsilon<double>(freq) < E3XX_RX_LB_BAND7_MIN_FREQ) {
        band = rx_band::LB_B6;
    } else if (fp_compare_epsilon<double>(freq) < E3XX_RX_HB_MIN_FREQ) {
        band = rx_band::LB_B7;
    } else if (fp_compare_epsilon<double>(freq) <= AD9361_RX_MAX_FREQ) {
        band = rx_band::HB;
    } else {
        band = rx_band::INVALID_BAND;
    }

    return band;
}

e3xx_radio_control_impl::tx_band e3xx_radio_control_impl::map_freq_to_tx_band(
    const double freq)
{
    e3xx_radio_control_impl::tx_band band;

    if (fp_compare_epsilon<double>(freq) < AD9361_TX_MIN_FREQ) {
        band = tx_band::INVALID_BAND;
    } else if (fp_compare_epsilon<double>(freq) < E3XX_TX_LB_160_MIN_FREQ) {
        band = tx_band::LB_80;
    } else if (fp_compare_epsilon<double>(freq) < E3XX_TX_LB_225_MIN_FREQ) {
        band = tx_band::LB_160;
    } else if (fp_compare_epsilon<double>(freq) < E3XX_TX_LB_400_MIN_FREQ) {
        band = tx_band::LB_225;
    } else if (fp_compare_epsilon<double>(freq) < E3XX_TX_LB_575_MIN_FREQ) {
        band = tx_band::LB_400;
    } else if (fp_compare_epsilon<double>(freq) < E3XX_TX_LB_1000_MIN_FREQ) {
        band = tx_band::LB_575;
    } else if (fp_compare_epsilon<double>(freq) < E3XX_TX_LB_1700_MIN_FREQ) {
        band = tx_band::LB_1000;
    } else if (fp_compare_epsilon<double>(freq) < E3XX_TX_LB_2750_MIN_FREQ) {
        band = tx_band::LB_1700;
    } else if (fp_compare_epsilon<double>(freq) < E3XX_TX_HB_MIN_FREQ) {
        band = tx_band::LB_2750;
    } else if (fp_compare_epsilon<double>(freq) <= AD9361_TX_MAX_FREQ) {
        band = tx_band::HB;
    } else {
        band = tx_band::INVALID_BAND;
    }

    return band;
}
