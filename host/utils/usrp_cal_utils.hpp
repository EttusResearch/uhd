//
// Copyright 2011-2012,2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/cal/database.hpp>
#include <uhd/cal/iq_cal.hpp>
#include <uhd/property_tree.hpp>
#include <uhd/usrp/dboard_eeprom.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/algorithm.hpp>
#include <uhd/utils/math.hpp>
#include <uhd/utils/paths.hpp>
#include <uhd/utils/thread.hpp>
#include <atomic>
#include <chrono>
#include <complex>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>

struct result_t
{
    double freq, real_corr, imag_corr, best, delta;
};

typedef std::complex<float> samp_type;

/***********************************************************************
 * Constants
 **********************************************************************/
static const double tau                   = 2 * uhd::math::PI;
static const size_t num_search_steps      = 5;
static const double default_precision     = 0.0001;
static const double default_freq_step     = 7.3e6;
static const size_t default_fft_bin_size  = 1000;
static constexpr size_t MAX_NUM_TX_ERRORS = 10;

/***********************************************************************
 * Set standard defaults for devices
 **********************************************************************/
static inline void set_optimum_defaults(uhd::usrp::multi_usrp::sptr usrp)
{
    constexpr size_t chan = 0;
    const auto rx_info    = usrp->get_usrp_rx_info(chan);
    const auto tx_info    = usrp->get_usrp_tx_info(chan);

    const std::string mb_name = rx_info["mboard_id"];
    if (mb_name.find("USRP2") != std::string::npos
        or mb_name.find("N200") != std::string::npos
        or mb_name.find("N210") != std::string::npos
        or mb_name.find("X300") != std::string::npos
        or mb_name.find("X310") != std::string::npos
        or mb_name.find("NI-2974") != std::string::npos
        or mb_name.find("n3xx") != std::string::npos) {
        usrp->set_tx_rate(12.5e6);
        usrp->set_rx_rate(12.5e6);
    } else if (mb_name.find("n320") != std::string::npos) {
        usrp->set_tx_rate(12.288e6);
        usrp->set_rx_rate(12.288e6);
    } else if (mb_name.find("B100") != std::string::npos) {
        usrp->set_tx_rate(4e6);
        usrp->set_rx_rate(4e6);
    } else {
        throw std::runtime_error(
            std::string("self-calibration is not supported for this device: ") + mb_name);
    }

    const std::string tx_name = tx_info["tx_subdev_name"];
    if (tx_name.find("WBX") == std::string::npos
        and tx_name.find("SBX") == std::string::npos
        and tx_name.find("CBX") == std::string::npos
        and tx_name.find("RFX") == std::string::npos
        and tx_name.find("UBX") == std::string::npos
        and tx_name.find("Rhodium") == std::string::npos) {
        throw std::runtime_error(
            std::string("self-calibration is not supported for this TX dboard :")
            + tx_name);
    }
    usrp->set_tx_gain(0);

    const std::string rx_name = rx_info["rx_subdev_name"];
    if (rx_name.find("WBX") == std::string::npos
        and rx_name.find("SBX") == std::string::npos
        and rx_name.find("CBX") == std::string::npos
        and rx_name.find("RFX") == std::string::npos
        and rx_name.find("UBX") == std::string::npos
        and rx_name.find("Rhodium") == std::string::npos) {
        throw std::runtime_error(
            std::string("self-calibration is not supported for this RX dboard :")
            + rx_name);
    }
    usrp->set_rx_gain(0);
}

/***********************************************************************
 * Retrieve d'board serial
 **********************************************************************/
static std::string get_serial(uhd::usrp::multi_usrp::sptr usrp, const std::string& tx_rx)
{
    const size_t chan            = 0;
    auto usrp_info               = (tx_rx == "tx") ? usrp->get_usrp_tx_info(chan)
                                                   : usrp->get_usrp_rx_info(chan);
    const std::string serial_key = tx_rx + "_serial";
    if (!usrp_info.has_key(serial_key)) {
        throw uhd::runtime_error("Cannot determine daughterboard serial!");
    }

    return usrp_info[serial_key];
}

