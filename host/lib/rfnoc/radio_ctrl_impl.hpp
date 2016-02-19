//
// Copyright 2014-2015 Ettus Research LLC
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

#ifndef INCLUDED_LIBUHD_RFNOC_RADIO_CTRL_IMPL_HPP
#define INCLUDED_LIBUHD_RFNOC_RADIO_CTRL_IMPL_HPP

#include "rx_vita_core_3000.hpp"
#include "tx_vita_core_3000.hpp"
#include "time_core_3000.hpp"
#include "gpio_atr_3000.hpp"
#include <uhd/rfnoc/radio_ctrl.hpp>

//! Shorthand for radio block constructor
#define UHD_RFNOC_RADIO_BLOCK_CONSTRUCTOR_DECL(CLASS_NAME) \
    CLASS_NAME##_impl(const make_args_t &make_args);

#define UHD_RFNOC_RADIO_BLOCK_CONSTRUCTOR(CLASS_NAME) \
    CLASS_NAME##_impl::CLASS_NAME##_impl( \
        const make_args_t &make_args \
    ) : block_ctrl_base(make_args), radio_ctrl_impl()

namespace uhd {
    namespace rfnoc {

/*! \brief Provide access to a radio.
 *
 */
class radio_ctrl_impl : public radio_ctrl
{
public:
    /************************************************************************
     * Structors
     ***********************************************************************/
    radio_ctrl_impl();
    virtual ~radio_ctrl_impl() {};

    /************************************************************************
     * Public Radio API calls
     ***********************************************************************/
    virtual double get_rate() const;
    virtual std::string get_antenna(const size_t chan) /* const */;
    virtual double get_tx_frequency(const size_t) /* const */;
    virtual double get_rx_frequency(const size_t) /* const */;
    virtual double get_tx_gain(const size_t) /* const */;
    virtual double get_rx_gain(const size_t) /* const */;

    /***********************************************************************
     * Block control API calls
     **********************************************************************/
    void set_rx_streamer(bool active, const size_t port);
    void set_tx_streamer(bool active, const size_t port);

    void issue_stream_cmd(const uhd::stream_cmd_t &stream_cmd, const size_t port);
    void handle_overrun(boost::weak_ptr<uhd::rx_streamer> streamer, const size_t port);

    double get_input_samp_rate(size_t /* port */) { return get_rate(); };
    double get_output_samp_rate(size_t /* port */) { return get_rate(); };

protected: // TODO see what's protected and what's private
    void _register_loopback_self_test(size_t chan);

    /***********************************************************************
     * Block control API calls
     **********************************************************************/
    double _get_tick_rate() { return get_rate(); };

    void _update_spp(int spp);

    virtual bool check_radio_config() { return true; };

    /************************************************************************
     * Peripherals
     ***********************************************************************/
    //! There is always only one time core per radio
    time_core_3000::sptr         _time64;
    //! Stores pointers to all streaming-related radio cores
    struct radio_perifs_t
    {
        rx_vita_core_3000::sptr  framer;
        tx_vita_core_3000::sptr  deframer;
        uhd::usrp::gpio_atr::gpio_atr_3000::sptr leds;
    };
    std::map<size_t, radio_perifs_t> _perifs;

    size_t _num_tx_channels;
    size_t _num_rx_channels;
    size_t _num_radios;

    // Cached values
    double _tick_rate;
    std::map<size_t, std::string> _antenna;
    std::map<size_t, double> _tx_freq;
    std::map<size_t, double> _rx_freq;
    std::map<size_t, double> _tx_gain;
    std::map<size_t, double> _rx_gain;

    std::map<size_t, bool> _rx_streamers_active;
    std::map<size_t, bool> _tx_streamers_active;
}; /* class radio_ctrl_impl */

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_RADIO_CTRL_IMPL_HPP */
// vim: sw=4 et:
