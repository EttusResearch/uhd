//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_RFNOC_E320_RADIO_CTRL_IMPL_HPP
#    define INCLUDED_LIBUHD_RFNOC_E320_RADIO_CTRL_IMPL_HPP

#    include "e3xx_constants.hpp"
#    include "e3xx_radio_ctrl_impl.hpp"

namespace uhd { namespace rfnoc {

/*! \brief Provide access to an E320 radio.
 */
class e320_radio_ctrl_impl : public e3xx_radio_ctrl_impl
{
public:
    /************************************************************************
     * Structors
     ***********************************************************************/
    e320_radio_ctrl_impl(const make_args_t& make_args);
    virtual ~e320_radio_ctrl_impl();

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
     * API calls
     ***********************************************************************/
    virtual bool check_radio_config();

    const std::string get_default_timing_mode()
    {
        return TIMING_MODE_2R2T;
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
        const size_t chan, const double freq, const std::string& ant);

    uint32_t get_tx_switches(const size_t chan, const double freq);

    uint32_t get_idle_switches();

    uint32_t get_tx_led();
    uint32_t get_rx_led();
    uint32_t get_txrx_led();
    uint32_t get_idle_led();
}; /* class radio_ctrl_impl */

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_E320_RADIO_CTRL_IMPL_HPP */
// vim: sw=4 et:
