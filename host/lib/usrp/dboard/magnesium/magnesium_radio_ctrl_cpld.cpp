//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "magnesium_radio_ctrl_impl.hpp"
#include "magnesium_cpld_ctrl.hpp"
#include "magnesium_constants.hpp"
#include <uhd/utils/log.hpp>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::rfnoc;

void magnesium_radio_ctrl_impl::_identify_with_leds(
    const int identify_duration
) {
    auto end_time = std::chrono::steady_clock::now()
                        + std::chrono::seconds(identify_duration);
    bool led_state = true;
    while (std::chrono::steady_clock::now() < end_time) {
        _cpld->set_tx_atr_bits(
            magnesium_cpld_ctrl::BOTH,
            magnesium_cpld_ctrl::ANY,
            led_state,
            false,
            false,
            true
        );
        _cpld->set_rx_input_atr_bits(
            magnesium_cpld_ctrl::BOTH,
            magnesium_cpld_ctrl::ANY,
            magnesium_cpld_ctrl::RX_SW1_TXRXINPUT, /* whatever */
            led_state,
            led_state
        );
        led_state = !led_state;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    _cpld->reset();
}

void magnesium_radio_ctrl_impl::_update_atr_switches(
    const magnesium_cpld_ctrl::chan_sel_t chan,
    const direction_t dir,
    const std::string &ant
){
    if (dir == RX_DIRECTION or dir == DX_DIRECTION) {
        // These default values work for RX2
        bool trx_led = false;
        bool rx2_led = true;
        auto rx_sw1 = magnesium_cpld_ctrl::RX_SW1_RX2INPUT;
        // The TRX switch in TX-idle mode defaults to TX-on mode. When TX is
        // off, and we're receiving on TX/RX however, we need to point TRX to
        // RX SW1. In all other cases, a TX state toggle (on to idle or  vice
        // versa) won't trigger a change of the TRX switch.
        auto sw_trx = _sw_trx[chan];
        UHD_LOG_TRACE(unique_id(),
            "Updating all RX-ATR related switches for antenna==" << ant);
        if (ant == "TX/RX") {
            rx_sw1 = magnesium_cpld_ctrl::RX_SW1_TRXSWITCHOUTPUT;
            sw_trx = magnesium_cpld_ctrl::SW_TRX_RXCHANNELPATH;
            trx_led = true;
            rx2_led = false;
        }
        else if (ant == "CAL") {
            // It makes intuitive sense to illuminate the green TX/RX LED when
            // receiving on CAL (because it goes over to the TX/RX port), but
            // the problem is that CAL is only useful when we're both TXing and
            // RXing, and then both green and red would be on the same LED.
            // So, for CAL, we light up the green RX2 LED.
            trx_led = false;
            rx2_led = true;
            rx_sw1 = magnesium_cpld_ctrl::RX_SW1_TXRXINPUT;
        }
        else if (ant == "LOCAL") {
            rx_sw1 = magnesium_cpld_ctrl::RX_SW1_RXLOCALINPUT;
        }
        _cpld->set_rx_input_atr_bits(
            chan,
            magnesium_cpld_ctrl::ON,
            rx_sw1,
            trx_led,
            rx2_led,
            true /* defer commit */
        );
        _cpld->set_rx_atr_bits(
            chan,
            magnesium_cpld_ctrl::ON,
            true,  /* amp on       */
            true,  /* mykonos on   */
            true   /* defer commit */
        );
        _cpld->set_rx_atr_bits(
            chan,
            magnesium_cpld_ctrl::IDLE,
            true,  /* amp stays on */
            true,  /* mykonos on   */
            true   /* defer commit */
        );
        _cpld->set_trx_sw_atr_bits(
            chan,
            magnesium_cpld_ctrl::IDLE, /* idle here means TX is off */
            sw_trx,
            false /* don't defer commit */
        );
    }
    if (dir == TX_DIRECTION or dir == DX_DIRECTION) {
        UHD_LOG_TRACE(unique_id(), "Updating all TX-ATR related switches...");
        _cpld->set_tx_atr_bits(
            chan,
            magnesium_cpld_ctrl::ON,
            true, /* LED on */
            true, /* PA on  */
            true, /* AMP on */
            true, /* Myk on */
            true  /* defer commit */
        );
        // Leaving PA on since we want shorter tx settling time.
        _cpld->set_tx_atr_bits(
            chan,
            magnesium_cpld_ctrl::IDLE,
            false, /* LED off */
            true, /* PA on  */
            true, /* AMP on */
            true,  /* Myk on  */
            false  /* don't defer commit */
        );
    };
}

void magnesium_radio_ctrl_impl::_update_rx_freq_switches(
    const double freq,
    const bool bypass_lnas,
    const magnesium_cpld_ctrl::chan_sel_t chan_sel
) {
    UHD_LOG_TRACE(unique_id(),
        "Update all RX freq related switches. f=" << freq << " Hz, "
        "bypass LNAS: " << (bypass_lnas ? "Yes" : "No") << ", chan=" << chan_sel
    );
    auto rx_sw2 = magnesium_cpld_ctrl::RX_SW2_BYPASSPATHTOSWITCH6;
    auto rx_sw3 = magnesium_cpld_ctrl::RX_SW3_SHUTDOWNSW3;
    auto rx_sw4 = magnesium_cpld_ctrl::RX_SW4_FILTER2100X2850MHZFROM;
    auto rx_sw5 = magnesium_cpld_ctrl::RX_SW5_FILTER1100X1575MHZFROM;
    auto rx_sw6 = magnesium_cpld_ctrl::RX_SW6_BYPASSPATHFROMSWITCH2;
    const auto band = _map_freq_to_rx_band(freq);
    const bool is_lowband = (band == rx_band::LOWBAND);
    const auto select_lowband_mixer_path = is_lowband ?
        magnesium_cpld_ctrl::LOWBAND_MIXER_PATH_SEL_LOBAND :
        magnesium_cpld_ctrl::LOWBAND_MIXER_PATH_SEL_BYPASS;
    const bool enable_lowband_mixer = is_lowband;
    const bool rx_lna1_enable =
        not bypass_lnas and (
                band == rx_band::BAND4 or
                band == rx_band::BAND5 or
                band == rx_band::BAND6);
    const bool rx_lna2_enable = not bypass_lnas and not rx_lna1_enable;
    UHD_LOG_TRACE(unique_id(),
        " Enabling LNA1: " << (rx_lna1_enable ? "Yes" : "No") <<
        " Enabling LNA2: " << (rx_lna2_enable ? "Yes" : "No"));
    // All the defaults are OK when using the bypass path.
    if (not bypass_lnas) {
        switch(band) {
        case rx_band::LOWBAND:
        case rx_band::BAND0:
            rx_sw2 = magnesium_cpld_ctrl::RX_SW2_LOWERFILTERBANKTOSWITCH3;
            rx_sw3 = magnesium_cpld_ctrl::RX_SW3_FILTER0490LPMHZ;
            rx_sw4 = magnesium_cpld_ctrl::RX_SW4_FILTER2700HPMHZ;
            rx_sw5 = magnesium_cpld_ctrl::RX_SW5_FILTER0490LPMHZFROM;
            rx_sw6 = magnesium_cpld_ctrl::RX_SW6_LOWERFILTERBANKFROMSWITCH5;
            break;
        case rx_band::BAND1:
            rx_sw2 = magnesium_cpld_ctrl::RX_SW2_LOWERFILTERBANKTOSWITCH3;
            rx_sw3 = magnesium_cpld_ctrl::RX_SW3_FILTER0440X0530MHZ;
            rx_sw4 = magnesium_cpld_ctrl::RX_SW4_FILTER2700HPMHZ;
            rx_sw5 = magnesium_cpld_ctrl::RX_SW5_FILTER0440X0530MHZFROM;
            rx_sw6 = magnesium_cpld_ctrl::RX_SW6_LOWERFILTERBANKFROMSWITCH5;
            break;
        case rx_band::BAND2:
            rx_sw2 = magnesium_cpld_ctrl::RX_SW2_LOWERFILTERBANKTOSWITCH3;
            rx_sw3 = magnesium_cpld_ctrl::RX_SW3_FILTER0650X1000MHZ;
            rx_sw4 = magnesium_cpld_ctrl::RX_SW4_FILTER2700HPMHZ;
            rx_sw5 = magnesium_cpld_ctrl::RX_SW5_FILTER0650X1000MHZFROM;
            rx_sw6 = magnesium_cpld_ctrl::RX_SW6_LOWERFILTERBANKFROMSWITCH5;
            break;
        case rx_band::BAND3:
            rx_sw2 = magnesium_cpld_ctrl::RX_SW2_LOWERFILTERBANKTOSWITCH3;
            rx_sw3 = magnesium_cpld_ctrl::RX_SW3_FILTER1100X1575MHZ;
            rx_sw4 = magnesium_cpld_ctrl::RX_SW4_FILTER2700HPMHZ;
            rx_sw5 = magnesium_cpld_ctrl::RX_SW5_FILTER1100X1575MHZFROM;
            rx_sw6 = magnesium_cpld_ctrl::RX_SW6_LOWERFILTERBANKFROMSWITCH5;
            break;
        case rx_band::BAND4:
            rx_sw2 = magnesium_cpld_ctrl::RX_SW2_LOWERFILTERBANKTOSWITCH3;
            rx_sw3 = magnesium_cpld_ctrl::RX_SW3_FILTER1600X2250MHZ;
            rx_sw4 = magnesium_cpld_ctrl::RX_SW4_FILTER1600X2250MHZFROM;
            rx_sw5 = magnesium_cpld_ctrl::RX_SW5_FILTER0440X0530MHZFROM;
            rx_sw6 = magnesium_cpld_ctrl::RX_SW6_UPPERFILTERBANKFROMSWITCH4;
            break;
        case rx_band::BAND5:
            rx_sw2 = magnesium_cpld_ctrl::RX_SW2_LOWERFILTERBANKTOSWITCH3;
            rx_sw3 = magnesium_cpld_ctrl::RX_SW3_FILTER2100X2850MHZ;
            rx_sw4 = magnesium_cpld_ctrl::RX_SW4_FILTER2100X2850MHZFROM;
            rx_sw5 = magnesium_cpld_ctrl::RX_SW5_FILTER0440X0530MHZFROM;
            rx_sw6 = magnesium_cpld_ctrl::RX_SW6_UPPERFILTERBANKFROMSWITCH4;
            break;
        case rx_band::BAND6:
            rx_sw2 = magnesium_cpld_ctrl::RX_SW2_UPPERFILTERBANKTOSWITCH4;
            rx_sw3 = magnesium_cpld_ctrl::RX_SW3_SHUTDOWNSW3;
            rx_sw4 = magnesium_cpld_ctrl::RX_SW4_FILTER2700HPMHZ;
            rx_sw5 = magnesium_cpld_ctrl::RX_SW5_FILTER0440X0530MHZFROM;
            rx_sw6 = magnesium_cpld_ctrl::RX_SW6_UPPERFILTERBANKFROMSWITCH4;
            break;
        case rx_band::INVALID_BAND:
            UHD_LOG_ERROR(unique_id(),
                "Cannot map RX frequency to band: " << freq);
            break;
        default:
            UHD_THROW_INVALID_CODE_PATH();
        }
    }

    _cpld->set_rx_lna_atr_bits(
        chan_sel,
        magnesium_cpld_ctrl::ANY,
        rx_lna1_enable,
        rx_lna2_enable,
        true /* defer commit */
    );
    _cpld->set_rx_switches(
        chan_sel,
        rx_sw2,
        rx_sw3,
        rx_sw4,
        rx_sw5,
        rx_sw6,
        select_lowband_mixer_path,
        enable_lowband_mixer
    );
}

void magnesium_radio_ctrl_impl::_update_tx_freq_switches(
    const double freq,
    const bool bypass_amp,
    const magnesium_cpld_ctrl::chan_sel_t chan_sel
){
    UHD_LOG_TRACE(unique_id(),
        "Update all TX freq related switches. f=" << freq << " Hz, "
        "bypass amp: " << (bypass_amp ? "Yes" : "No") << ", chan=" << chan_sel
    );
    auto tx_sw1 = magnesium_cpld_ctrl::TX_SW1_SHUTDOWNTXSW1;
    auto tx_sw2 = magnesium_cpld_ctrl::TX_SW2_TOTXFILTERLP6400MHZ;
    auto tx_sw3 = magnesium_cpld_ctrl::TX_SW3_BYPASSPATHTOTRXSW;
    const auto band = _map_freq_to_tx_band(freq);
    const bool is_lowband = (band == tx_band::LOWBAND);
    const auto select_lowband_mixer_path = is_lowband ?
        magnesium_cpld_ctrl::LOWBAND_MIXER_PATH_SEL_LOBAND :
        magnesium_cpld_ctrl::LOWBAND_MIXER_PATH_SEL_BYPASS;
    const bool enable_lowband_mixer = is_lowband;
    // Defaults are fine for bypassing the amp stage
    if (bypass_amp) {
        _sw_trx[chan_sel] = magnesium_cpld_ctrl::SW_TRX_BYPASSPATHTOTXSW3;
    } else {
         // Set filters based on frequency
        switch(band) {
        case tx_band::LOWBAND:
            _sw_trx[chan_sel] =
                magnesium_cpld_ctrl::SW_TRX_FROMLOWERFILTERBANKTXSW1;
            tx_sw1 = magnesium_cpld_ctrl::TX_SW1_FROMTXFILTERLP0800MHZ;
            tx_sw2 = magnesium_cpld_ctrl::TX_SW2_TOTXFILTERLP0800MHZ;
            tx_sw3 = magnesium_cpld_ctrl::TX_SW3_TOTXFILTERBANKS;
            break;
        case tx_band::BAND0:
            _sw_trx[chan_sel] =
                magnesium_cpld_ctrl::SW_TRX_FROMLOWERFILTERBANKTXSW1;
            tx_sw1 = magnesium_cpld_ctrl::TX_SW1_FROMTXFILTERLP0800MHZ;
            tx_sw2 = magnesium_cpld_ctrl::TX_SW2_TOTXFILTERLP0800MHZ;
            tx_sw3 = magnesium_cpld_ctrl::TX_SW3_TOTXFILTERBANKS;
            break;
        case tx_band::BAND1:
            _sw_trx[chan_sel] =
                magnesium_cpld_ctrl::SW_TRX_FROMLOWERFILTERBANKTXSW1;
            tx_sw1 = magnesium_cpld_ctrl::TX_SW1_FROMTXFILTERLP1700MHZ;
            tx_sw2 = magnesium_cpld_ctrl::TX_SW2_TOTXFILTERLP1700MHZ;
            tx_sw3 = magnesium_cpld_ctrl::TX_SW3_TOTXFILTERBANKS;
            break;
        case tx_band::BAND2:
            _sw_trx[chan_sel] =
                magnesium_cpld_ctrl::SW_TRX_FROMLOWERFILTERBANKTXSW1;
            tx_sw1 = magnesium_cpld_ctrl::TX_SW1_FROMTXFILTERLP3400MHZ;
            tx_sw2 = magnesium_cpld_ctrl::TX_SW2_TOTXFILTERLP3400MHZ;
            tx_sw3 = magnesium_cpld_ctrl::TX_SW3_TOTXFILTERBANKS;
            break;
        case tx_band::BAND3:
            _sw_trx[chan_sel] =
                magnesium_cpld_ctrl::SW_TRX_FROMTXUPPERFILTERBANKLP6400MHZ;
            tx_sw1 = magnesium_cpld_ctrl::TX_SW1_SHUTDOWNTXSW1;
            tx_sw2 = magnesium_cpld_ctrl::TX_SW2_TOTXFILTERLP6400MHZ;
            tx_sw3 = magnesium_cpld_ctrl::TX_SW3_TOTXFILTERBANKS;
            break;
        case tx_band::INVALID_BAND:
            UHD_LOG_ERROR(unique_id(),
                "Cannot map TX frequency to band: " << freq);
            break;
        default:
            UHD_THROW_INVALID_CODE_PATH();
        }
    }

    _cpld->set_trx_sw_atr_bits(
        chan_sel,
        magnesium_cpld_ctrl::ON,
        _sw_trx[chan_sel],
        true /* defer commit */
    );
    _cpld->set_tx_switches(
        chan_sel,
        tx_sw1,
        tx_sw2,
        tx_sw3,
        select_lowband_mixer_path,
        enable_lowband_mixer,
        magnesium_cpld_ctrl::ON
    );
}

