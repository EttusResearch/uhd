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

#ifndef INCLUDED_LIBUHD_RFNOC_EISCAT_RADIO_CTRL_IMPL_HPP
#define INCLUDED_LIBUHD_RFNOC_EISCAT_RADIO_CTRL_IMPL_HPP

#include "radio_ctrl_impl.hpp"
#include "uhd/types/direction.hpp"

namespace uhd {
    namespace rfnoc {

/*! \brief Provide access to an eiscat radio.
 */
class eiscat_radio_ctrl_impl : public radio_ctrl_impl
{
public:
    typedef boost::shared_ptr<eiscat_radio_ctrl_impl> sptr;

    /************************************************************************
     * Structors
     ***********************************************************************/
    UHD_RFNOC_RADIO_BLOCK_CONSTRUCTOR_DECL(eiscat_radio_ctrl)
    virtual ~eiscat_radio_ctrl_impl();

    /************************************************************************
     * API calls
     * Note: Tx calls are here mostly to throw errors.
     ***********************************************************************/
    double set_rate(double rate);

    void set_tx_antenna(const std::string &ant, const size_t chan);
    void set_rx_antenna(const std::string &ant, const size_t chan);

    double set_tx_frequency(const double freq, const size_t chan);
    double set_rx_frequency(const double freq, const size_t chan);
    double set_rx_bandwidth(const double bandwidth, const size_t chan);
    double get_tx_frequency(const size_t chan);

    double set_tx_gain(const double gain, const size_t chan);
    double set_rx_gain(const double gain, const size_t chan);

    size_t get_chan_from_dboard_fe(const std::string &fe, const uhd::direction_t dir);
    std::string get_dboard_fe_from_chan(const size_t chan, const uhd::direction_t dir);

    double get_output_samp_rate(size_t port);

private:
    enum radio_connection_t { PRIMARY, SECONDARY };

    radio_connection_t                  _radio_type;
    std::string                         _radio_slot;
}; /* class radio_ctrl_impl */

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_EISCAT_RADIO_CTRL_IMPL_HPP */
// vim: sw=4 et:

