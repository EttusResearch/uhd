//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_RFNOC_EISCAT_RADIO_CTRL_IMPL_HPP
#define INCLUDED_LIBUHD_RFNOC_EISCAT_RADIO_CTRL_IMPL_HPP

#include <uhd/types/direction.hpp>
#include <uhdlib/rfnoc/rpc_block_ctrl.hpp>
#include <uhdlib/rfnoc/radio_ctrl_impl.hpp>

namespace uhd {
    namespace rfnoc {

/*! \brief Provide access to an EISCAT radio, including beamformer.
 *
 * Note: This will control both daughterboards. Since we have a single RFNoC
 * block, we only have one of these per motherboard.
 *
 * EISCAT radios have a whole bunch of features which don't have APIs provided
 * by radio_ctrl. This means the most interesting features are controlled by
 * set_arg() and get_arg(). Notable exception is set_rx_antenna(), which is
 * heavily abused for all sorts of things.
 *
 * List of relevant args:
 * - sysref (bool): Write to this to trigger a SYSREF pulse to *both*
 *                  daughterboards. Will honor command time. Will always return
 *                  true when read.
 * - gain (double): Set the gain for antenna X, where X is the set_arg() `port`
 *                  value. The gain is normalized in [0,1]. Can be read to get
 *                  the current value. Example: `set_arg("gain", 0.5, 5)` will
 *                  set the digital gain for antenna 5 to mid-point.
 * - fir_ctrl_time (time_spec_t): This time will be used for following
 *                                fir_select writes. Will return the last value
 *                                that was written.
 * - fir_select (int): Will queue a filter for manipulating a specific
 *                     contribution. The value is the filter index in the BRAM.
 *                     The port parameter specifies which filter; filters are
 *                     indexed 0...159 using the equation beam_index * 16 +
 *                     antenna_idx. Example: `set_arg("fir_select", 357, 16)`
 *                     will apply filter number 357 to the zeroth antenna for
 *                     beam number 1 (i.e. the second beam). Returns the last
 *                     value that was written. May be incorrect before written
 *                     for the first time.
 * - fir_taps (vector<int32_t>): Updates FIR tap values in the BRAM. Port is
 *                               the filter index. Will always return an impulse
 *                               response, not the actual filter value.
 * - assert_adcs_deframers (bool): Writing this does nothing. Reading it back
 *                                 will run the initialization of ADCs and
 *                                 deframers. Return value is success.
 * - assert_deframer_status (bool): Writing this does nothing. Reading it will
 *                                  run the final step of the JESD deframer
 *                                  initialization routine. Returns success.
 * - choose_beams (int): Configures beam selection (upper, lower, are neighbour
 *                       contributions included). See set_beam_selection() for
 *                       details.
 * - enable_firs (int): Can be used to disable fir FIR matrix. This routes the
 *                      JESD output directly to the noc_shell.
 * - enable_counter (int): If the feature is available in the given FPGA image,
 *                         setting this to true will disable the JESD core
 *                         output and will input a counter signal (ramp)
 *                         instead.
 * - configure_beams (int): Danger, danger: Directly writes the
 *                          SR_BEAMS_TO_NEIGHBOR register. Writing this can put
 *                          some of the other properties out of sync, because
 *                          writing to those will also write to this, but not
 *                          vice versa.
 *
 *
 * ## Time-aligned synchronization sequence:
 *
 * 0. Make sure all devices are getting the same ref clock and PPS!
 * 1. Call set_command_time() with the same time on all blocks (make it far
 *    enough in the future)
 * 2. Call set_arg<bool>("sysref") on all blocks. This should SYSREF all dboards
 *    synchronously.
 * 3. On all blocks, call get_arg<bool>("assert_adcs_deframers") and verify it
 *    returns true.
 * 4. Repeat steps 1 and 2 with, obviously, another time that's in the future.
 * 5. On all blocks, call get_arg<bool>("assert_deframer_status") and make sure
 *    it returned true.
 */
class eiscat_radio_ctrl_impl : public radio_ctrl_impl, public rpc_block_ctrl
{
public:
    using sptr = boost::shared_ptr<eiscat_radio_ctrl_impl>;
    using fir_tap_t = int32_t; // See also EISCAT_BITS_PER_TAP

    /************************************************************************
     * Structors
     ***********************************************************************/
    UHD_RFNOC_RADIO_BLOCK_CONSTRUCTOR_DECL(eiscat_radio_ctrl)
    virtual ~eiscat_radio_ctrl_impl();

    /************************************************************************
     * API calls
     * Note: Tx calls are here mostly to throw errors.
     ***********************************************************************/
    //! Returns the actual tick rate. Will display a warning if rate is not that
    // value.
    double set_rate(double rate);

    //! \throws uhd::runtime_error
    void set_tx_antenna(const std::string &ant, const size_t chan);

    /*! Configures FPGA switching for antenna selection
     *
     * Valid antenna values:
     * - BF: This is the default. Will apply the beamforming matrix in whatever
     *   state it currently is.
     * - RX0...RX15: Will mux the antenna signal 0...15 straight to this
     *   channel. Note that this will disable the FIR matrix entirely, and will
     *   also disable contributions from other USRPs globally.
     * - BF0...BF15: Will configure the FIR filter matrix such that only the
     *   contributions from antenna 0...15 are passed to this channel. This
     *   should produce the same signal as RX0..RX15, reduced by 12 dB (because
     *   the FIR matri needs to account for bit growth from adding 16 channels).
     *   Will also disable contributions from other channels globally.
     * - FI$idx: Here, $idx is a number (the filter index, hence the name).
     *   This will apply filter index $idx to all input channels. Useful for
     *   testing actual beamforming applications, when the same signal is
     *   applied to all inputs.
     *
     * Note that this is very useful for testing and debugging. For actual
     * beamforming operations, this API call won't be enough. Rather, set this
     * to 'BF' (or don't do anything) and use the block properties
     *
     * \throws uhd::value_error if the antenna value was not valid
     */
    void set_rx_antenna(const std::string &ant, const size_t chan);

    //! \throws uhd::runtime_error
    double set_tx_frequency(const double freq, const size_t chan);
    //! \returns Some value in the EISCAT passband
    double set_rx_frequency(const double freq, const size_t chan);
    //! \returns Width of the EISCAT analog frontend filters
    double set_rx_bandwidth(const double bandwidth, const size_t chan);
    //! \throws uhd::runtime_error
    double get_tx_frequency(const size_t chan);

    //! \throws uhd::runtime_error
    double set_tx_gain(const double gain, const size_t chan);
    //! \returns zero
    double set_rx_gain(const double gain, const size_t chan);

    size_t get_chan_from_dboard_fe(const std::string &fe, const uhd::direction_t dir);
    std::string get_dboard_fe_from_chan(const size_t chan, const uhd::direction_t dir);

    //! \returns The EISCAT sampling rate
    double get_output_samp_rate(size_t port);

protected:
    virtual bool check_radio_config();

    /*! Finalize initialization sequence (ADCs, deframers) etc.
     */
    void set_rpc_client(
        uhd::rpc_client::sptr rpcc,
        const uhd::device_addr_t &block_args
    );

private:
    /*************************************************************************
     * Private methods
     * To control the dboard (and execute these), take a look at the block
     * properties.
     ************************************************************************/
    /*! Write filter taps for a specific FIR filter.
     *
     * Note: If the number of taps is smaller than the number of available
     * filter taps, it is padded with zero (i.e., all taps are always written
     * and this can't be use to partially update filters).
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
     * \param time_spec If non-zero, the taps get applied at this time.
     *                  Otherwise, they get sent out now.
     * \param write_time If false, time will never get written *even if* it is
     *                   non-zero. The assumption is that someone else wrote
     *                   the value previously
     *                   \param write_time If false, time will never get written *even if* it is
     *                                     non-zero. The assumption is that someone else wrote
     *                                     the value previously 
     */
    void select_filter(
        const size_t beam_index,
        const size_t antenna_index,
        const size_t fir_index,
        const uhd::time_spec_t &time_spec,
        const bool write_time=true
    );


    /*! Sets the command time for the next call to select_filter()
     *
     * \param time_spec This value gets written to the FPGA and is applied to
     *                  *all* subsequent filter selections. To request
     *                  immediate application of filters, set this to zero.
     */
    void set_fir_ctrl_time(const uhd::time_spec_t &time_spec);

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

    //! Sends a SYSREF pulse. Device arg use_mpm_sysref can be used to send it
    // via MPM. Default is to send it via CHDR, in which case calling this
    // function *will modify the command time!*, but it will ensure that the
    // sysref is sent on an even time
    void send_sysref();

    //! Run initialization of JESD cores, put ADCs into reset
    bool assert_jesd_cores_initialized();

    //! Run initialization of ADCs and deframers; returns success status
    bool assert_adcs_deframers();

    //! Run final step of JESD core setup; returns success status
    bool assert_deframer_status();

    /*! The number of channels this block outputs
     *
     * This is *not* the number of antennas, but the number of streams a single
     * block outputs to the crossbar.
     */
    size_t _num_ports;

    //! Running with 1 dboard is theoretically possible; thus, store the
    // number of active dboards.
    size_t _num_dboards = 0;

    //! Additional block args; gets set during set_rpc_client()
    uhd::device_addr_t _block_args;

    /*! Reference to the RPC client
     */
    uhd::rpc_client::sptr _rpcc;

}; /* class radio_ctrl_impl */

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_EISCAT_RADIO_CTRL_IMPL_HPP */
// vim: sw=4 et:

