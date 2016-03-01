//
// Copyright 2015 Ettus Research LLC
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

#ifndef INCLUDED_LIBUHD_RFNOC_RADIO_CTRL_HPP
#define INCLUDED_LIBUHD_RFNOC_RADIO_CTRL_HPP

#include <uhd/rfnoc/source_block_ctrl_base.hpp>
#include <uhd/rfnoc/sink_block_ctrl_base.hpp>
#include <uhd/rfnoc/rate_node_ctrl.hpp>
#include <uhd/rfnoc/tick_node_ctrl.hpp>
#include <uhd/rfnoc/scalar_node_ctrl.hpp>
#include <uhd/rfnoc/terminator_node_ctrl.hpp>

namespace uhd {
    namespace rfnoc {

/*! \brief Block controller for all RFNoC-based radio blocks
 */
class UHD_API radio_ctrl :
    public source_block_ctrl_base,
    public sink_block_ctrl_base,
    public rate_node_ctrl,
    public tick_node_ctrl,
    public terminator_node_ctrl
{
public:
    UHD_RFNOC_BLOCK_OBJECT(radio_ctrl)

    /************************************************************************
     * API calls
     ***********************************************************************/
    /*! Return the tick/sample rate on all channels (rx and tx).
     *
     * \return The tick rate.
     */
    virtual double get_rate() const = 0;

    /*! Set the tick/sample rate on all channels (rx and tx).
     *
     * Will coerce to the nearest possible rate and return the actual value.
     */
    virtual double set_rate(double rate) = 0;

    /*! Return the selected TX antenna for channel \p chan.
     *
     * \return The selected antenna.
     */
    virtual std::string get_tx_antenna(const size_t chan) /* const */ = 0;

    /*! Select RX antenna \p for channel \p chan.
     *
     * \throws uhd::value_error if \p ant is not a valid value.
     */
    virtual void set_tx_antenna(const std::string &ant, const size_t chan) = 0;

    /*! Return the selected RX antenna for channel \p chan.
     *
     * \return The selected antenna.
     */
    virtual std::string get_rx_antenna(const size_t chan) /* const */ = 0;

    /*! Select RX antenna \p for channel \p chan.
     *
     * \throws uhd::value_error if \p ant is not a valid value.
     */
    virtual void set_rx_antenna(const std::string &ant, const size_t chan) = 0;

    /*! Return the current transmit LO frequency on channel \p chan.
     *
     * Note that the AD9361 only has one LO for all TX channels, and the
     * \p chan parameter is thus only for API compatibility.
     *
     * \return The current LO frequency.
     */
    virtual double get_tx_frequency(const size_t chan) /* const */ = 0;

    /*! Tune the TX LO for channel \p.
     *
     * This function will attempt to tune as close as possible, and return a
     * coerced value of the actual tuning result.
     *
     * \return The actual LO frequency.
     */
    virtual double set_tx_frequency(const double freq, size_t chan) = 0;

    /*! Return the current receive LO frequency on channel \p chan.
     *
     * \return The current LO frequency.
     */
    virtual double get_rx_frequency(const size_t chan) /* const */ = 0;

    /*! Tune the RX LO for channel \p.
     *
     * This function will attempt to tune as close as possible, and return a
     * coerced value of the actual tuning result.
     *
     * \return The actual LO frequency.
     */
    virtual double set_rx_frequency(const double freq, const size_t chan) = 0;

    /*! Return the transmit gain on channel \p chan
     *
     * \return The actual gain value
     */
    virtual double get_tx_gain(const size_t chan) = 0;

    /*! Set the transmit gain on channel \p chan
     *
     * This function will attempt to set the gain as close as possible,
     * and return a coerced value of the actual gain value.
     *
     * \return The actual gain value
     */
    virtual double set_tx_gain(const double gain, const size_t chan) = 0;

    /*! Return the transmit gain on channel \p chan
     *
     * \return The actual gain value
     */
    virtual double get_rx_gain(const size_t chan) = 0;

    /*! Set the transmit gain on channel \p chan
     *
     * This function will attempt to set the gain as close as possible,
     * and return a coerced value of the actual gain value.
     *
     * \return The actual gain value
     */
    virtual double set_rx_gain(const double gain, const size_t chan) = 0;

}; /* class radio_ctrl */

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_RADIO_CTRL_HPP */
