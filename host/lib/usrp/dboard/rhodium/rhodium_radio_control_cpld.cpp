//
// Copyright 2018 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "rhodium_constants.hpp"
#include "rhodium_cpld_ctrl.hpp"
#include "rhodium_radio_control.hpp"
#include <uhd/utils/log.hpp>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::rfnoc;

namespace {

const char* rx_band_to_log(rhodium_radio_control_impl::rx_band rx_band)
{
    switch (rx_band) {
        case rhodium_radio_control_impl::rx_band::RX_BAND_0:
            return "0";
        case rhodium_radio_control_impl::rx_band::RX_BAND_1:
            return "1";
        case rhodium_radio_control_impl::rx_band::RX_BAND_2:
            return "2";
        case rhodium_radio_control_impl::rx_band::RX_BAND_3:
            return "3";
        case rhodium_radio_control_impl::rx_band::RX_BAND_4:
            return "4";
        case rhodium_radio_control_impl::rx_band::RX_BAND_5:
            return "5";
        case rhodium_radio_control_impl::rx_band::RX_BAND_6:
            return "6";
        case rhodium_radio_control_impl::rx_band::RX_BAND_7:
            return "7";
        case rhodium_radio_control_impl::rx_band::RX_BAND_INVALID:
            return "INVALID";
        default:
            UHD_THROW_INVALID_CODE_PATH();
    }
}

const char* tx_band_to_log(rhodium_radio_control_impl::tx_band tx_band)
{
    switch (tx_band) {
        case rhodium_radio_control_impl::tx_band::TX_BAND_0:
            return "0";
        case rhodium_radio_control_impl::tx_band::TX_BAND_1:
            return "1";
        case rhodium_radio_control_impl::tx_band::TX_BAND_2:
            return "2";
        case rhodium_radio_control_impl::tx_band::TX_BAND_3:
            return "3";
        case rhodium_radio_control_impl::tx_band::TX_BAND_4:
            return "4";
        case rhodium_radio_control_impl::tx_band::TX_BAND_5:
            return "5";
        case rhodium_radio_control_impl::tx_band::TX_BAND_6:
            return "6";
        case rhodium_radio_control_impl::tx_band::TX_BAND_7:
            return "7";
        case rhodium_radio_control_impl::tx_band::TX_BAND_INVALID:
            return "INVALID";
        default:
            UHD_THROW_INVALID_CODE_PATH();
    }
}
} // namespace

void rhodium_radio_control_impl::_update_rx_freq_switches(const double freq)
{
    RFNOC_LOG_TRACE("Update all RX freq related switches. f=" << freq << " Hz");
    const auto band = _map_freq_to_rx_band(freq);
    const auto UHD_UNUSED(log_band) = rx_band_to_log(band);
    RFNOC_LOG_TRACE("Selected band " << log_band);

    // select values for lowband/highband switches
    const bool is_lowband = (band == rx_band::RX_BAND_0);
    auto rx_sw2_sw7       = is_lowband ? rhodium_cpld_ctrl::RX_SW2_SW7_LOWBANDFILTERBANK
                                 : rhodium_cpld_ctrl::RX_SW2_SW7_HIGHBANDFILTERBANK;
    auto rx_hb_lb_sel = is_lowband ? rhodium_cpld_ctrl::RX_HB_LB_SEL_LOWBAND
                                   : rhodium_cpld_ctrl::RX_HB_LB_SEL_HIGHBAND;

    // select values for filter bank switches
    rhodium_cpld_ctrl::rx_sw3_t rx_sw3;
    rhodium_cpld_ctrl::rx_sw4_sw5_t rx_sw4_sw5;
    rhodium_cpld_ctrl::rx_sw6_t rx_sw6;
    switch (band) {
        case rx_band::RX_BAND_0:
            // Low band doesn't use the filter banks, use configuration for band 1
        case rx_band::RX_BAND_1:
            rx_sw3     = rhodium_cpld_ctrl::RX_SW3_TOSWITCH4;
            rx_sw4_sw5 = rhodium_cpld_ctrl::RX_SW4_SW5_FILTER0450X0760MHZ;
            rx_sw6     = rhodium_cpld_ctrl::RX_SW6_FROMSWITCH5;
            break;
        case rx_band::RX_BAND_2:
            rx_sw3     = rhodium_cpld_ctrl::RX_SW3_TOSWITCH4;
            rx_sw4_sw5 = rhodium_cpld_ctrl::RX_SW4_SW5_FILTER0760X1100MHZ;
            rx_sw6     = rhodium_cpld_ctrl::RX_SW6_FROMSWITCH5;
            break;
        case rx_band::RX_BAND_3:
            rx_sw3     = rhodium_cpld_ctrl::RX_SW3_TOSWITCH4;
            rx_sw4_sw5 = rhodium_cpld_ctrl::RX_SW4_SW5_FILTER1100X1410MHZ;
            rx_sw6     = rhodium_cpld_ctrl::RX_SW6_FROMSWITCH5;
            break;
        case rx_band::RX_BAND_4:
            rx_sw3     = rhodium_cpld_ctrl::RX_SW3_TOSWITCH4;
            rx_sw4_sw5 = rhodium_cpld_ctrl::RX_SW4_SW5_FILTER1410X2050MHZ;
            rx_sw6     = rhodium_cpld_ctrl::RX_SW6_FROMSWITCH5;
            break;
        case rx_band::RX_BAND_5:
            rx_sw3     = rhodium_cpld_ctrl::RX_SW3_TOFILTER2050X3000MHZ;
            rx_sw4_sw5 = rhodium_cpld_ctrl::RX_SW4_SW5_FILTER0450X0760MHZ;
            rx_sw6     = rhodium_cpld_ctrl::RX_SW6_FROMFILTER2050X3000MHZ;
            break;
        case rx_band::RX_BAND_6:
            rx_sw3     = rhodium_cpld_ctrl::RX_SW3_TOFILTER3000X4500MHZ;
            rx_sw4_sw5 = rhodium_cpld_ctrl::RX_SW4_SW5_FILTER0450X0760MHZ;
            rx_sw6     = rhodium_cpld_ctrl::RX_SW6_FROMFILTER3000X4500MHZ;
            break;
        case rx_band::RX_BAND_7:
            rx_sw3     = rhodium_cpld_ctrl::RX_SW3_TOFILTER4500X6000MHZ;
            rx_sw4_sw5 = rhodium_cpld_ctrl::RX_SW4_SW5_FILTER0450X0760MHZ;
            rx_sw6     = rhodium_cpld_ctrl::RX_SW6_FROMFILTER4500X6000MHZ;
            break;
        case rx_band::RX_BAND_INVALID:
            throw uhd::runtime_error(
                str(boost::format("Cannot map RX frequency to band: %f") % freq));
        default:
            UHD_THROW_INVALID_CODE_PATH();
    }

    // commit settings to cpld
    _cpld->set_rx_switches(rx_sw2_sw7, rx_sw3, rx_sw4_sw5, rx_sw6, rx_hb_lb_sel);
}

void rhodium_radio_control_impl::_update_tx_freq_switches(const double freq)
{
    RFNOC_LOG_TRACE("Update all TX freq related switches. f=" << freq << " Hz");
    const auto band = _map_freq_to_tx_band(freq);
    const auto UHD_UNUSED(log_band) = tx_band_to_log(band);
    RFNOC_LOG_TRACE("Selected band " << log_band);

    // select values for lowband/highband switches
    const bool is_lowband = (band == tx_band::TX_BAND_0);
    auto tx_hb_lb_sel     = is_lowband ? rhodium_cpld_ctrl::TX_HB_LB_SEL_LOWBAND
                                   : rhodium_cpld_ctrl::TX_HB_LB_SEL_HIGHBAND;

    // select values for filter bank switches
    rhodium_cpld_ctrl::tx_sw2_t tx_sw2;
    rhodium_cpld_ctrl::tx_sw3_sw4_t tx_sw3_sw4;
    rhodium_cpld_ctrl::tx_sw5_t tx_sw5;
    switch (band) {
        case tx_band::TX_BAND_0:
            // Low band doesn't use the filter banks, use configuration for band 1
        case tx_band::TX_BAND_1:
            tx_sw2     = rhodium_cpld_ctrl::TX_SW2_FROMSWITCH3;
            tx_sw3_sw4 = rhodium_cpld_ctrl::TX_SW3_SW4_FROMTXFILTERLP0650MHZ;
            tx_sw5     = rhodium_cpld_ctrl::TX_SW5_TOSWITCH4;
            break;
        case tx_band::TX_BAND_2:
            tx_sw2     = rhodium_cpld_ctrl::TX_SW2_FROMSWITCH3;
            tx_sw3_sw4 = rhodium_cpld_ctrl::TX_SW3_SW4_FROMTXFILTERLP1000MHZ;
            tx_sw5     = rhodium_cpld_ctrl::TX_SW5_TOSWITCH4;
            break;
        case tx_band::TX_BAND_3:
            tx_sw2     = rhodium_cpld_ctrl::TX_SW2_FROMSWITCH3;
            tx_sw3_sw4 = rhodium_cpld_ctrl::TX_SW3_SW4_FROMTXFILTERLP1350MHZ;
            tx_sw5     = rhodium_cpld_ctrl::TX_SW5_TOSWITCH4;
            break;
        case tx_band::TX_BAND_4:
            tx_sw2     = rhodium_cpld_ctrl::TX_SW2_FROMSWITCH3;
            tx_sw3_sw4 = rhodium_cpld_ctrl::TX_SW3_SW4_FROMTXFILTERLP1900MHZ;
            tx_sw5     = rhodium_cpld_ctrl::TX_SW5_TOSWITCH4;
            break;
        case tx_band::TX_BAND_5:
            tx_sw2     = rhodium_cpld_ctrl::TX_SW2_FROMTXFILTERLP3000MHZ;
            tx_sw3_sw4 = rhodium_cpld_ctrl::TX_SW3_SW4_FROMTXFILTERLP0650MHZ;
            tx_sw5     = rhodium_cpld_ctrl::TX_SW5_TOTXFILTERLP3000MHZ;
            break;
        case tx_band::TX_BAND_6:
            tx_sw2     = rhodium_cpld_ctrl::TX_SW2_FROMTXFILTERLP4100MHZ;
            tx_sw3_sw4 = rhodium_cpld_ctrl::TX_SW3_SW4_FROMTXFILTERLP0650MHZ;
            tx_sw5     = rhodium_cpld_ctrl::TX_SW5_TOTXFILTERLP4100MHZ;
            break;
        case tx_band::TX_BAND_7:
            tx_sw2     = rhodium_cpld_ctrl::TX_SW2_FROMTXFILTERLP6000MHZ;
            tx_sw3_sw4 = rhodium_cpld_ctrl::TX_SW3_SW4_FROMTXFILTERLP0650MHZ;
            tx_sw5     = rhodium_cpld_ctrl::TX_SW5_TOTXFILTERLP6000MHZ;
            break;
        case tx_band::TX_BAND_INVALID:
            throw uhd::runtime_error(
                str(boost::format("Cannot map TX frequency to band: %f") % freq));
        default:
            UHD_THROW_INVALID_CODE_PATH();
    }

    // commit settings to cpld
    _cpld->set_tx_switches(tx_sw2, tx_sw3_sw4, tx_sw5, tx_hb_lb_sel);
}

void rhodium_radio_control_impl::_update_rx_input_switches(const std::string& input)
{
    RFNOC_LOG_TRACE("Update all RX input related switches. input=" << input);
    const rhodium_cpld_ctrl::cal_iso_sw_t cal_iso =
        (input == "CAL") ? rhodium_cpld_ctrl::CAL_ISO_CALLOOPBACK
                         : rhodium_cpld_ctrl::CAL_ISO_ISOLATION;
    const rhodium_cpld_ctrl::rx_sw1_t sw1 = [input] {
        if (input == "TX/RX") {
            return rhodium_cpld_ctrl::RX_SW1_FROMTXRXINPUT;
        } else if (input == "RX2") {
            return rhodium_cpld_ctrl::RX_SW1_FROMRX2INPUT;
        } else if (input == "CAL") {
            return rhodium_cpld_ctrl::RX_SW1_FROMCALLOOPBACK;
        } else if (input == "TERM") {
            return rhodium_cpld_ctrl::RX_SW1_ISOLATION;
        } else {
            throw uhd::runtime_error(
                "Invalid antenna in _update_rx_input_switches: " + input);
        }
    }();

    RFNOC_LOG_TRACE("Selected switch values:"
                    " sw1="
                    << sw1 << " cal_iso=" << cal_iso);
    _cpld->set_rx_input_switches(sw1, cal_iso);
}

void rhodium_radio_control_impl::_update_tx_output_switches(const std::string& output)
{
    RFNOC_LOG_TRACE("Update all TX output related switches. output=" << output);
    rhodium_cpld_ctrl::tx_sw1_t sw1;

    if (output == "TX/RX") {
        // SW1 needs to select low/high band
        if (_is_tx_lowband(get_tx_frequency(0))) {
            sw1 = rhodium_cpld_ctrl::TX_SW1_TOLOWBAND;
        } else {
            sw1 = rhodium_cpld_ctrl::TX_SW1_TOSWITCH2;
        }
    } else if (output == "CAL") {
        sw1 = rhodium_cpld_ctrl::TX_SW1_TOCALLOOPBACK;
    } else if (output == "TERM") {
        sw1 = rhodium_cpld_ctrl::TX_SW1_ISOLATION;
    } else {
        throw uhd::runtime_error(
            "Invalid antenna in _update_tx_output_switches: " + output);
    }

    RFNOC_LOG_TRACE("Selected switch values: sw1=" << sw1);
    _cpld->set_tx_output_switches(sw1);
}
