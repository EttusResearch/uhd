//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

//
// Driver for the N310/N300 daughterboard ("Magnesium")
//

#ifndef INCLUDED_LIBUHD_RFNOC_MAGNESIUM_RADIO_CTRL_IMPL_HPP
#define INCLUDED_LIBUHD_RFNOC_MAGNESIUM_RADIO_CTRL_IMPL_HPP

#include "magnesium_cpld_ctrl.hpp"
#include "magnesium_cpld_regs.hpp"
#include "magnesium_ad9371_iface.hpp"
#include <uhd/types/serial.hpp>
#include <uhd/usrp/dboard_manager.hpp>
#include <uhd/usrp/gpio_defs.hpp>
#include <uhdlib/usrp/cores/gpio_atr_3000.hpp>
#include <uhdlib/rfnoc/rpc_block_ctrl.hpp>
#include <uhdlib/rfnoc/radio_ctrl_impl.hpp>
#include <uhdlib/usrp/common/adf435x.hpp>
#include <mutex>

namespace uhd {
    namespace rfnoc {

/*! \brief RFNoC block / daughterboard driver for a "Magnesium" daughterboard.
 *
 * This daughterboard is used on the USRP N310 and N300.
 */
class magnesium_radio_ctrl_impl : public radio_ctrl_impl, public rpc_block_ctrl
{
public:
    typedef boost::shared_ptr<magnesium_radio_ctrl_impl> sptr;

    //! Frequency bands for RX. Bands are a function of the analog filter banks
    enum class rx_band {
        INVALID_BAND,
        LOWBAND,
        BAND0,
        BAND1,
        BAND2,
        BAND3,
        BAND4,
        BAND5,
        BAND6
    };

    //! Frequency bands for TX. Bands are a function of the analog filter banks
    enum class tx_band {
        INVALID_BAND,
        LOWBAND,
        BAND0,
        BAND1,
        BAND2,
        BAND3
    };

    /************************************************************************
     * Structors
     ***********************************************************************/
    UHD_RFNOC_RADIO_BLOCK_CONSTRUCTOR_DECL(magnesium_radio_ctrl)
    virtual ~magnesium_radio_ctrl_impl();

    /************************************************************************
     * API calls
     ***********************************************************************/
    // Note: We use the cached values in radio_ctrl_impl, so most getters are
    // not reimplemented here
    double set_rate(double rate);

    void set_tx_antenna(const std::string &ant, const size_t chan);
    void set_rx_antenna(const std::string &ant, const size_t chan);

    double set_tx_frequency(const double freq, const size_t chan);
    double set_rx_frequency(const double freq, const size_t chan);
    double get_rx_frequency(const size_t chan);
    double get_tx_frequency(const size_t chan);
    double set_tx_bandwidth(const double bandwidth, const size_t chan);
    double set_rx_bandwidth(const double bandwidth, const size_t chan);

    // RX LO
    std::vector<std::string> get_rx_lo_names(const size_t chan);
    std::vector<std::string> get_rx_lo_sources(
            const std::string &name,
            const size_t chan
    );
    freq_range_t get_rx_lo_freq_range(
            const std::string &name,
            const size_t chan
    );

    void set_rx_lo_source(
            const std::string &src,
            const std::string &name,
            const size_t chan
    );
    const std::string get_rx_lo_source(
            const std::string &name,
            const size_t chan
    );

    double set_rx_lo_freq(
            double freq,
            const std::string &name,
            const size_t chan
    );
    double get_rx_lo_freq(const std::string &name, const size_t chan);

    // TX LO
    std::vector<std::string> get_tx_lo_names(const size_t chan);
    std::vector<std::string> get_tx_lo_sources(
            const std::string &name,
            const size_t chan
    );
    freq_range_t get_tx_lo_freq_range(
            const std::string &name,
            const size_t chan
    );

    void set_tx_lo_source(
            const std::string &src,
            const std::string &name,
            const size_t chan
    );
    const std::string get_tx_lo_source(
            const std::string &name,
            const size_t chan
    );

    double set_tx_lo_freq(
            double freq,
            const std::string &name,
            const size_t chan
    );
    double get_tx_lo_freq(const std::string &name, const size_t chan);

    // gain
    double set_tx_gain(const double gain, const size_t chan);
    double set_rx_gain(const double gain, const size_t chan);
    void set_tx_gain_source(const std::string& src, const std::string& name, const size_t chan);
    std::string get_tx_gain_source(const std::string& name, const size_t chan);
    void set_rx_gain_source(const std::string& src, const std::string& name, const size_t chan);
    std::string get_rx_gain_source(const std::string& name, const size_t chan);

    size_t get_chan_from_dboard_fe(const std::string &fe, const direction_t dir);
    std::string get_dboard_fe_from_chan(const size_t chan, const direction_t dir);

    void set_rpc_client(
        uhd::rpc_client::sptr rpcc,
        const uhd::device_addr_t &block_args
    );

private:
    /**************************************************************************
     * Helpers
     *************************************************************************/
    //! Set tx gain on each gain element
    double _set_tx_gain(const std::string &name, const double gain, const size_t chan);

    //! Set rx gain on each gain element
    double _set_rx_gain(const std::string &name, const double gain, const size_t chan);

    //! Get tx gain on each gain element
    double _get_tx_gain(const std::string &name, const size_t chan);

    //! Get rx gain on each gain element
    double _get_rx_gain(const std::string &name, const size_t chan);

    //! Initialize all the peripherals connected to this block
    void _init_peripherals();

    //! Set state of this class to sensible defaults
    void _init_defaults();

    //! Init a subtree for the RF frontends
    void _init_frontend_subtree(
        uhd::property_tree::sptr subtree,
        const size_t chan_idx
    );

    //! Initialize property tree
    void _init_prop_tree();

    void _init_mpm_sensors(
        const direction_t dir,
        const size_t chan_idx
    );

    //! Map a frequency in Hz to an rx_band value. Will return
    //  rx_band::INVALID_BAND if the frequency is out of range.
    rx_band _map_freq_to_rx_band(const double freq);
    //! Map a frequency in Hz to an tx_band value. Will return
    //  tx_band::INVALID_BAND if the frequency is out of range.
    tx_band _map_freq_to_tx_band(const double freq);

    /**************************************************************************
     * Sensors
     *************************************************************************/
    //! Return LO lock status. Factors in current band (low/high) and
    // direction (TX/RX)
    bool get_lo_lock_status(
            const direction_t dir
    );

    /**************************************************************************
     * Gain Controls (implemented in magnesium_radio_ctrl_gain.cpp)
     *************************************************************************/
    //! Set the attenuation of the DSA
    double _dsa_set_att(
            const double att,
            const size_t chan,
            const direction_t dir
    );

    double _dsa_get_att(
            const size_t chan,
            const direction_t dir
    );

    //! Write the DSA word
    void _set_dsa_val(
            const size_t chan,
            const direction_t dir,
            const uint32_t dsa_val
    );


    double _set_all_gain(
        const double gain,
        const double freq,
        const size_t chan,
        const direction_t dir
    );

    double _get_all_gain(
        const size_t chan,
        const direction_t dir
    );

    void _update_gain(const size_t chan, direction_t dir);

    void _update_freq(
        const size_t chan,
        const uhd::direction_t dir
    );

    /**************************************************************************
     * CPLD Controls (implemented in magnesium_radio_ctrl_cpld.cpp)
     *************************************************************************/
    //! Blink the front-panel LEDs for \p identify_duration, then reset CPLD
    //  and resume normal operation.
    void _identify_with_leds(
        const int identify_duration
    );

    void _update_rx_freq_switches(
        const double freq,
        const bool bypass_lnas,
        const magnesium_cpld_ctrl::chan_sel_t chan_sel
    );

    void _update_tx_freq_switches(
        const double freq,
        const bool bypass_amps,
        const magnesium_cpld_ctrl::chan_sel_t chan_sel
    );

    void _update_atr_switches(
        const magnesium_cpld_ctrl::chan_sel_t chan,
        const direction_t dir,
        const std::string &ant
    );

    double _set_rx_lo_freq(
        const std::string source,
        const std::string name,
        const double freq,
        const size_t chan
    );

    double _set_tx_lo_freq(
        const std::string source,
        const std::string name,
        const double freq,
        const size_t chan
    );
    /**************************************************************************
     * Private attributes
     *************************************************************************/
    //! Locks access to setter APIs
    std::mutex _set_lock;

    //! Letter representation of the radio we're currently running
    std::string _radio_slot;

    //! Prepended for all dboard RPC calls
    std::string _rpc_prefix;

    //! If true, this is a master radio. This attribute will go away when we
    // move back to 1 radio block per dboard.
    bool _master;

    //! Additional block args; gets set during set_rpc_client()
    uhd::device_addr_t _block_args;

    //! Reference to the RPC client
    uhd::rpc_client::sptr _rpcc;

    //! Reference to the SPI core
    uhd::spi_iface::sptr _spi;

    //! Reference to the TX LO
    adf435x_iface::sptr _tx_lo;

    //! Reference to the RX LO
    adf435x_iface::sptr _rx_lo;

    //! Reference to the CPLD controls. Even if there's multiple radios,
    //  there's only one CPLD control.
    std::shared_ptr<magnesium_cpld_ctrl> _cpld;

    //! Reference to the AD9371 controls
    magnesium_ad9371_iface::uptr _ad9371;

    //! ATR controls. These control the external DSA and the AD9371 gain
    //  up/down bits. They do *not* control the ATR state of the CPLD, the
    //  tx/rx run states are hooked up directly to the CPLD.
    //
    //  Every radio channel gets its own ATR state register.
    std::vector<usrp::gpio_atr::gpio_atr_3000::sptr> _gpio;

    //! Front panel GPIO controller. Note that only one radio block per
    //  module can be the FP-GPIO master.
    usrp::gpio_atr::gpio_atr_3000::sptr _fp_gpio;

    //! Sampling rate, and also ref clock frequency for the lowband LOs.
    double _master_clock_rate = 1.0;
     //! Desired RF frequency
    std::map<direction_t,double> _desired_rf_freq = { {RX_DIRECTION, 2.44e9}, {TX_DIRECTION, 2.44e9} };
    //! Coerced adf4351 frequency
    //! Coerced ad9371 frequency
    std::map<direction_t,double> _ad9371_freq = { {RX_DIRECTION, 2.44e9}, {TX_DIRECTION, 2.44e9} };
    //! Coerced adf4351 frequency
    std::map<direction_t,double> _adf4351_freq = { {RX_DIRECTION, 2.44e9}, {TX_DIRECTION, 2.44e9} };
    //! Low band enable
    std::map<direction_t,bool> _is_low_band = { {RX_DIRECTION, false}, {TX_DIRECTION, false} };
    //! AD9371 gain
    double _ad9371_rx_gain = 0.0;
    double _ad9371_tx_gain = 0.0;
    std::map<direction_t,double> _ad9371_att = { {RX_DIRECTION, 0.0}, {TX_DIRECTION, 0.0} };
    //! DSA attenuation
    double _dsa_rx_att = 0.0;
    double _dsa_tx_att = 0.0;
    std::map<direction_t,double> _dsa_att = { {RX_DIRECTION, 0.0}, {TX_DIRECTION, 0.0} };
    //! amp gain
    std::map<direction_t,bool> _amp_bypass = { {RX_DIRECTION, true}, {TX_DIRECTION, true} };
    //! All gain
    double _all_rx_gain = 0.0;
    double _all_tx_gain = 0.0;
    //! Gain profile
    std::map<direction_t,std::string> _gain_profile = { {RX_DIRECTION, "default"}, {TX_DIRECTION, "default"} };

    bool _rx_bypass_lnas = true;
    bool _tx_bypass_amp  = true;

    //! TRX switch state of 2 channels
    std::map<magnesium_cpld_ctrl::chan_sel_t, magnesium_cpld_ctrl::sw_trx_t> _sw_trx = {
        {magnesium_cpld_ctrl::CHAN1,
            magnesium_cpld_ctrl::SW_TRX_FROMLOWERFILTERBANKTXSW1},
        {magnesium_cpld_ctrl::CHAN2,
            magnesium_cpld_ctrl::SW_TRX_FROMLOWERFILTERBANKTXSW1}
    };

    //! RX LO SOURCE
    // NOTE for magnesium only ad9371 LO that can be connected to the external LO so we only need one var here
    std::string _rx_lo_source = "internal";

}; /* class radio_ctrl_impl */

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_MAGNESIUM_RADIO_CTRL_IMPL_HPP */
// vim: sw=4 et:

