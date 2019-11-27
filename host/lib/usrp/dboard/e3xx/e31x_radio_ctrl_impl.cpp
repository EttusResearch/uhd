//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "e31x_radio_ctrl_impl.hpp"
#include "e31x_regs.hpp"

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::rfnoc;

e31x_radio_ctrl_impl::e31x_radio_ctrl_impl(
    const make_args_t &make_args
): block_ctrl_base(make_args)
{
    // Swap front ends for E310
    _fe_swap = true;
    // The E310 has INT0, not FP0, as the GPIO bank name
    _fp_gpio_bank_name = "INT0";
}

e31x_radio_ctrl_impl::~e31x_radio_ctrl_impl()
{
    UHD_LOG_TRACE(unique_id(), "e31x_radio_ctrl_impl::dtor() ");
}

/******************************************************************************
 * API Calls
 *****************************************************************************/
bool e31x_radio_ctrl_impl::check_radio_config()
{
    // mapping of frontend to radio perif index
    static const size_t FE0 = 1;
    static const size_t FE1 = 0;
    const size_t num_rx = _is_streamer_active(RX_DIRECTION, FE0) + _is_streamer_active(RX_DIRECTION, FE1);
    const size_t num_tx = _is_streamer_active(TX_DIRECTION, FE0) + _is_streamer_active(TX_DIRECTION, FE1);

    //setup the active chains in the codec
    if ((num_rx + num_tx) == 0) {
        // Ensure at least one RX chain is enabled so AD9361 outputs a sample clock
        this->set_streaming_mode(true, false, true, false);
    } else {
        this->set_streaming_mode(
                _is_streamer_active(TX_DIRECTION, FE0),
                _is_streamer_active(TX_DIRECTION, FE1),
                _is_streamer_active(RX_DIRECTION, FE0),
                _is_streamer_active(RX_DIRECTION, FE1)
        );
    }
    return true;
}

/*  loopback_self_test checks the integrity of the FPGA->AD936x->FPGA sample interface.
    The AD936x is put in loopback mode that sends the TX data unchanged to the RX side.
    A test value is written to the codec_idle register in the TX side of the radio.
    The readback register is then used to capture the values on the TX and RX sides
    simultaneously for comparison. It is a reasonably effective test for AC timing
    since I/Q Ch0/Ch1 alternate over the same wires. Note, however, that it uses
    whatever timing is configured at the time the test is called rather than select
    worst case conditions to stress the interface.
    Note: This currently only tests 2R2T mode
*/
void e31x_radio_ctrl_impl::loopback_self_test(
    std::function<void(uint32_t)> poker_functor, std::function<uint64_t()> peeker_functor)
{
    // Save current rate before running this test
    const double current_rate = this->get_rate();
    // Set 2R2T mode, stream on all channels
    this->set_streaming_mode(true, false, true, false);
    // Set maximum rate for 2R2T mode
    this->set_rate(30.72e6);
    // Put AD936x in loopback mode
    _ad9361->data_port_loopback(true);
    UHD_LOG_INFO(unique_id(), "Performing CODEC loopback test... ");
    size_t hash                     = size_t(time(NULL));
    constexpr size_t loopback_count = 100;

    // Allow some time for AD936x to enter loopback mode.
    // There is no clear statement in the documentation of how long it takes,
    // but UG-570 does say to "allow six ADC_CLK/64 clock cycles of flush time"
    // when leaving the TX or RX states.  That works out to ~75us at the
    // minimum clock rate of 5 MHz, which lines up with test results.
    // Sleeping 1ms is far more than enough.
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    for (size_t i = 0; i < loopback_count; i++) {
        // Create test word
        boost::hash_combine(hash, i);
        const uint32_t word32 = uint32_t(hash) & 0xfff0fff0;
        // const uint32_t word32 = 0xCA00C100;
        // Write test word to codec_idle idle register (on TX side)
        poker_functor(word32);

        // Read back values - TX is lower 32-bits and RX is upper 32-bits
        const uint64_t rb_word64 = peeker_functor();
        const uint32_t rb_tx     = uint32_t(rb_word64 >> 32);
        const uint32_t rb_rx     = uint32_t(rb_word64 & 0xffffffff);

        // Compare TX and RX values to test word
        bool test_fail = word32 != rb_tx or word32 != rb_rx;
        if (test_fail) {
            UHD_LOG_WARNING(unique_id(),
                "CODEC loopback test failed! "
                    << boost::format("Expected: 0x%08X Received (TX/RX): 0x%08X/0x%08X")
                           % word32 % rb_tx % rb_rx);
            throw uhd::runtime_error("CODEC loopback test failed.");
        }
    }
    UHD_LOG_INFO(unique_id(), "CODEC loopback test passed");

    // Zero out the idle data.
    poker_functor(0);

    // Take AD936x out of loopback mode
    _ad9361->data_port_loopback(false);
    this->set_streaming_mode(true, false, true, false);
    // Switch back to current rate
    this->set_rate(current_rate);
}


uint32_t e31x_radio_ctrl_impl::get_tx_switches(
    const size_t chan,
    const double freq
) {
    UHD_LOG_TRACE(unique_id(),
        "Update all TX freq related switches. f=" << freq << " Hz, "
    );

    size_t fe_chan = _fe_swap ? (chan ? 0 : 1): chan;

    auto tx_sw1 = TX_SW1_LB_2750;  // SW1 = 0
    auto vctxrx_sw = VCTXRX_SW_OFF;
    auto tx_bias = (fe_chan == 0) ? TX1_BIAS_LB_ON: TX2_BIAS_LB_ON;

    const auto band = e3xx_radio_ctrl_impl::map_freq_to_tx_band(freq);
    switch(band) {
    case tx_band::LB_80:
        tx_sw1 = TX_SW1_LB_80;
        vctxrx_sw = (fe_chan == 0) ? VCTXRX1_SW_TX_LB: VCTXRX2_SW_TX_LB;
        break;
    case tx_band::LB_160:
        tx_sw1 = TX_SW1_LB_160;
        vctxrx_sw = (fe_chan == 0) ? VCTXRX1_SW_TX_LB: VCTXRX2_SW_TX_LB;
        break;
    case tx_band::LB_225:
        tx_sw1 = TX_SW1_LB_225;
        vctxrx_sw = (fe_chan == 0) ? VCTXRX1_SW_TX_LB: VCTXRX2_SW_TX_LB;
        break;
    case tx_band::LB_400:
        tx_sw1 = TX_SW1_LB_400;
        vctxrx_sw = (fe_chan == 0) ? VCTXRX1_SW_TX_LB: VCTXRX2_SW_TX_LB;
        break;
    case tx_band::LB_575:
        tx_sw1 = TX_SW1_LB_575;
        vctxrx_sw = (fe_chan == 0) ? VCTXRX1_SW_TX_LB: VCTXRX2_SW_TX_LB;
        break;
    case tx_band::LB_1000:
        tx_sw1 = TX_SW1_LB_1000;
        vctxrx_sw = (fe_chan == 0) ? VCTXRX1_SW_TX_LB: VCTXRX2_SW_TX_LB;
        break;
    case tx_band::LB_1700:
        tx_sw1 = TX_SW1_LB_1700;
        vctxrx_sw = (fe_chan == 0) ? VCTXRX1_SW_TX_LB: VCTXRX2_SW_TX_LB;
        break;
    case tx_band::LB_2750:
        tx_sw1 = TX_SW1_LB_2750;
        vctxrx_sw = (fe_chan == 0) ? VCTXRX1_SW_TX_LB: VCTXRX2_SW_TX_LB;
        break;
    case tx_band::HB:
        tx_sw1 = TX_SW1_LB_80;
        vctxrx_sw = VCTXRX_SW_TX_HB;
        tx_bias = (fe_chan == 0) ? TX1_BIAS_HB_ON: TX2_BIAS_HB_ON;
        break;
    case tx_band::INVALID_BAND:
        UHD_LOG_ERROR(unique_id(),
            "Cannot map TX frequency to band: " << freq);
        UHD_THROW_INVALID_CODE_PATH();
        break;

    }

    UHD_LOG_TRACE(unique_id(),
        "TX band = " << int(band) << "TX SW1 = " << tx_sw1
                     << "TX VCTXRX_SW = " << vctxrx_sw
                     << "TX_BIAS = " << tx_bias);

    auto tx_regs = 0 |
             vctxrx_sw << VCTXRX_SW_SHIFT |
             tx_bias << TX_BIAS_SHIFT |
             tx_sw1 << TX_SW1_SHIFT;
    return tx_regs;
}

