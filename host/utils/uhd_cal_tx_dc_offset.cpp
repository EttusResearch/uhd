//
// Copyright 2010,2012,2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "usrp_cal_utils.hpp"
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/algorithm.hpp>
#include <uhd/utils/paths.hpp>
#include <uhd/utils/safe_main.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <chrono>
#include <complex>
#include <ctime>
#include <functional>
#include <iostream>

namespace po = boost::program_options;

/***********************************************************************
 * Tune RX and TX routine
 **********************************************************************/
static double tune_rx_and_tx(
    uhd::usrp::multi_usrp::sptr usrp, const double tx_lo_freq, const double rx_offset)
{
    // tune the transmitter with no cordic
    uhd::tune_request_t tx_tune_req(tx_lo_freq);
    tx_tune_req.dsp_freq_policy = uhd::tune_request_t::POLICY_MANUAL;
    tx_tune_req.dsp_freq        = 0;
    usrp->set_tx_freq(tx_tune_req);

    // tune the receiver
    double rx_freq        = usrp->get_tx_freq() - rx_offset;
    double min_fe_rx_freq = usrp->get_fe_rx_freq_range().start();
    double max_fe_rx_freq = usrp->get_fe_rx_freq_range().stop();
    uhd::tune_request_t rx_tune_req(rx_freq);
    rx_tune_req.dsp_freq_policy = uhd::tune_request_t::POLICY_MANUAL;
    rx_tune_req.dsp_freq        = 0;
    if (rx_freq < min_fe_rx_freq)
        rx_tune_req.dsp_freq = rx_freq - min_fe_rx_freq;
    else if (rx_freq > max_fe_rx_freq)
        rx_tune_req.dsp_freq = rx_freq - max_fe_rx_freq;
    usrp->set_rx_freq(rx_tune_req);

    wait_for_lo_lock(usrp);

    return usrp->get_tx_freq();
}

/***********************************************************************
 * Main
 **********************************************************************/
