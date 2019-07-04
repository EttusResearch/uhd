//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "e320_radio_control_impl.hpp"
#include "e320_regs.hpp"
#include <uhd/rfnoc/registry.hpp>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::rfnoc;

e320_radio_control_impl::e320_radio_control_impl(make_args_ptr make_args)
    : e3xx_radio_control_impl(std::move(make_args))
{
    RFNOC_LOG_TRACE("e320_radio_control_impl::ctor()");
    // Don't swap front ends for E320
    _fe_swap = false;
    _init_mpm();
}

e320_radio_control_impl::~e320_radio_control_impl()
{
    RFNOC_LOG_TRACE("e320_radio_control_impl::dtor() ");
}

/******************************************************************************
 * E320 API Calls
 *****************************************************************************/
uint32_t e320_radio_control_impl::get_tx_switches(const size_t chan, const double freq)
{
    RFNOC_LOG_TRACE("Update all TX freq related switches. f=" << freq << " Hz, ");
    auto tx_sw1 = TX_SW1_LB_160;
    auto tx_sw2 = TX_SW2_LB_160;
    auto trx_sw = (chan == 0) ? TRX1_SW_TX_LB : TRX2_SW_TX_LB;
    auto tx_amp = TX_AMP_LF_ON;

    const auto band = e3xx_radio_control_impl::map_freq_to_tx_band(freq);
    switch (band) {
        case tx_band::LB_80:
            tx_sw1 = TX_SW1_LB_80;
            tx_sw2 = TX_SW2_LB_80;
            break;
        case tx_band::LB_160:
            tx_sw1 = TX_SW1_LB_160;
            tx_sw2 = TX_SW2_LB_160;
            break;
        case tx_band::LB_225:
            tx_sw1 = TX_SW1_LB_225;
            tx_sw2 = TX_SW2_LB_225;
            break;
        case tx_band::LB_400:
            tx_sw1 = TX_SW1_LB_400;
            tx_sw2 = TX_SW2_LB_400;
            break;
        case tx_band::LB_575:
            tx_sw1 = TX_SW1_LB_575;
            tx_sw2 = TX_SW2_LB_575;
            break;
        case tx_band::LB_1000:
            tx_sw1 = TX_SW1_LB_1000;
            tx_sw2 = TX_SW2_LB_1000;
            break;
        case tx_band::LB_1700:
            tx_sw1 = TX_SW1_LB_1700;
            tx_sw2 = TX_SW2_LB_1700;
            break;
        case tx_band::LB_2750:
            tx_sw1 = TX_SW1_LB_2750;
            tx_sw2 = TX_SW2_LB_2750;
            break;
        case tx_band::HB:
            trx_sw = (chan == 0) ? TRX1_SW_TX_HB : TRX2_SW_TX_HB;
            tx_amp = TX_AMP_HF_ON;
            break;
        case tx_band::INVALID_BAND:
            RFNOC_LOG_ERROR("Cannot map TX frequency to band: " << freq);
            UHD_THROW_INVALID_CODE_PATH();
            break;
    }

    auto tx_regs = tx_amp << TX_AMP_SHIFT | trx_sw << TRX_SW_SHIFT
                   | tx_sw2 << TX_SW2_SHIFT | tx_sw1 << TX_SW1_SHIFT;
    return tx_regs;
}

uint32_t e320_radio_control_impl::get_rx_switches(
    const size_t chan, const double freq, const std::string& ant)
{
    RFNOC_LOG_TRACE("Update all RX freq related switches. f=" << (freq / 1e6) << " MHz");
    // Default to OFF
    auto rx_sw1 = RX_SW1_OFF;
    auto rx_sw2 = RX_SW2_OFF;
    auto rx_sw3 = RX_SW3_OFF;
    auto trx_sw = (chan == 0) ? TRX1_SW_RX : TRX2_SW_RX;
    if (ant == "TX/RX") {
        rx_sw3 = RX_SW3_HBRX_LBTRX;
        trx_sw = (chan == 0) ? TRX1_SW_RX : TRX2_SW_RX;
    } else if (ant == "RX2") {
        rx_sw3 = RX_SW3_HBTRX_LBRX;
        // Set TRX switch to TX when receiving on RX2
        trx_sw = TRX1_SW_TX_HB;
    }

    const auto band = e3xx_radio_control_impl::map_freq_to_rx_band(freq);
    switch (band) {
        case rx_band::LB_B2:
            rx_sw1 = RX_SW1_LB_B2;
            rx_sw2 = RX_SW2_LB_B2;
            break;
        case rx_band::LB_B3:
            rx_sw1 = RX_SW1_LB_B3;
            rx_sw2 = RX_SW2_LB_B3;
            break;
        case rx_band::LB_B4:
            rx_sw1 = RX_SW1_LB_B4;
            rx_sw2 = RX_SW2_LB_B4;
            break;
        case rx_band::LB_B5:
            rx_sw1 = RX_SW1_LB_B5;
            rx_sw2 = RX_SW2_LB_B5;
            break;
        case rx_band::LB_B6:
            rx_sw1 = RX_SW1_LB_B6;
            rx_sw2 = RX_SW2_LB_B6;
            break;
        case rx_band::LB_B7:
            rx_sw1 = RX_SW1_LB_B7;
            rx_sw2 = RX_SW2_LB_B7;
            break;
        case rx_band::HB:
            rx_sw1 = RX_SW1_OFF;
            rx_sw2 = RX_SW2_OFF;
            if (ant == "TX/RX") {
                rx_sw3 = RX_SW3_HBTRX_LBRX;
            } else if (ant == "RX2") {
                rx_sw3 = RX_SW3_HBRX_LBTRX;
            }
            break;
        case rx_band::INVALID_BAND:
            RFNOC_LOG_ERROR("Cannot map RX frequency to band: " << freq);
            UHD_THROW_INVALID_CODE_PATH();
            break;
    }

    auto rx_regs = trx_sw << TRX_SW_SHIFT | rx_sw3 << RX_SW3_SHIFT
                   | rx_sw2 << RX_SW2_SHIFT | rx_sw1 << RX_SW1_SHIFT;
    return rx_regs;
}

uint32_t e320_radio_control_impl::get_idle_switches()
{
    uint32_t idle_regs = TX_AMP_OFF << TX_AMP_SHIFT | TRX1_SW_TX_HB << TRX_SW_SHIFT
                         | TX_SW2_LB_80 << TX_SW2_SHIFT | TX_SW1_LB_80 << TX_SW1_SHIFT
                         | RX_SW3_OFF << RX_SW3_SHIFT | RX_SW2_OFF << RX_SW2_SHIFT
                         | RX_SW1_OFF << RX_SW1_SHIFT;
    return idle_regs;
}

uint32_t e320_radio_control_impl::get_idle_led()
{
    return 0;
}

uint32_t e320_radio_control_impl::get_rx_led()
{
    return 1 << TRX_LED_GRN_SHIFT;
}

uint32_t e320_radio_control_impl::get_tx_led()
{
    return 1 << TX_LED_RED_SHIFT;
}

uint32_t e320_radio_control_impl::get_txrx_led()
{
    return 1 << RX_LED_GRN_SHIFT;
}

UHD_RFNOC_BLOCK_REGISTER_FOR_DEVICE_DIRECT(
    e320_radio_control, RADIO_BLOCK, E320, "Radio", true, "radio_clk", "bus_clk")