uint32_t e31x_radio_ctrl_impl::get_rx_switches(
    const size_t chan,
    const double freq,
    const std::string &ant
){
    UHD_LOG_TRACE(unique_id(),
        "Update all E310 RX freq related switches. f=" << freq << " Hz, "
    );

    size_t fe_chan = _fe_swap ? (chan ? 0 : 1): chan;

    // Default to OFF
    auto rx_sw1 = RX_SW1_OFF;
    auto rx_swc = RX_SWC_OFF;
    auto rx_swb = RX_SWB_OFF;
    auto vctxrx_sw = VCTXRX_SW_OFF;
    auto vcrx_sw = (ant == "TX/RX") ? VCRX_TXRX_SW_LB: VCRX_RX_SW_LB;
    if (ant == "TX/RX") {
        vctxrx_sw = (fe_chan == 0) ? VCTXRX1_SW_RX: VCTXRX2_SW_RX;
    }

    const auto band = e3xx_radio_ctrl_impl::map_freq_to_rx_band(freq);

    switch(band) {
    case rx_band::LB_B2:
        rx_sw1 = (fe_chan == 0) ? RX1_SW1_LB_B2: RX2_SW1_LB_B2;
        rx_swc = (fe_chan == 0) ? RX1_SWC_LB_B2: RX2_SWC_LB_B2;
        rx_swb = RX_SWB_OFF;
        break;
    case rx_band::LB_B3:
        rx_sw1 = (fe_chan == 0) ? RX1_SW1_LB_B3: RX2_SW1_LB_B3;
        rx_swc = (fe_chan == 0) ? RX1_SWC_LB_B3: RX2_SWC_LB_B3;
        rx_swb = RX_SWB_OFF;
        break;
    case rx_band::LB_B4:
        rx_sw1 = (fe_chan == 0) ? RX1_SW1_LB_B4: RX2_SW1_LB_B4;
        rx_swc = (fe_chan == 0) ? RX1_SWC_LB_B4: RX2_SWC_LB_B4;
        rx_swb = RX_SWB_OFF;
        break;
    case rx_band::LB_B5:
        rx_sw1 = (fe_chan == 0) ? RX1_SW1_LB_B5: RX2_SW1_LB_B5;
        rx_swc = RX_SWC_OFF;
        rx_swb = (fe_chan == 0) ? RX1_SWB_LB_B5: RX2_SWB_LB_B5;
        break;
    case rx_band::LB_B6:
        rx_sw1 = (fe_chan == 0) ? RX1_SW1_LB_B6: RX2_SW1_LB_B6;
        rx_swc = RX_SWC_OFF;
        rx_swb = (fe_chan == 0) ? RX1_SWB_LB_B6: RX2_SWB_LB_B6;
        break;
    case rx_band::LB_B7:
        rx_sw1 = (fe_chan == 0) ? RX1_SW1_LB_B7: RX2_SW1_LB_B7;
        rx_swc = RX_SWC_OFF;
        rx_swb = (fe_chan == 0) ? RX1_SWB_LB_B7: RX2_SWB_LB_B7;
        break;
    case rx_band::HB:
        rx_sw1 = RX_SW1_OFF;
        rx_swc = RX_SWC_OFF;
        rx_swb = RX_SWB_OFF;
        vcrx_sw = (ant == "TX/RX") ? VCRX_TXRX_SW_HB: VCRX_RX_SW_HB;
        break;
    case rx_band::INVALID_BAND:
        UHD_LOG_ERROR(unique_id(),
            "Cannot map RX frequency to band: " << freq);
        UHD_THROW_INVALID_CODE_PATH();
        break;
    }

    UHD_LOG_TRACE(unique_id(),
        "RX band = " << int(band) << "RX SW1 = " << rx_sw1 << "RX SWC = " << rx_swc
                     << "RX SWB = " << rx_swb << "RX VCRX_SW = " << vcrx_sw
                     << "RX VCTXRX_SW = " << vctxrx_sw);

    auto rx_regs = 0 |
             vcrx_sw << VCRX_SW_SHIFT |
             vctxrx_sw << VCTXRX_SW_SHIFT |
             rx_swc << RX_SWC_SHIFT |
             rx_swb << RX_SWB_SHIFT |
             rx_sw1 << RX_SW1_SHIFT;
    return rx_regs;
}

uint32_t e31x_radio_ctrl_impl::get_idle_switches()
{
    uint32_t idle_regs = VCRX_SW_OFF << VCRX_SW_SHIFT |
                        VCTXRX_SW_OFF << VCTXRX_SW_SHIFT |
                        TX_BIAS_OFF << TX_BIAS_SHIFT |
                        RX_SWC_OFF << RX_SWC_SHIFT |
                        RX_SWB_OFF << RX_SWB_SHIFT |
                        RX_SW1_OFF << RX_SW1_SHIFT |
                        TX_SW1_LB_2750 << TX_SW1_SHIFT;
    return idle_regs;
}

uint32_t e31x_radio_ctrl_impl::get_idle_led()
{
    return 0;
}

uint32_t e31x_radio_ctrl_impl::get_rx_led()
{
    return 1 << LED_RX_RX_SHIFT;
}

uint32_t e31x_radio_ctrl_impl::get_tx_led()
{
    return 1 << LED_TXRX_TX_SHIFT;
}

uint32_t e31x_radio_ctrl_impl::get_txrx_led()
{
    return 1 << LED_TXRX_RX_SHIFT;
}
UHD_RFNOC_BLOCK_REGISTER(e31x_radio_ctrl, "E31XRadio");