int UHD_SAFE_MAIN(int argc, char* argv[])
{
    std::string args, subdev, serial;
    double tx_wave_freq, tx_wave_ampl, tx_gain, rx_offset;
    double freq_start, freq_stop, freq_step;
    size_t nsamps;
    double precision;

    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()
        ("help", "help message")
        ("verbose", "enable some verbose")
        ("args", po::value<std::string>(&args)->default_value(""), "device address args [default = \"\"]")
        ("subdev", po::value<std::string>(&subdev), "Subdevice specification (default: first subdevice, often 'A')")
        ("tx_wave_freq", po::value<double>(&tx_wave_freq)->default_value(507.123e3), "Transmit wave frequency in Hz")
        ("tx_wave_ampl", po::value<double>(&tx_wave_ampl)->default_value(0.7), "Transmit wave amplitude")
        ("tx_gain", po::value<double>(&tx_gain), "Tx gain in dB (do not specify for default)")
        ("rx_offset", po::value<double>(&rx_offset)->default_value(.9344e6), "RX LO offset from the TX LO in Hz")
        ("freq_start", po::value<double>(&freq_start), "Frequency start in Hz (do not specify for default)")
        ("freq_stop", po::value<double>(&freq_stop), "Frequency stop in Hz (do not specify for default)")
        ("freq_step", po::value<double>(&freq_step)->default_value(default_freq_step), "Step size for LO sweep in Hz")
        ("nsamps", po::value<size_t>(&nsamps), "Samples per data capture")
        ("precision", po::value<double>(&precision)->default_value(default_precision), "Correction precision (default=0.0001)")
    ;
    // clang-format on

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    // print the help message
    if (vm.count("help")) {
        std::cout << boost::format("USRP Generate TX DC Offset Calibration Table %s")
                         % desc
                  << std::endl;
        std::cout << "This application measures leakage between RX and TX on a "
                     "transceiver daughterboard to self-calibrate.\n"
                     "Note: Not all daughterboards support this feature. Refer to the "
                     "UHD manual for details.\n"
                  << std::endl;
        return EXIT_FAILURE;
    }

    // Create a USRP device
    uhd::usrp::multi_usrp::sptr usrp = setup_usrp_for_cal(args, subdev, serial);

    if (not vm.count("nsamps"))
        nsamps = size_t(usrp->get_rx_rate() / default_fft_bin_size);

    // create a receive streamer
    uhd::stream_args_t stream_args("fc32"); // complex floats
    uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);

    // create a transmit streamer
    uhd::tx_streamer::sptr tx_stream = usrp->get_tx_stream(stream_args);

    // create a transmitter thread
    std::atomic_flag transmit = ATOMIC_FLAG_INIT;
    std::atomic<bool> transmit_started(false);
    transmit.test_and_set();
    auto transmitter = std::thread(std::bind(&tx_thread,
        &transmit,
        usrp,
        tx_stream,
        tx_wave_freq,
        tx_wave_ampl,
        vm.count("tx_gain") ? std::optional<double>(tx_gain) : std::optional<double>(),
        std::ref(transmit_started)));

    // Wait for tx_thread to start transmitting
    size_t iter = 0;
    while (!transmit_started.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (++iter > 100) {
            std::cerr << "TX thread did not start transmitting within 10 seconds."
                      << std::endl;
            return EXIT_FAILURE;
        }
    }

    // re-usable buffer for samples
    std::vector<samp_type> buff;

    // store the results here
    std::vector<result_t> results;

    if (not vm.count("freq_start"))
        freq_start = usrp->get_fe_tx_freq_range().start();
    if (not vm.count("freq_stop"))
        freq_stop = usrp->get_fe_tx_freq_range().stop();

    // check start and stop frequencies
    if (freq_start < usrp->get_fe_tx_freq_range().start()) {
        std::cerr << "freq_start must be " << usrp->get_fe_tx_freq_range().start()
                  << " or greater for this daughter board" << std::endl;
        return EXIT_FAILURE;
    }
    if (freq_stop > usrp->get_fe_tx_freq_range().stop()) {
        std::cerr << "freq_stop must be " << usrp->get_fe_tx_freq_range().stop()
                  << " or less for this daughter board" << std::endl;
        return EXIT_FAILURE;
    }

    // check rx_offset
    double min_rx_offset =
        usrp->get_rx_freq_range().start() - usrp->get_fe_tx_freq_range().start();
    double max_rx_offset =
        usrp->get_rx_freq_range().stop() - usrp->get_fe_tx_freq_range().stop();
    if (rx_offset < min_rx_offset or rx_offset > max_rx_offset) {
        std::cerr << "rx_offset must be between " << min_rx_offset << " and "
                  << max_rx_offset << " for this daughter board" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << boost::format("Calibration frequency range: %d MHz -> %d MHz")
                     % (freq_start / 1e6) % (freq_stop / 1e6)
              << std::endl;

    // set RX gain
    usrp->set_rx_gain(0);

    size_t tx_error_count = 0;
    for (double tx_lo_i = freq_start; tx_lo_i <= freq_stop; tx_lo_i += freq_step) {
        const double tx_lo = tune_rx_and_tx(usrp, tx_lo_i, rx_offset);

        // frequency constants for this tune event
        const double actual_rx_rate = usrp->get_rx_rate();
        const double actual_tx_freq = usrp->get_tx_freq();
        const double actual_rx_freq = usrp->get_rx_freq();
        const double bb_dc_freq     = actual_tx_freq - actual_rx_freq;

        // reset TX DC offset
        usrp->set_tx_dc_offset(std::complex<double>(0, 0));

        // capture initial uncorrected value
        capture_samples(usrp, rx_stream, buff, nsamps);
        const double initial_dc_dbrms =
            compute_tone_dbrms(buff, bb_dc_freq / actual_rx_rate);

        // bounds and results from searching
        double i_corr_start  = -1.0;
        double i_corr_stop   = 1.0;
        double i_corr_step   = (i_corr_stop - i_corr_start) / (num_search_steps + 1);
        double q_corr_start  = -1.0;
        double q_corr_stop   = 1.0;
        double q_corr_step   = (q_corr_stop - q_corr_start) / (num_search_steps + 1);
        double best_dc_dbrms = initial_dc_dbrms;
        double best_i_corr   = 0;
        double best_q_corr   = 0;
        while (i_corr_step >= precision or q_corr_step >= precision) {
            for (double i_corr = i_corr_start + i_corr_step;
                 i_corr <= i_corr_stop - i_corr_step;
                 i_corr += i_corr_step) {
                for (double q_corr = q_corr_start + q_corr_step;
                     q_corr <= q_corr_stop - q_corr_step;
                     q_corr += q_corr_step) {
                    const std::complex<double> correction(i_corr, q_corr);
                    usrp->set_tx_dc_offset(correction);

                    // receive some samples
                    capture_samples(usrp, rx_stream, buff, nsamps);
                    // check for TX errors in the current captured iteration
                    if (has_tx_error(tx_stream)) {
                        std::cout << "[WARNING] TX error detected! "
                                  << "Repeating current iteration" << std::endl;
                        // Undo the Q correction step
                        q_corr -= q_corr_step;
                        tx_error_count++;
                        if (tx_error_count >= MAX_NUM_TX_ERRORS) {
                            throw uhd::runtime_error(
                                "Too many TX errors. Aborting calibration.");
                        }
                        continue;
                    }
                    const double dc_dbrms =
                        compute_tone_dbrms(buff, bb_dc_freq / actual_rx_rate);

                    if (dc_dbrms < best_dc_dbrms) {
                        best_dc_dbrms = dc_dbrms;
                        best_i_corr   = i_corr;
                        best_q_corr   = q_corr;
                    }
                }
            }

            i_corr_start = best_i_corr - i_corr_step;
            i_corr_stop  = best_i_corr + i_corr_step;
            i_corr_step  = (i_corr_stop - i_corr_start) / (num_search_steps + 1);
            q_corr_start = best_q_corr - q_corr_step;
            q_corr_stop  = best_q_corr + q_corr_step;
            q_corr_step  = (q_corr_stop - q_corr_start) / (num_search_steps + 1);
        }

        if (best_dc_dbrms < initial_dc_dbrms) // keep result
        {
            result_t result;
            result.freq      = tx_lo;
            result.real_corr = best_i_corr;
            result.imag_corr = best_q_corr;
            result.best      = best_dc_dbrms;
            result.delta     = initial_dc_dbrms - best_dc_dbrms;
            results.push_back(result);
            if (vm.count("verbose"))
                std::cout << boost::format(
                                 "TX DC: %f MHz: lowest offset %f dB, corrected %f dB")
                                 % (tx_lo / 1e6) % result.best % result.delta
                          << std::endl;
            else
                std::cout << "." << std::flush;
        }

        // Reset underrun counts, start a new counter for the next frequency
        tx_error_count = 0;
    } // end for each frequency loop

    std::cout << std::endl;

    // stop the transmitter
    transmit.clear();
    transmitter.join();

    store_results(results, "TX", "tx", "dc", serial);

    return EXIT_SUCCESS;
}