/***********************************************************************
 * Check for empty serial
 **********************************************************************/
void check_for_empty_serial(uhd::usrp::multi_usrp::sptr usrp)
{
    if (get_serial(usrp, "rx").empty()) {
        std::string error_string =
            "This dboard has no serial!\n\nPlease see the Calibration "
            "documentation for details on how to fix this.";
        throw std::runtime_error(error_string);
    }
}

/***********************************************************************
 * Compute power of a tone
 **********************************************************************/
static inline double compute_tone_dbrms(const std::vector<samp_type>& samples,
    const double freq) // freq is fractional
{
    // shift the samples so the tone at freq is down at DC
    // and average the samples to measure the DC component
    samp_type average = 0;
    for (size_t i = 0; i < samples.size(); i++)
        average += samp_type(std::polar(1.0, -freq * tau * i)) * samples[i];

    return 20 * std::log10(std::abs(average / float(samples.size())));
}

/***********************************************************************
 * Write a dat file
 **********************************************************************/
static inline void write_samples_to_file(
    const std::vector<samp_type>& samples, const std::string& file)
{
    std::ofstream outfile(file.c_str(), std::ofstream::binary);
    outfile.write((const char*)&samples.front(), samples.size() * sizeof(samp_type));
    outfile.close();
}

/***********************************************************************
 * Store data to file
 **********************************************************************/
static void store_results(const std::vector<result_t>& results,
    const std::string& XX, // "TX" or "RX"
    const std::string& xx, // "tx" or "rx"
    const std::string& what, // Type of test, e.g. "iq",
    const std::string& serial)
{
    using namespace uhd::usrp::cal;
    // Note: We could also load existing data and update it.
    auto cal_data = iq_cal::make(XX + " Frontend Calibration", serial, time(NULL));
    for (size_t i = 0; i < results.size(); i++) {
        cal_data->set_cal_coeff(results[i].freq,
            {results[i].real_corr, results[i].imag_corr},
            results[i].best,
            results[i].delta);
    }

    const std::string cal_key = xx + "_" + what;
    database::write_cal_data(cal_key, serial, cal_data->serialize());
}

/***********************************************************************
 * Data capture routine
 **********************************************************************/
static void capture_samples(uhd::usrp::multi_usrp::sptr usrp,
    uhd::rx_streamer::sptr rx_stream,
    std::vector<samp_type>& buff,
    const size_t nsamps_requested)
{
    buff.resize(nsamps_requested);
    uhd::rx_metadata_t md;

    // Right after the stream is started, there will be transient data.
    // That transient data is discarded and only "good" samples are returned.
    size_t nsamps_to_discard = size_t(usrp->get_rx_rate() * 0.001); // 1ms to be discarded
    std::vector<samp_type> discard_buff(nsamps_to_discard);

    uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
    stream_cmd.num_samps  = buff.size() + nsamps_to_discard;
    stream_cmd.stream_now = true;
    usrp->issue_stream_cmd(stream_cmd);
    size_t num_rx_samps = 0;

    // Discard the transient samples.
    rx_stream->recv(&discard_buff.front(), discard_buff.size(), md);
    if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE) {
        throw std::runtime_error(std::string("Receiver error: ") + md.strerror());
    }

    // Now capture the data we want
    num_rx_samps = rx_stream->recv(&buff.front(), buff.size(), md);

    // validate the received data
    if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE) {
        throw std::runtime_error(std::string("Receiver error: ") + md.strerror());
    }

    // we can live if all the data didnt come in
    if (num_rx_samps > buff.size() / 2) {
        buff.resize(num_rx_samps);
        return;
    }
    if (num_rx_samps != buff.size())
        throw std::runtime_error("did not get all the samples requested");
}

/***********************************************************************
 * Setup function
 **********************************************************************/
static uhd::usrp::multi_usrp::sptr setup_usrp_for_cal(
    std::string& args, std::string& subdev, std::string& serial)
{
    const std::string args_with_ignore = args + ",ignore_cal_file=1,ignore-cal-file=1";
    std::cout << std::endl;
    std::cout << "Creating the usrp device with: " << args_with_ignore << "..."
              << std::endl;
    uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args_with_ignore);

    // Configure subdev
    if (!subdev.empty()) {
        usrp->set_tx_subdev_spec(subdev);
        usrp->set_rx_subdev_spec(subdev);
    }
    std::cout << "Running calibration for " << usrp->get_tx_subdev_name(0) << std::endl;
    serial = get_serial(usrp, "tx");
    std::cout << "Daughterboard serial: " << serial << std::endl;

    // set the antennas to cal
    if (not uhd::has(usrp->get_rx_antennas(), "CAL")
        or not uhd::has(usrp->get_tx_antennas(), "CAL"))
        throw std::runtime_error(
            "This board does not have the CAL antenna option, cannot self-calibrate.");
    usrp->set_rx_antenna("CAL");
    usrp->set_tx_antenna("CAL");

    // fail if daughterboard has no serial
    check_for_empty_serial(usrp);

    // set optimum defaults
    set_optimum_defaults(usrp);

    return usrp;
}

/***********************************************************************
 * Function to find optimal RX gain setting (for the current frequency)
 **********************************************************************/
UHD_INLINE void set_optimal_rx_gain(uhd::usrp::multi_usrp::sptr usrp,
    uhd::rx_streamer::sptr rx_stream,
    double wave_freq = 0.0,
    double gain_step = 3.0)
{
    const double gain_step_threshold = gain_step * 0.5;
    const double actual_rx_rate      = usrp->get_rx_rate();
    const double actual_tx_freq      = usrp->get_tx_freq();
    const double actual_rx_freq      = usrp->get_rx_freq();
    const double bb_tone_freq        = actual_tx_freq - actual_rx_freq + wave_freq;
    const size_t nsamps              = size_t(actual_rx_rate / default_fft_bin_size);

    std::vector<samp_type> buff(nsamps);
    uhd::gain_range_t rx_gain_range = usrp->get_rx_gain_range();
    double rx_gain                  = rx_gain_range.start() + gain_step;
    double curr_dbrms               = 0.0;
    double prev_dbrms               = 0.0;
    double delta                    = 0.0;

    // No sense in setting the gain where this is no gain range
    if (rx_gain_range.stop() - rx_gain_range.start() < gain_step)
        return;

    // The algorithm below cycles through the RX gain range
    // looking for the point where the signal begins to get
    // clipped and the gain begins to be compressed.  It does
    // this by looking for the gain setting where the increase
    // in the tone is less than the gain step by more than the
    // gain compression threshold (curr - prev < gain - threshold).
    // This routine starts searching at the bottom of the gain range
    // rather than the top in order to minimize the chances of
    // exposing frontend components to a dangerous amount of power
    // from the incoming signal.

    // Initialize prev_dbrms value
    usrp->set_rx_gain(rx_gain);
    capture_samples(usrp, rx_stream, buff, nsamps);
    prev_dbrms = compute_tone_dbrms(buff, bb_tone_freq / actual_rx_rate);
    rx_gain += gain_step;

    // First, get the signal above the noise floor
    while (rx_gain <= rx_gain_range.stop()) {
        usrp->set_rx_gain(rx_gain);
        capture_samples(usrp, rx_stream, buff, nsamps);
        curr_dbrms = compute_tone_dbrms(buff, bb_tone_freq / actual_rx_rate);
        delta      = curr_dbrms - prev_dbrms;

        // Check that the signal power is not already high
        if (curr_dbrms >= 0)
            break;
        // Check that the signal power has increased as the gain increases
        if (delta >= gain_step - gain_step_threshold)
            break;

        prev_dbrms = curr_dbrms;
        rx_gain += gain_step;
    }

    // Find RX gain where signal begins to clip
    while (rx_gain <= rx_gain_range.stop()) {
        usrp->set_rx_gain(rx_gain);
        capture_samples(usrp, rx_stream, buff, nsamps);
        curr_dbrms = compute_tone_dbrms(buff, bb_tone_freq / actual_rx_rate);
        delta      = curr_dbrms - prev_dbrms;

        // check if the gain is compressed beyone the threshold
        if (delta < gain_step - gain_step_threshold)
            break; // if so, we are done

        prev_dbrms = curr_dbrms;
        rx_gain += gain_step;
    }

    // The rx_gain value at this point is the gain setting where clipping
    // occurs or the gain setting that is just beyond the gain range.
    // The gain is reduced by 2 steps to make sure it is within the range and
    // under the point where it is clipped with enough room to make adjustments.
    rx_gain -= 2 * gain_step;

    // Make sure the gain is within the range.
    rx_gain = rx_gain_range.clip(rx_gain);

    // Finally, set the gain.
    usrp->set_rx_gain(rx_gain);
}

/***********************************************************************
 * Transmit thread
 **********************************************************************/
static void tx_thread(std::atomic_flag* transmit,
    uhd::usrp::multi_usrp::sptr usrp,
    uhd::tx_streamer::sptr tx_stream,
    const double tx_wave_freq,
    const double tx_wave_ampl)
{
    // increase thread priority for TX to prevent underruns
    uhd::set_thread_priority_safe();

    // set max TX gain
    usrp->set_tx_gain(usrp->get_tx_gain_range().stop());

    // setup variables
    uhd::tx_metadata_t md;
    md.has_time_spec        = false;
    const double tx_rate    = usrp->get_tx_rate();
    const size_t frame_size = tx_stream->get_max_num_samps();

    // set up buffer
    // make buffer size of 1 second aligned to a complete wave
    // to provide accuracy down to 1 Hz with no discontinuity
    const size_t buff_size =
        tx_wave_freq == 0.0
            ? frame_size
            : static_cast<size_t>(tx_rate)
                  - static_cast<size_t>((tx_wave_freq - static_cast<size_t>(tx_wave_freq))
                                        * tx_rate / tx_wave_freq);
    // Since send calls are aligned to the frame size, make the buffer 1 frame
    // larger to prevent an overrun when it reaches the end and wraps around.
    std::vector<samp_type> buff(buff_size + frame_size);

    // fill buffer
    for (size_t i = 0; i < buff.size(); i++) {
        if (tx_wave_freq == 0.0) {
            // fill with constant value
            buff[i] = samp_type(static_cast<float>(tx_wave_ampl), 0.0);
        } else {
            // fill with sine waves
            buff[i] =
                samp_type(std::polar(tx_wave_ampl, (tau * i * tx_wave_freq / tx_rate)));
        }
    }

    // send until stopped
    size_t index = 0;
    while (transmit->test_and_set()) {
        // send calls are aligned to the frame size for optimal performance
        tx_stream->send(&buff[index], frame_size, md);

        // increment index
        index += frame_size;

        // wrap around at end of buffer
        // (actual buffer size is 1 frame larger to prevent overrun)
        index %= buff_size;
    }

    // send a mini EOB packet
    md.end_of_burst = true;
    tx_stream->send("", 0, md);
}

/*! Returns true if any error on the TX stream has occurred
 */
bool has_tx_error(uhd::tx_streamer::sptr tx_stream)
{
    uhd::async_metadata_t async_md;
    if (!tx_stream->recv_async_msg(async_md, 0.0)) {
        return false;
    }

    return async_md.event_code
           & (0
               // Any of these errors are considered a problematic TX error:
               | uhd::async_metadata_t::EVENT_CODE_UNDERFLOW
               | uhd::async_metadata_t::EVENT_CODE_SEQ_ERROR
               | uhd::async_metadata_t::EVENT_CODE_TIME_ERROR
               | uhd::async_metadata_t::EVENT_CODE_UNDERFLOW_IN_PACKET
               | uhd::async_metadata_t::EVENT_CODE_SEQ_ERROR_IN_BURST);
}

void wait_for_lo_lock(uhd::usrp::multi_usrp::sptr usrp)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    const auto timeout =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(100);
    while (not usrp->get_tx_sensor("lo_locked").to_bool()
           or not usrp->get_rx_sensor("lo_locked").to_bool()) {
        if (std::chrono::steady_clock::now() > timeout) {
            throw std::runtime_error("timed out waiting for TX and/or RX LO to lock");
        }
    }
}
