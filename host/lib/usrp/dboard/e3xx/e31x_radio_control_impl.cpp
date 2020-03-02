//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "e31x_radio_control_impl.hpp"
#include "e31x_regs.hpp"
#include <uhd/rfnoc/registry.hpp>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::rfnoc;

e31x_radio_control_impl::e31x_radio_control_impl(make_args_ptr make_args)
    : e3xx_radio_control_impl(std::move(make_args))
{
    // Swap front ends for E310
    _fe_swap = true;
    _init_mpm();
}

e31x_radio_control_impl::~e31x_radio_control_impl()
{
    RFNOC_LOG_TRACE("e31x_radio_control_impl::dtor()");
}

/******************************************************************************
 * API Calls
 *****************************************************************************/
uint32_t e31x_radio_control_impl::get_tx_switches(const size_t chan, const double freq)
{
    RFNOC_LOG_TRACE("Update all TX freq related switches. f=" << freq << " Hz, ");

    size_t fe_chan = _fe_swap ? (chan ? 0 : 1) : chan;

    auto tx_sw1    = TX_SW1_LB_2750; // SW1 = 0
    auto vctxrx_sw = VCTXRX_SW_OFF;
    auto tx_bias   = (fe_chan == 0) ? TX1_BIAS_LB_ON : TX2_BIAS_LB_ON;

    const auto band = e3xx_radio_control_impl::map_freq_to_tx_band(freq);
    switch (band) {
        case tx_band::LB_80:
            tx_sw1    = TX_SW1_LB_80;
            vctxrx_sw = (fe_chan == 0) ? VCTXRX1_SW_TX_LB : VCTXRX2_SW_TX_LB;
            break;
        case tx_band::LB_160:
            tx_sw1    = TX_SW1_LB_160;
            vctxrx_sw = (fe_chan == 0) ? VCTXRX1_SW_TX_LB : VCTXRX2_SW_TX_LB;
            break;
        case tx_band::LB_225:
            tx_sw1    = TX_SW1_LB_225;
            vctxrx_sw = (fe_chan == 0) ? VCTXRX1_SW_TX_LB : VCTXRX2_SW_TX_LB;
            break;
        case tx_band::LB_400:
            tx_sw1    = TX_SW1_LB_400;
            vctxrx_sw = (fe_chan == 0) ? VCTXRX1_SW_TX_LB : VCTXRX2_SW_TX_LB;
            break;
        case tx_band::LB_575:
            tx_sw1    = TX_SW1_LB_575;
            vctxrx_sw = (fe_chan == 0) ? VCTXRX1_SW_TX_LB : VCTXRX2_SW_TX_LB;
            break;
        case tx_band::LB_1000:
            tx_sw1    = TX_SW1_LB_1000;
            vctxrx_sw = (fe_chan == 0) ? VCTXRX1_SW_TX_LB : VCTXRX2_SW_TX_LB;
            break;
        case tx_band::LB_1700:
            tx_sw1    = TX_SW1_LB_1700;
            vctxrx_sw = (fe_chan == 0) ? VCTXRX1_SW_TX_LB : VCTXRX2_SW_TX_LB;
            break;
        case tx_band::LB_2750:
            tx_sw1    = TX_SW1_LB_2750;
            vctxrx_sw = (fe_chan == 0) ? VCTXRX1_SW_TX_LB : VCTXRX2_SW_TX_LB;
            break;
        case tx_band::HB:
            tx_sw1    = TX_SW1_LB_80;
            vctxrx_sw = VCTXRX_SW_TX_HB;
            tx_bias   = (fe_chan == 0) ? TX1_BIAS_HB_ON : TX2_BIAS_HB_ON;
            break;
        case tx_band::INVALID_BAND:
            RFNOC_LOG_ERROR("Cannot map TX frequency to band: " << freq);
            UHD_THROW_INVALID_CODE_PATH();
            break;
    }

    RFNOC_LOG_TRACE("TX band = " << int(band) << "TX SW1 = " << tx_sw1
                                 << "TX VCTXRX_SW = " << vctxrx_sw
                                 << "TX_BIAS = " << tx_bias);

    auto tx_regs = 0 | vctxrx_sw << VCTXRX_SW_SHIFT | tx_bias << TX_BIAS_SHIFT
                   | tx_sw1 << TX_SW1_SHIFT;
    return tx_regs;
}

uint32_t e31x_radio_control_impl::get_rx_switches(
    const size_t chan, const double freq, const std::string& ant)
{
    RFNOC_LOG_TRACE("Update all E310 RX freq related switches. f=" << freq << " Hz, ");

    size_t fe_chan = _fe_swap ? (chan ? 0 : 1) : chan;

    // Default to OFF
    auto rx_sw1    = RX_SW1_OFF;
    auto rx_swc    = RX_SWC_OFF;
    auto rx_swb    = RX_SWB_OFF;
    auto vctxrx_sw = VCTXRX_SW_OFF;
    auto vcrx_sw   = (ant == "TX/RX") ? VCRX_TXRX_SW_LB : VCRX_RX_SW_LB;
    if (ant == "TX/RX") {
        vctxrx_sw = (fe_chan == 0) ? VCTXRX1_SW_RX : VCTXRX2_SW_RX;
    }

    const auto band = e3xx_radio_control_impl::map_freq_to_rx_band(freq);

    switch (band) {
        case rx_band::LB_B2:
            rx_sw1 = (fe_chan == 0) ? RX1_SW1_LB_B2 : RX2_SW1_LB_B2;
            rx_swc = (fe_chan == 0) ? RX1_SWC_LB_B2 : RX2_SWC_LB_B2;
            rx_swb = RX_SWB_OFF;
            break;
        case rx_band::LB_B3:
            rx_sw1 = (fe_chan == 0) ? RX1_SW1_LB_B3 : RX2_SW1_LB_B3;
            rx_swc = (fe_chan == 0) ? RX1_SWC_LB_B3 : RX2_SWC_LB_B3;
            rx_swb = RX_SWB_OFF;
            break;
        case rx_band::LB_B4:
            rx_sw1 = (fe_chan == 0) ? RX1_SW1_LB_B4 : RX2_SW1_LB_B4;
            rx_swc = (fe_chan == 0) ? RX1_SWC_LB_B4 : RX2_SWC_LB_B4;
            rx_swb = RX_SWB_OFF;
            break;
        case rx_band::LB_B5:
            rx_sw1 = (fe_chan == 0) ? RX1_SW1_LB_B5 : RX2_SW1_LB_B5;
            rx_swc = RX_SWC_OFF;
            rx_swb = (fe_chan == 0) ? RX1_SWB_LB_B5 : RX2_SWB_LB_B5;
            break;
        case rx_band::LB_B6:
            rx_sw1 = (fe_chan == 0) ? RX1_SW1_LB_B6 : RX2_SW1_LB_B6;
            rx_swc = RX_SWC_OFF;
            rx_swb = (fe_chan == 0) ? RX1_SWB_LB_B6 : RX2_SWB_LB_B6;
            break;
        case rx_band::LB_B7:
            rx_sw1 = (fe_chan == 0) ? RX1_SW1_LB_B7 : RX2_SW1_LB_B7;
            rx_swc = RX_SWC_OFF;
            rx_swb = (fe_chan == 0) ? RX1_SWB_LB_B7 : RX2_SWB_LB_B7;
            break;
        case rx_band::HB:
            rx_sw1  = RX_SW1_OFF;
            rx_swc  = RX_SWC_OFF;
            rx_swb  = RX_SWB_OFF;
            vcrx_sw = (ant == "TX/RX") ? VCRX_TXRX_SW_HB : VCRX_RX_SW_HB;
            break;
        case rx_band::INVALID_BAND:
            RFNOC_LOG_ERROR("Cannot map RX frequency to band: " << freq);
            UHD_THROW_INVALID_CODE_PATH();
            break;
    }
    RFNOC_LOG_TRACE("RX SW1=" << rx_sw1 << " RX SWC=" << rx_swc << " RX SWB=" << rx_swb
                              << " RX VCRX_SW=" << vcrx_sw
                              << " RX VCTXRX_SW=" << vctxrx_sw);

    auto rx_regs = 0 | vcrx_sw << VCRX_SW_SHIFT | vctxrx_sw << VCTXRX_SW_SHIFT
                   | rx_swc << RX_SWC_SHIFT | rx_swb << RX_SWB_SHIFT
                   | rx_sw1 << RX_SW1_SHIFT;
    return rx_regs;
}

uint32_t e31x_radio_control_impl::get_idle_switches()
{
    uint32_t idle_regs = VCRX_SW_OFF << VCRX_SW_SHIFT | VCTXRX_SW_OFF << VCTXRX_SW_SHIFT
                         | TX_BIAS_OFF << TX_BIAS_SHIFT | RX_SWC_OFF << RX_SWC_SHIFT
                         | RX_SWB_OFF << RX_SWB_SHIFT | RX_SW1_OFF << RX_SW1_SHIFT
                         | TX_SW1_LB_2750 << TX_SW1_SHIFT;
    return idle_regs;
}

uint32_t e31x_radio_control_impl::get_idle_led()
{
    return 0;
}

uint32_t e31x_radio_control_impl::get_rx_led()
{
    return 1 << LED_RX_RX_SHIFT;
}

uint32_t e31x_radio_control_impl::get_tx_led()
{
    return 1 << LED_TXRX_TX_SHIFT;
}

uint32_t e31x_radio_control_impl::get_txrx_led()
{
    return 1 << LED_TXRX_RX_SHIFT;
}

UHD_RFNOC_BLOCK_REGISTER_FOR_DEVICE_DIRECT(
    e31x_radio_control, RADIO_BLOCK, E310, "Radio", true, "radio_clk", "bus_clk")
