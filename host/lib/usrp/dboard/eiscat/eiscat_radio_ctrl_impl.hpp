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
 *
 * Note: This will control both daughterboards.
 */
class eiscat_radio_ctrl_impl : public radio_ctrl_impl
{
public:
    using sptr = boost::shared_ptr<eiscat_radio_ctrl_impl>;
    using fir_tap_t = int32_t;

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

    /*! Configures FPGA switching for antenna selection
     *
     *
     * Valid antenna values:
     * - BF: This is the default. Will apply the beamforming matrix in whatever
     *   state it currently is.
     * - RX0...RX15: Will mux the antenna signal 0...15 straight to this
     *   channel. Note that this will disable the FIR matrix entirely, and will
     *   also disable contributions from other USRPs.
     * - BF0...BF15: Will configure the FIR filter matrix such that only the
     *   contributions from antenna 0...15 are passed to this channel. This
     *   should produce the same signal as RX0..RX15, reduced by 12 dB (because
     *   the FIR matri needs to account for bit growth from adding 16 channels)
     * - FI$idx: Here, $idx is a number (the filter index, hence the name).
     *   This will apply filter index $idx to all input channels. Useful for
     *   testing actual beamforming applications, when the same signal is
     *   applied to all inputs.
     *
     * \throws uhd::value_error if the antenna value was not valid
     */
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

protected:
    virtual bool check_radio_config();

private:

    /*! Write filter taps for a specific FIR filter.
     *
     * Note: If the number of taps is smaller than the number of available
     * filter taps, it is padded with zero.
     *
     * \param fir_idx The index of the FIR filter we are reprogramming
     * \param taps A list of FIR filter taps for this filter.
     *
     * \throws uhd::value_error if the number of taps is longer than the number
     *                          of taps that the filter can handle, or if any
     *                          tap has more bits than allowed.
     */
    void write_fir_taps(
        const size_t fir_idx,
        const std::vector<fir_tap_t> &taps
    );


    /*! Choose a filter to be applied between an output beam and antenna input
     *
     * \param beam_index Beam index
     * \param antenna_index Antenna index
     * \param fir_index The index of the FIR filter taps that get applied
     * \param time_spec If non-zero, the taps get applied at this time
     */
    void select_filter(
        const size_t beam_index,
        const size_t antenna_index,
        const size_t fir_index,
        const uhd::time_spec_t &time_spec
    );

    /*! Sets the digital gain on a specific antenna
     *
     * \param antenna_idx Antenna for which this gain setting applies
     * \param normalized_gain A value in [0, 1] which gets converted to a
     *                        digital gain value
     */
    void set_antenna_gain(
        const size_t antenna_idx,
        const double normalized_gain
    );

    /*! Directly writes a value to the beam configuration register.
     */
    void configure_beams(uint32_t reg_value);

    /*! Controls selection of beams coming from the FIR matrix.
     *
     * The following values are allowed:
     * - 0: We stream the lower 5 beams, plus the neighbours contribution
     * - 1: We stream the upper 5 beams, plus the neighbours contribution
     * - 2: We stream the lower 5 beams, without the neighbours contribution
     * - 3: We stream the upper 5 beams, without the neighbours contribution
     */
    void set_beam_selection(int beam_selection);

    /*! Controls if we're using the FIR matrix
     *
     * If this is false, the beam selection is irrelevant.
     */
    void enable_firs(bool enable);

    /*! Enables counter instead of JESD core output
     */
    void enable_counter(bool enable);

    /*! The number of channels this block outputs
     *
     * This is *not* the number of antennas, but the number of streams a single
     * block outputs to the crossbar.
     */
    size_t _num_ports;

}; /* class radio_ctrl_impl */

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_EISCAT_RADIO_CTRL_IMPL_HPP */
// vim: sw=4 et:

