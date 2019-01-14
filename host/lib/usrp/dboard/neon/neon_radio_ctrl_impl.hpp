//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_RFNOC_NEON_RADIO_CTRL_IMPL_HPP
#define INCLUDED_LIBUHD_RFNOC_NEON_RADIO_CTRL_IMPL_HPP

#include "neon_ad9361_iface.hpp"
#include <uhd/types/serial.hpp>
#include <uhd/usrp/dboard_manager.hpp>
#include <uhd/usrp/gpio_defs.hpp>
#include <uhdlib/rfnoc/radio_ctrl_impl.hpp>
#include <uhdlib/rfnoc/rpc_block_ctrl.hpp>
#include <uhdlib/usrp/cores/gpio_atr_3000.hpp>
#include <mutex>

namespace uhd { namespace rfnoc {

/*! \brief Provide access to an Magnesium radio.
 */
class neon_radio_ctrl_impl : public radio_ctrl_impl, public rpc_block_ctrl
{
public:
    typedef boost::shared_ptr<neon_radio_ctrl_impl> sptr;

    //! Frequency bands for RX. Bands are a function of the analog filter banks
    enum class rx_band { INVALID_BAND, LB_B2, LB_B3, LB_B4, LB_B5, LB_B6, LB_B7, HB };

    //! Frequency bands for TX. Bands are a function of the analog filter banks
    enum class tx_band {
        INVALID_BAND,
        LB_80,
        LB_160,
        LB_225,
        LB_400,
        LB_575,
        LB_1000,
        LB_1700,
        LB_2750,
        HB
    };

    /**************************************************************************
     * ATR/ Switches Types
     *************************************************************************/
    //! ATR state
    enum atr_state_t { IDLE, RX_ONLY, TX_ONLY, FULL_DUPLEX };

    //! Channel select:
    enum chan_sel_t { CHAN1, CHAN2, BOTH };

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
     * Structors
     ***********************************************************************/
    UHD_RFNOC_RADIO_BLOCK_CONSTRUCTOR_DECL(neon_radio_ctrl)
    virtual ~neon_radio_ctrl_impl();

    /************************************************************************
     * API calls
     ***********************************************************************/
    // Note: We use the cached values in radio_ctrl_impl, so most getters are
    // not reimplemented here

    virtual bool check_radio_config();

    //! Set which channel mode is used
    void set_channel_mode(const std::string& channel_mode);

    //! Set streaming mode - active chains, channel_mode, timing_mode
    void set_streaming_mode(
        const bool tx1, const bool tx2, const bool rx1, const bool rx2);

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

    double set_rate(const double rate);

    void set_tx_antenna(const std::string& ant, const size_t chan);
    void set_rx_antenna(const std::string& ant, const size_t chan);

    double set_tx_frequency(const double freq, const size_t chan);
    double set_rx_frequency(const double freq, const size_t chan);
    double set_tx_bandwidth(const double bandwidth, const size_t chan);
    double set_rx_bandwidth(const double bandwidth, const size_t chan);

    // gain
    double set_tx_gain(const double gain, const size_t chan);
    double set_rx_gain(const double gain, const size_t chan);

    size_t get_chan_from_dboard_fe(const std::string& fe, const direction_t dir);
    std::string get_dboard_fe_from_chan(const size_t chan, const direction_t dir);

    void set_rpc_client(uhd::rpc_client::sptr rpcc, const uhd::device_addr_t& block_args);

private:
    /**************************************************************************
     * Helpers
     *************************************************************************/
    //! Initialize all the peripherals connected to this block
    void _init_peripherals();

    //! Set state of this class to sensible defaults
    void _init_defaults();

    //! Init a subtree for the RF frontends
    void _init_frontend_subtree(uhd::property_tree::sptr subtree, const size_t chan_idx);

    //! Initialize Catalina defaults
    void _init_codec();

    //! Initialize property tree
    void _init_prop_tree();

    void _init_mpm_sensors(const direction_t dir, const size_t chan_idx);

    //! Map a frequency in Hz to an rx_band value. Will return
    //  rx_band::INVALID_BAND if the frequency is out of range.
    rx_band _map_freq_to_rx_band(const double freq);
    //! Map a frequency in Hz to an tx_band value. Will return
    //  tx_band::INVALID_BAND if the frequency is out of range.
    tx_band _map_freq_to_tx_band(const double freq);

    /*************************************************************************
     * Sensors
     *************************************************************************/
    //! Return LO lock status. Factors in current band (low/high) and
    // direction (TX/RX)
    bool get_lo_lock_status(const direction_t dir);

    /**************************************************************************
     * Misc Controls
     *************************************************************************/
    //! Blink the front-panel LEDs for \p identify_duration,
    //  and resume normal operation.
    void _identify_with_leds(const int identify_duration);

    uint32_t _get_rx_switches(
        const size_t chan, const double freq, const std::string& ant);

    uint32_t _get_tx_switches(const size_t chan, const double freq);

    void _set_atr_bits(const size_t chan);

    /**************************************************************************
     * Private attributes
     *************************************************************************/
    //! Locks access to setter APIs
    std::mutex _set_lock;

    //! Letter representation of the radio we're currently running
    std::string _radio_slot;

    //! Prepended for all dboard RPC calls
    std::string _rpc_prefix;

    //! Additional block args; gets set during set_rpc_client()
    uhd::device_addr_t _block_args;

    //! Reference to the RPC client
    uhd::rpc_client::sptr _rpcc;

    //! Reference to the SPI core
    uhd::spi_iface::sptr _spi;

    //! Reference to the AD9361 controls
    // neon_ad9361_iface::uptr _ad9361;
    ad9361_ctrl::sptr _ad9361;

    //! ATR controls. These control the AD9361 gain
    //  up/down bits.
    //  Every radio channel gets its own ATR state register.
    std::vector<usrp::gpio_atr::gpio_atr_3000::sptr> _db_gpio;

    // ATR controls for LEDs
    std::vector<usrp::gpio_atr::gpio_atr_3000::sptr> _leds_gpio;

    //! Front panel GPIO controller. Note that only one radio block per
    //  module can be the FP-GPIO master.
    usrp::gpio_atr::gpio_atr_3000::sptr _fp_gpio;

    //! Sampling rate
    double _master_clock_rate = 1.0;
}; /* class radio_ctrl_impl */

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_NEON_RADIO_CTRL_IMPL_HPP */
