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
    double set_rate(double rate);

    void set_tx_antenna(const std::string &ant, const size_t chan);
    void set_rx_antenna(const std::string &ant, const size_t chan);

    double set_tx_frequency(const double freq, const size_t chan);
    double set_rx_frequency(const double freq, const size_t chan);
    double set_rx_bandwidth(const double bandwidth, const size_t chan);

    double set_tx_gain(const double gain, const size_t chan);
    double set_rx_gain(const double gain, const size_t chan);

    std::string get_tx_antenna(const size_t chan);
    std::string get_rx_antenna(const size_t chan);

    double get_tx_frequency(const size_t chan);
    double get_rx_frequency(const size_t chan);
    double get_rx_bandwidth(const size_t chan);

    double get_tx_gain(const size_t chan);
    double get_rx_gain(const size_t chan);

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
    //! Return the path to the dboards property subtree
    fs_path _get_fe_path(const size_t chan, const direction_t dir);

    //! Initialize all the peripherals connected to this block
    void _init_peripherals();

    //! Set state of this class to sensible defaults
    void _init_defaults();

    /**************************************************************************
     * AD9371 Controls
     *************************************************************************/
    double _myk_set_frequency(
            const double freq,
            const size_t chan,
            const direction_t dir
    );

    double _myk_set_gain(
            const double gain,
            const size_t chan,
            const direction_t dir
    );
    void _myk_set_antenna(
            const std::string &ant,
            const size_t chan,
            const direction_t dir
    );

    double _myk_set_bandwidth(
            const double bandwidth,
            const size_t chan,
            const direction_t dir
    );

    double _myk_get_frequency(
            const size_t chan,
            const direction_t dir
    );

    double _myk_get_gain(
            const size_t chan,
            const direction_t dir
    );

    std::string _myk_get_antenna(
            const size_t chan,
            const direction_t dir
    );

    double _myk_get_bandwidth(
            const size_t chan,
            const direction_t dir
    );

    double _lo_set_frequency(
            adf435x_iface::sptr lo_iface,
            const double freq,
            const size_t chan
    );

    void _update_freq_switches(
        const double freq,
        const size_t chan,
        const direction_t dir
    );

    void _update_atr_switches(
        const size_t chan,
        const direction_t dir,
        const std::string &ant
    );
    //! Letter representation of the radio we're currently running
    std::string _radio_slot;

    //! Stores the prefix to RPC calls
    std::string _rpc_prefix;

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

    //! ATR controls. These control the external DSA and the AD9371 gain
    //  up/down bits. They do *not* control the ATR state of the CPLD, the
    //  tx/rx run states are hooked up directly to the CPLD.
    //
    //  Every radio channel gets its own ATR state register.
    std::vector<usrp::gpio_atr::gpio_atr_3000::sptr> _gpio;

    //! Front panel GPIO controller. Note that only one radio block per
    //  module can be the FP-GPIO master.
    usrp::gpio_atr::gpio_atr_3000::sptr _fp_gpio;

}; /* class radio_ctrl_impl */

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_MAGNESIUM_RADIO_CTRL_IMPL_HPP */
// vim: sw=4 et:

