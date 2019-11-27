//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_RFNOC_E31X_RADIO_CTRL_IMPL_HPP
#define INCLUDED_LIBUHD_RFNOC_E31X_RADIO_CTRL_IMPL_HPP

#include "e3xx_constants.hpp"
#include "e3xx_radio_ctrl_impl.hpp"

namespace uhd {
    namespace rfnoc {

/*! \brief Provide access to an E31X radio.
 */
class e31x_radio_ctrl_impl : public e3xx_radio_ctrl_impl
{
public:
    /************************************************************************
     * Structors
     ***********************************************************************/
    e31x_radio_ctrl_impl(
        const make_args_t &make_args
    );
    virtual ~e31x_radio_ctrl_impl();

protected:

    /**************************************************************************
     * ATR/ Switches Types
     *************************************************************************/

    enum tx_sw1_t {
        TX_SW1_LB_80 = 7,
        TX_SW1_LB_160 = 6,
        TX_SW1_LB_225 = 5,
        TX_SW1_LB_400 = 4,
        TX_SW1_LB_575 = 3,
        TX_SW1_LB_1000 = 2,
        TX_SW1_LB_1700 = 1,
        TX_SW1_LB_2750 = 0,
        TX_SW1_HB_5850 = 7
    };

    enum vctxrx_sw_t {
        VCTXRX_SW_TX_HB = 3,
        VCTXRX1_SW_TX_LB = 1,
        VCTXRX1_SW_RX = 2,
        VCTXRX2_SW_TX_LB = 2,
        VCTXRX2_SW_RX = 1,
        VCTXRX_SW_OFF = 0,
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
        RX_SW1_OFF = 7
    };

    enum rx_swc_t {
        RX2_SWC_LB_B2 = 1,
        RX2_SWC_LB_B3 = 3,
        RX2_SWC_LB_B4 = 2,
        RX1_SWC_LB_B2 = 2,
        RX1_SWC_LB_B3 = 3,
        RX1_SWC_LB_B4 = 1,
        RX_SWC_OFF = 0
    };

    enum rx_swb_t {
        RX2_SWB_LB_B5 = 1,
        RX2_SWB_LB_B6 = 3,
        RX2_SWB_LB_B7 = 2,
        RX1_SWB_LB_B5 = 2,
        RX1_SWB_LB_B6 = 3,
        RX1_SWB_LB_B7 = 1,
        RX_SWB_OFF = 0
    };

    enum vcrx_sw_t {
        VCRX_RX_SW_LB = 1,
        VCRX_RX_SW_HB = 2,
        VCRX_TXRX_SW_LB = 2,
        VCRX_TXRX_SW_HB = 1,
        VCRX_SW_OFF = 0 //or 3
    };

    // (TX_ENABLEB, TX_ENABLEA)
    enum tx_bias_t {
        TX1_BIAS_HB_ON = 1,
        TX1_BIAS_LB_ON = 2,
        TX2_BIAS_HB_ON = 1,
        TX2_BIAS_LB_ON = 2,
        TX_BIAS_OFF = 0
    };

    /************************************************************************
     * API calls
     ***********************************************************************/
    virtual bool check_radio_config();

    const std::string get_default_timing_mode()
    {
        return TIMING_MODE_1R1T;
    };

    /*! Run a loopback self test.
     *
     * This will write data to the AD936x and read it back again.
     * If this test fails, it generally means the interface is broken,
     * so we assume it passes and throw otherwise. Running this requires
     * a core that we can peek and poke the loopback values into.
     *
     * \param iface An interface to the associated radio control core
     * \param iface The radio control core's address to write the loopback value
     * \param iface The radio control core's readback address to read back the returned
     * value
     *
     * \throws a uhd::runtime_error if the loopback value didn't match.
     */
    void loopback_self_test(std::function<void(uint32_t)> poker_functor,
        std::function<uint64_t()> peeker_functor);

    uint32_t get_rx_switches(
        const size_t chan,
        const double freq,
        const std::string &ant
    );

    uint32_t get_tx_switches(
        const size_t chan,
        const double freq
    );

    uint32_t get_idle_switches();

    uint32_t get_tx_led();
    uint32_t get_rx_led();
    uint32_t get_txrx_led();
    uint32_t get_idle_led();
}; /* class radio_ctrl_impl */

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_E31X_RADIO_CTRL_IMPL_HPP */
// vim: sw=4 et:
