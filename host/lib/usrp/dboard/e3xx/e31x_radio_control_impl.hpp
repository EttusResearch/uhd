//
// Copyright 2018 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_RFNOC_E31X_RADIO_CTRL_IMPL_HPP
#    define INCLUDED_LIBUHD_RFNOC_E31X_RADIO_CTRL_IMPL_HPP

#    include "e3xx_constants.hpp"
#    include "e3xx_radio_control_impl.hpp"

namespace {
static constexpr char E31x_GPIO_BANK[] = "INT0";
}

namespace uhd { namespace rfnoc {

/*! Provide access to an E31X radio.
 *
 * This class only contains hardware-specific things that are different between
 * E31X and E320.
 */
class e31x_radio_control_impl : public e3xx_radio_control_impl
{
public:
    /************************************************************************
     * Structors and deinit
     ***********************************************************************/
    e31x_radio_control_impl(make_args_ptr make_args);
    ~e31x_radio_control_impl() override;

    std::vector<std::string> get_gpio_banks() const override
    {
        return {E31x_GPIO_BANK};
    }

private:
    /**************************************************************************
     * ATR/ Switches Types
     *************************************************************************/
    enum tx_sw1_t {
        TX_SW1_LB_80   = 7,
        TX_SW1_LB_160  = 6,
        TX_SW1_LB_225  = 5,
        TX_SW1_LB_400  = 4,
        TX_SW1_LB_575  = 3,
        TX_SW1_LB_1000 = 2,
        TX_SW1_LB_1700 = 1,
        TX_SW1_LB_2750 = 0,
        TX_SW1_HB_5850 = 7
    };

    enum vctxrx_sw_t {
        VCTXRX_SW_TX_HB  = 3,
        VCTXRX1_SW_TX_LB = 1,
        VCTXRX1_SW_RX    = 2,
        VCTXRX2_SW_TX_LB = 2,
        VCTXRX2_SW_RX    = 1,
        VCTXRX_SW_OFF    = 0,
    };

    enum rx_sw1_t {
        RX2_SW1_LB_B2 = 5,
        RX2_SW1_LB_B3 = 3,
        RX2_SW1_LB_B4 = 1,
        RX2_SW1_LB_B5 = 0,
        RX2_SW1_LB_B6 = 2,
        RX2_SW1_LB_B7 = 4,
        RX1_SW1_LB_B2 = 4,
        RX1_SW1_LB_B3 = 2,
        RX1_SW1_LB_B4 = 0,
        RX1_SW1_LB_B5 = 1,
        RX1_SW1_LB_B6 = 3,
        RX1_SW1_LB_B7 = 5,
        RX_SW1_OFF    = 7
    };

    enum rx_swc_t {
        RX2_SWC_LB_B2 = 1,
        RX2_SWC_LB_B3 = 3,
        RX2_SWC_LB_B4 = 2,
        RX1_SWC_LB_B2 = 2,
        RX1_SWC_LB_B3 = 3,
        RX1_SWC_LB_B4 = 1,
        RX_SWC_OFF    = 0
    };

    enum rx_swb_t {
        RX2_SWB_LB_B5 = 1,
        RX2_SWB_LB_B6 = 3,
        RX2_SWB_LB_B7 = 2,
        RX1_SWB_LB_B5 = 2,
        RX1_SWB_LB_B6 = 3,
        RX1_SWB_LB_B7 = 1,
        RX_SWB_OFF    = 0
    };

    enum vcrx_sw_t {
        VCRX_RX_SW_LB   = 1,
        VCRX_RX_SW_HB   = 2,
        VCRX_TXRX_SW_LB = 2,
        VCRX_TXRX_SW_HB = 1,
        VCRX_SW_OFF     = 0 // or 3
    };

    // (TX_ENABLEB, TX_ENABLEA)
    enum tx_bias_t {
        TX1_BIAS_HB_ON = 1,
        TX1_BIAS_LB_ON = 2,
        TX2_BIAS_HB_ON = 1,
        TX2_BIAS_LB_ON = 2,
        TX_BIAS_OFF    = 0
    };

    /************************************************************************
     * E3XX API calls
     ***********************************************************************/
    const std::string get_default_timing_mode() override
    {
        return TIMING_MODE_1R1T;
    };

    uint32_t get_rx_switches(
        const size_t chan, const double freq, const std::string& ant) override;

    uint32_t get_tx_switches(const size_t chan, const double freq) override;

    uint32_t get_idle_switches() override;

    uint32_t get_tx_led() override;
    uint32_t get_rx_led() override;
    uint32_t get_txrx_led() override;
    uint32_t get_idle_led() override;
};

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_E31X_RADIO_CTRL_IMPL_HPP */
// vim: sw=4 et:
