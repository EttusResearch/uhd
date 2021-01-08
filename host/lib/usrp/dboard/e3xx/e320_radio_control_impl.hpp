//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_RFNOC_E320_RADIO_CTRL_IMPL_HPP
#    define INCLUDED_LIBUHD_RFNOC_E320_RADIO_CTRL_IMPL_HPP

#    include "e3xx_constants.hpp"
#    include "e3xx_radio_control_impl.hpp"

namespace {
static constexpr char E320_GPIO_BANK[] = "FP0";
}

namespace uhd { namespace rfnoc {

/*! \brief Provide access to an E320 radio.
 *
 * This class only contains hardware-specific things that are different between
 * E320 and E31X.
 */
class e320_radio_control_impl : public e3xx_radio_control_impl
{
public:
    /************************************************************************
     * Structors
     ***********************************************************************/
    e320_radio_control_impl(make_args_ptr make_args);
    ~e320_radio_control_impl() override;

    std::vector<std::string> get_gpio_banks() const override
    {
        return {E320_GPIO_BANK};
    }

protected:
    /**************************************************************************
     * ATR/ Switches Types
     *************************************************************************/

    enum tx_sw1_t {
        TX_SW1_LB_80   = 3,
        TX_SW1_LB_160  = 7,
        TX_SW1_LB_225  = 1,
        TX_SW1_LB_400  = 5,
        TX_SW1_LB_575  = 2,
        TX_SW1_LB_1000 = 6,
        TX_SW1_LB_1700 = 0,
        TX_SW1_LB_2750 = 4
    };

    enum tx_sw2_t {
        TX_SW2_LB_80   = 7,
        TX_SW2_LB_160  = 3,
        TX_SW2_LB_225  = 5,
        TX_SW2_LB_400  = 1,
        TX_SW2_LB_575  = 6,
        TX_SW2_LB_1000 = 2,
        TX_SW2_LB_1700 = 4,
        TX_SW2_LB_2750 = 0
    };

    enum trx_sw_t {
        TRX1_SW_TX_HB = 2,
        TRX1_SW_TX_LB = 1,
        TRX1_SW_RX    = 4,
        TRX2_SW_TX_HB = 2,
        TRX2_SW_TX_LB = 4,
        TRX2_SW_RX    = 1
    };

    enum rx_sw1_t {
        RX_SW1_LB_B2 = 4,
        RX_SW1_LB_B3 = 5,
        RX_SW1_LB_B4 = 2,
        RX_SW1_LB_B5 = 3,
        RX_SW1_LB_B6 = 0,
        RX_SW1_LB_B7 = 1,
        RX_SW1_OFF   = 7

    };

    enum rx_sw2_t {
        RX_SW2_LB_B2 = 5,
        RX_SW2_LB_B3 = 4,
        RX_SW2_LB_B4 = 3,
        RX_SW2_LB_B5 = 2,
        RX_SW2_LB_B6 = 1,
        RX_SW2_LB_B7 = 0,
        RX_SW2_OFF   = 7
    };

    enum rx_sw3_t {
        RX_SW3_HBRX_LBTRX = 1,
        RX_SW3_HBTRX_LBRX = 2,
        RX_SW3_OFF        = 0 // or 3
    };

    enum tx_amp_t { TX_AMP_HF_ON = 2, TX_AMP_LF_ON = 1, TX_AMP_OFF = 3 };

    /************************************************************************
     * E3XX API calls
     ***********************************************************************/
    const std::string get_default_timing_mode() override
    {
        return TIMING_MODE_2R2T;
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

#endif /* INCLUDED_LIBUHD_RFNOC_E320_RADIO_CTRL_IMPL_HPP */
// vim: sw=4 et:
