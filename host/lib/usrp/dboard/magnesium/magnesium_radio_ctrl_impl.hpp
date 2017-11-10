//
// Copyright 2017 Ettus Research
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef INCLUDED_LIBUHD_RFNOC_MAGNESIUM_RADIO_CTRL_IMPL_HPP
#define INCLUDED_LIBUHD_RFNOC_MAGNESIUM_RADIO_CTRL_IMPL_HPP

#include "radio_ctrl_impl.hpp"
#include "rpc_block_ctrl.hpp"
#include "magnesium_cpld_ctrl.hpp"
#include "magnesium_cpld_regs.hpp"
#include "magnesium_ad9371_iface.hpp"
#include "adf435x.hpp"
#include "gpio_atr_3000.hpp"
#include <uhd/types/serial.hpp>
#include <uhd/usrp/dboard_manager.hpp>
#include <uhd/usrp/gpio_defs.hpp>

namespace uhd {
    namespace rfnoc {

/*! \brief Provide access to an Magnesium radio.
 */
class magnesium_radio_ctrl_impl : public radio_ctrl_impl, public rpc_block_ctrl
{
public:
    typedef boost::shared_ptr<magnesium_radio_ctrl_impl> sptr;

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

    double set_tx_gain(const double gain, const size_t chan);
    double set_rx_gain(const double gain, const size_t chan);
    size_t get_chan_from_dboard_fe(const std::string &fe, const direction_t dir);
    std::string get_dboard_fe_from_chan(const size_t chan, const direction_t dir);

    double get_output_samp_rate(size_t port);

    void set_rpc_client(
        uhd::rpc_client::sptr rpcc,
        const uhd::device_addr_t &block_args
    );

private:
    /**************************************************************************
     * Helpers
     *************************************************************************/
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

    /**************************************************************************
     * CPLD Controls (implemented in magnesium_radio_ctrl_cpld.cpp)
     *************************************************************************/
    void _update_rx_freq_switches(
        const double freq,
        const bool bypass_lnas,
        const size_t chan
    );

    void _update_tx_freq_switches(
        const double freq,
        const bool bypass_amps,
        const size_t chan
    );

    void _update_atr_switches(
        const magnesium_cpld_ctrl::chan_sel_t chan,
        const direction_t dir,
        const std::string &ant
    );

    /**************************************************************************
     * Private attributes
     *************************************************************************/
    //! Letter representation of the radio we're currently running
    std::string _radio_slot;

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

    //! AD9371 gain
    double _ad9371_rx_gain = 0.0;
    double _ad9371_tx_gain = 0.0;

    //! DSA attenuation
    double _dsa_rx_att = 0.0;
    double _dsa_tx_att = 0.0;

    //! All gain
    double _all_rx_gain = 0.0;
    double _all_tx_gain = 0.0;

    bool _rx_bypass_lnas = true;
    bool _tx_bypass_amp  = true;

    //! TRX switch state of 2 channels
    std::map<magnesium_cpld_ctrl::chan_sel_t, magnesium_cpld_ctrl::sw_trx_t> _sw_trx = {
        {magnesium_cpld_ctrl::CHAN1,
            magnesium_cpld_ctrl::SW_TRX_FROMLOWERFILTERBANKTXSW1},
        {magnesium_cpld_ctrl::CHAN2,
            magnesium_cpld_ctrl::SW_TRX_FROMLOWERFILTERBANKTXSW1}
    };
}; /* class radio_ctrl_impl */

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_MAGNESIUM_RADIO_CTRL_IMPL_HPP */
// vim: sw=4 et:

