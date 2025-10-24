//
// Copyright 2025 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

/***********************************************************************
 * UHD TX/RX Complex Gain Example
 *
 * This example demonstrates applying complex gain to TX and RX channels.
 *
 * The following usage example applies a TX gain of 0.5+0.5j immediately
 * and a RX gain of 0.5-0.5j after 10 us:
 * ./txrx_complex_gain --tx-args addr=192.168.10.2 --rx-args addr=192.168.10.2
 *      --tx-rate 1e6 --rx-rate 1e6 --tx-freq 1e9 --rx-freq 1e9 \
 *      --tx-gain 0.5+0.5j --rx-gain 0.5-0.5j --rx-gain-delay 10e-6 \
 *      --tx-channels 0 --rx-channels 0,1 --ampl 0.5 --wave-freq 1e5 \
 * --wave-type CONST --nsamps 1000 --settling 1.2
 ***********************************************************************/

#include "wavetable.hpp"

#include <uhd/features/complex_gain_iface.hpp>
#include <uhd/types/tune_request.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/utils/thread.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <chrono>
#include <cmath>
#include <csignal>
#include <fstream>
#include <functional>
#include <iostream>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <thread>

namespace po = boost::program_options;

/***********************************************************************
 * Signal handlers
 **********************************************************************/
static bool stop_signal_called = false;
void sig_int_handler(int)
{
    stop_signal_called = true;
}

/***********************************************************************
 * transmit_worker function
 * A function to be used in a thread for transmitting
 **********************************************************************/
void transmit_worker(std::vector<std::complex<float>>& buff,
    wave_table_class wave_table,
    uhd::tx_streamer::sptr tx_streamer,
    uhd::tx_metadata_t metadata,
    size_t step,
    size_t index,
    int num_channels)
{
    std::vector<std::complex<float>*> buffs(num_channels, &buff.front());

    // send data until the signal handler gets called
    while (not stop_signal_called) {
        // fill the buffer with the waveform
        for (size_t n = 0; n < buff.size(); n++) {
            buff[n] = wave_table(index += step);
        }

        // send the entire contents of the buffer
        tx_streamer->send(buffs, buff.size(), metadata);

        metadata.start_of_burst = false;
        metadata.has_time_spec  = false;
    }

    // send a mini EOB packet
    metadata.end_of_burst = true;
    tx_streamer->send("", 0, metadata);
}

/***********************************************************************
 * Utilities
 **********************************************************************/
//! Change to filename, e.g. from usrp_samples.dat to usrp_samples.00.dat,
//  but only if multiple names are to be generated.
std::string generate_out_filename(
    const std::string& base_fn, size_t n_names, size_t this_name)
{
    if (n_names == 1) {
        return base_fn;
    }

    boost::filesystem::path base_fn_fp(base_fn);
    base_fn_fp.replace_extension(boost::filesystem::path(
        str(boost::format("%02d%s") % this_name % base_fn_fp.extension().string())));
    return base_fn_fp.string();
}

/***********************************************************************
 * recv_to_file function
 **********************************************************************/
template <typename samp_type>
std::vector<std::vector<samp_type>> recv_to_file(uhd::usrp::multi_usrp::sptr usrp,
    const std::string& cpu_format,
    const std::string& wire_format,
    const std::string& file,
    size_t samps_per_buff,
    int num_requested_samples,
    double settling_time,
    std::vector<size_t> rx_channel_nums)
{
    int num_total_samps = 0;
    // create a receive streamer
    uhd::stream_args_t stream_args(cpu_format, wire_format);
    stream_args.channels             = rx_channel_nums;
    uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);

    // Prepare buffers for received samples and metadata
    uhd::rx_metadata_t md;
    std::vector<std::vector<samp_type>> buffs(
        rx_channel_nums.size(), std::vector<samp_type>(samps_per_buff));
    // create a vector of pointers to point to each of the channel buffers
    std::vector<samp_type*> buff_ptrs;
    for (size_t i = 0; i < buffs.size(); i++) {
        buff_ptrs.push_back(&buffs[i].front());
    }

    // Create one ofstream object per channel
    // (use shared_ptr because ofstream is non-copyable)
    std::vector<std::shared_ptr<std::ofstream>> outfiles;
    for (size_t i = 0; i < buffs.size(); i++) {
        const std::string this_filename = generate_out_filename(file, buffs.size(), i);
        outfiles.push_back(std::shared_ptr<std::ofstream>(
            new std::ofstream(this_filename.c_str(), std::ofstream::binary)));
    }
    UHD_ASSERT_THROW(outfiles.size() == buffs.size());
    UHD_ASSERT_THROW(buffs.size() == rx_channel_nums.size());
    bool overflow_message = true;
    // We increase the first timeout to cover for the delay between now + the
    // command time, plus 500ms of buffer. In the loop, we will then reduce the
    // timeout for subsequent receives.
    double timeout = settling_time + 0.5f;

    // setup streaming
    uhd::stream_cmd_t stream_cmd((num_requested_samples == 0)
                                     ? uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS
                                     : uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
    stream_cmd.num_samps  = num_requested_samples;
    stream_cmd.stream_now = false;
    stream_cmd.time_spec  = usrp->get_time_now() + uhd::time_spec_t(settling_time);
    rx_stream->issue_stream_cmd(stream_cmd);

    while (not stop_signal_called
           and (num_requested_samples > num_total_samps or num_requested_samples == 0)) {
        size_t num_rx_samps = rx_stream->recv(buff_ptrs, samps_per_buff, md, timeout);
        timeout             = 0.1f; // small timeout for subsequent recv

        if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT) {
            std::cout << "Timeout while streaming" << std::endl;
            break;
        }
        if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_OVERFLOW) {
            if (overflow_message) {
                overflow_message = false;
                std::cerr
                    << boost::format(
                           "Got an overflow indication. Please consider the following:\n"
                           "  Your write medium must sustain a rate of %fMB/s.\n"
                           "  Dropped samples will not be written to the file.\n"
                           "  Please modify this example for your purposes.\n"
                           "  This message will not appear again.\n")
                           % (usrp->get_rx_rate() * sizeof(samp_type) / 1e6);
            }
            continue;
        }
        if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE) {
            throw std::runtime_error("Receiver error " + md.strerror());
        }

        num_total_samps += num_rx_samps;

        for (size_t i = 0; i < outfiles.size(); i++) {
            outfiles[i]->write(
                (const char*)buff_ptrs[i], num_rx_samps * sizeof(samp_type));
        }
    }

    // Shut down receiver
    stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
    rx_stream->issue_stream_cmd(stream_cmd);

    // Close files
    for (size_t i = 0; i < outfiles.size(); i++) {
        outfiles[i]->close();
    }

    return buffs;
}

/***********************************************************************
 * Template function to get a feature interface for a given channel
 **********************************************************************/
template <typename FeatureInterface>
FeatureInterface& get_feature_interface(uhd::usrp::multi_usrp::sptr usrp, size_t chan)
{
    if (!usrp->get_radio_control(chan).has_feature<FeatureInterface>()) {
        throw std::runtime_error(
            "The requested feature is not available on this device.");
    }
    return usrp->get_radio_control(chan).get_feature<FeatureInterface>();
}

/***********************************************************************
 * Helper function to parse complex number strings like "1.0+0.0j"
 **********************************************************************/
std::complex<double> parse_complex(const std::string& str)
{
    std::istringstream iss(str);
    double real = 0.0, imag = 0.0;

    if (str.find('+') != std::string::npos || str.find('-', 1) != std::string::npos) {
        // Format: "real+imagj" or "real-imagj"
        iss >> real >> imag;
    } else {
        iss >> real;
    }

    return std::complex<double>(real, imag);
}

int UHD_SAFE_MAIN(int argc, char* argv[])
{
    // TX variables to be set by po
    std::string tx_args, tx_gain_list, tx_gain_delay, tx_channels, tx_subdev, wave_type;
    double tx_rate, tx_freq, wave_freq;
    float ampl;

    // RX variables to be set by po
    std::string rx_args, rx_gain_list, rx_gain_delay, rx_channels, rx_subdev, type, file;
    size_t total_num_samps, spb;
    double rx_rate, rx_freq, settling;

    // setup the program options
    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()
        ("help", "help message")
        ("tx-args", po::value<std::string>(&tx_args)->default_value(""), "uhd transmit device address args")
        ("rx-args", po::value<std::string>(&rx_args)->default_value(""), "uhd receive device address args")
        ("tx-subdev", po::value<std::string>(&tx_subdev), "TX subdevice specification")
        ("rx-subdev", po::value<std::string>(&rx_subdev), "RX subdevice specification")
        ("file", po::value<std::string>(&file)->default_value("usrp_samples.dat"), "name of the file to write binary samples to")
        ("nsamps", po::value<size_t>(&total_num_samps)->default_value(0), "total number of samples to receive")
        ("settling", po::value<double>(&settling)->default_value(double(0.2)), "settling time (seconds) before receiving")
        ("spb", po::value<size_t>(&spb)->default_value(0), "samples per buffer, 0 for default")
        ("tx-rate", po::value<double>(&tx_rate), "rate of transmit outgoing samples")
        ("rx-rate", po::value<double>(&rx_rate), "rate of receive incoming samples")
        ("tx-freq", po::value<double>(&tx_freq), "transmit RF center frequency in Hz")
        ("rx-freq", po::value<double>(&rx_freq), "receive RF center frequency in Hz")
        ("ampl", po::value<float>(&ampl)->default_value(float(0.3)), "amplitude of the waveform [0 to 0.7]")
        ("wave-type", po::value<std::string>(&wave_type)->default_value("CONST"), "waveform type (CONST, SQUARE, RAMP, SINE)")
        ("wave-freq", po::value<double>(&wave_freq)->default_value(0), "waveform frequency in Hz")
        ("tx-gain", po::value<std::string>(&tx_gain_list),
            "TX complex gain values (format: 'real+imagj') \
            (\"1.0+0.0j\", \"1.0+0.0j, 1.0+0.0j\")")
        ("rx-gain", po::value<std::string>(&rx_gain_list),
            "RX complex gain values (format: 'real+imagj') \
            (\"1.0+0.0j\", \"1.0+0.0j, 1.0+0.0j\")")
        ("tx-gain-delay", po::value<std::string>(&tx_gain_delay)->default_value("0.0"),
            "time to apply each gain (seconds in the future)")
        ("rx-gain-delay", po::value<std::string>(&rx_gain_delay)->default_value("0.0"),
            "time to apply each gain (seconds in the future)")
        ("tx-channels", po::value<std::string>(&tx_channels)->default_value("0"),
            "comma-separated list of TX channels to apply gain (\"0\", \"1\", \"0,1\", etc)")
        ("rx-channels", po::value<std::string>(&rx_channels)->default_value("0"),
            "comma-separated list of RX channels to apply gain (\"0\", \"1\", \"0,1\", etc)")
        ("verbose", "view streaming debug info")
    ;
    // clang-format on
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    // print the help message
    if (vm.count("help")) {
        std::cout << boost::format("UHD TXRX Complex Gain Example %s") % desc
                  << std::endl;
        return ~0;
    }

    // Parse complex gain values from strings
    std::vector<std::string> tx_gain_strs;
    std::vector<std::string> rx_gain_strs;
    std::vector<std::complex<double>> tx_gain_val;
    std::vector<std::complex<double>> rx_gain_val;
    boost::split(tx_gain_strs, tx_gain_list, boost::is_any_of("\"',"));
    boost::split(rx_gain_strs, rx_gain_list, boost::is_any_of("\"',"));
    try {
        for (const auto& gain_str : tx_gain_strs) {
            if (!gain_str.empty()) {
                tx_gain_val.push_back(parse_complex(gain_str));
            }
        }
        for (const auto& gain_str : rx_gain_strs) {
            if (!gain_str.empty()) {
                rx_gain_val.push_back(parse_complex(gain_str));
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing gain values: " << e.what() << std::endl;
        std::cerr << "Use format like '1.0+0.0j' for complex numbers" << std::endl;
        return EXIT_FAILURE;
    }

    // Parse gain delay
    std::vector<double> rx_gain_delays, tx_gain_delays;
    std::vector<std::string> rx_time_str, tx_time_str;

    if (vm.count("rx-gain-delay")) {
        boost::split(rx_time_str, rx_gain_delay, boost::is_any_of("\"',"));
        for (const auto& time_str : rx_time_str) {
            rx_gain_delays.push_back(std::stod(time_str));
        }
    }

    if (vm.count("tx-gain-delay")) {
        boost::split(tx_time_str, tx_gain_delay, boost::is_any_of("\"',"));
        for (const auto& time_str : tx_time_str) {
            tx_gain_delays.push_back(std::stod(time_str));
        }
    }

    // create a usrp device
    std::cout << std::endl;
    uhd::usrp::multi_usrp::sptr tx_usrp, rx_usrp;
    std::cout << "Creating the usrp device with: " << tx_args << "..." << std::endl;
    tx_usrp = uhd::usrp::multi_usrp::make(tx_args);
    std::cout << "Creating the usrp device with: " << rx_args << "..." << std::endl;
    rx_usrp = uhd::usrp::multi_usrp::make(rx_args);


    // always select the subdevice first, the channel mapping affects the other settings
    if (vm.count("tx-subdev")) {
        tx_usrp->set_tx_subdev_spec(tx_subdev);
    }
    if (vm.count("rx-subdev")) {
        rx_usrp->set_rx_subdev_spec(rx_subdev);
    }
    std::cout << "Using TX device: " << tx_usrp->get_pp_string() << std::endl;
    std::cout << "Using RX device: " << rx_usrp->get_pp_string() << std::endl;

    // Parse channel list
    std::vector<size_t> tx_channel_nums, rx_channel_nums;
    std::vector<std::string> tx_channel_str, rx_channel_str;
    boost::split(tx_channel_str, tx_channels, boost::is_any_of("\"',"));
    boost::split(rx_channel_str, rx_channels, boost::is_any_of("\"',"));
    for (size_t ch = 0; ch < tx_channel_str.size(); ch++) {
        size_t chan = std::stoi(tx_channel_str[ch]);
        if (chan >= tx_usrp->get_tx_num_channels()) {
            throw std::runtime_error("Invalid TX channel(s) specified.");
        } else {
            tx_channel_nums.push_back(chan);
        }
    }
    for (size_t ch = 0; ch < rx_channel_str.size(); ch++) {
        size_t chan = std::stoi(rx_channel_str[ch]);
        if (chan >= rx_usrp->get_rx_num_channels()) {
            throw std::runtime_error("Invalid RX channel(s) specified.");
        } else {
            rx_channel_nums.push_back(chan);
        }
    }

    // set the transmit sample rate
    if (not vm.count("tx-rate")) {
        std::cerr << "Please specify the transmit sample rate with --tx-rate"
                  << std::endl;
        return ~0;
    }

    std::cout << boost::format("Setting TX Rate: %f Msps...") % (tx_rate / 1e6)
              << std::endl;
    tx_usrp->set_tx_rate(tx_rate);
    std::cout << boost::format("Actual TX Rate: %f Msps...")
                     % (tx_usrp->get_tx_rate() / 1e6)
              << std::endl
              << std::endl;


    // set the receive sample rate
    if (not vm.count("rx-rate")) {
        std::cerr << "Please specify the sample rate with --rx-rate" << std::endl;
        return ~0;
    }
    std::cout << boost::format("Setting RX Rate: %f Msps...") % (rx_rate / 1e6)
              << std::endl;
    rx_usrp->set_rx_rate(rx_rate);
    std::cout << boost::format("Actual RX Rate: %f Msps...")
                     % (rx_usrp->get_rx_rate() / 1e6)
              << std::endl
              << std::endl;

    // set the transmit center frequency
    if (not vm.count("tx-freq")) {
        std::cerr << "Please specify the transmit center frequency with --tx-freq"
                  << std::endl;
        return ~0;
    }

    for (size_t chan_idx = 0; chan_idx < tx_channel_nums.size(); chan_idx++) {
        size_t chan = tx_channel_nums[chan_idx];

        std::cout << "Configuring TX Channel " << chan << std::endl;
        std::cout << boost::format("Setting TX Freq: %f MHz...") % (tx_freq / 1e6)
                  << std::endl;
        uhd::tune_request_t tx_tune_request(tx_freq);
        tx_usrp->set_tx_freq(tx_tune_request, chan);
        std::cout << boost::format("Actual TX Freq: %f MHz...")
                         % (tx_usrp->get_tx_freq(chan) / 1e6)
                  << std::endl
                  << std::endl;

        // Applying TX complex gain here
        if (vm.count("tx-gain")) {
            std::cout << "\n--- Applying gain on channel " << chan << " ---" << std::endl;
            size_t tx_gain_idx = (chan_idx < tx_gain_val.size()) ? chan_idx : 0;
            double gain_delay =
                (chan_idx < tx_gain_delays.size()) ? tx_gain_delays[chan_idx] : 0.0;
            if (vm.count("verbose")) {
                if (gain_delay < 0.5)
                    std::cout << "TX Gain will likely be applied ASAP" << std::endl;
            }
            try {
                // Get TX complex gain interface for this channel
                auto& tx_cgain =
                    get_feature_interface<uhd::features::tx_complex_gain_iface>(
                        tx_usrp, chan);

                // Set TX gain for this channel (use channel index for array access)
                // Channel to apply gain needs to be translated to block channel
                tx_cgain.set_gain_coeff(tx_gain_val[tx_gain_idx],
                    tx_usrp->get_tx_radio_channel(chan),
                    tx_usrp->get_time_now() + gain_delay);
                std::cout << "Set TX complex gain to " << tx_gain_val[tx_gain_idx]
                          << " on channel " << chan << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(10));

                // Get current TX gain
                auto current_tx_gain =
                    tx_cgain.get_gain_coeff(tx_usrp->get_tx_radio_channel(chan));
                std::cout << "Current TX complex gain is " << current_tx_gain
                          << std::endl;

            } catch (const std::exception& e) {
                std::cout << "TX Complex Gain not applied on channel " << chan << ": "
                          << e.what() << std::endl;
                return EXIT_FAILURE;
            }
        }
    }

    // set the receive center frequency
    if (not vm.count("rx-freq")) {
        std::cerr << "Please specify the center frequency with --rx-freq" << std::endl;
        return ~0;
    }

    for (size_t chan_idx = 0; chan_idx < rx_channel_nums.size(); chan_idx++) {
        size_t chan = rx_channel_nums[chan_idx];
        std::cout << boost::format("Setting RX Freq: %f MHz...") % (rx_freq / 1e6)
                  << std::endl;
        uhd::tune_request_t rx_tune_request(rx_freq);
        rx_usrp->set_rx_freq(rx_tune_request, chan);
        std::cout << boost::format("Actual RX Freq: %f MHz...")
                         % (rx_usrp->get_rx_freq(chan) / 1e6)
                  << std::endl
                  << std::endl;

        if (vm.count("rx-gain")) {
            std::cout << "\n--- Applying gain on channel " << chan << " ---" << std::endl;
            size_t rx_gain_idx = (chan_idx < rx_gain_val.size()) ? chan_idx : 0;
            double gain_delay =
                (chan_idx < rx_gain_delays.size()) ? rx_gain_delays[chan_idx] : 0.0;
            if (vm.count("verbose")) {
                if (gain_delay < 1.0)
                    std::cout << "RX Gain will likely be applied ASAP" << std::endl;
            }

            // Applying RX complex gain here
            try {
                // Get RX complex gain interface for this channel
                auto& rx_cgain =
                    get_feature_interface<uhd::features::rx_complex_gain_iface>(
                        rx_usrp, chan);

                // Set RX gain for this channel (use channel index for array access)
                // Channel to apply gain needs to be translated to block channel
                rx_cgain.set_gain_coeff(rx_gain_val[rx_gain_idx],
                    rx_usrp->get_rx_radio_channel(chan),
                    rx_usrp->get_time_now() + gain_delay);
                std::cout << "Set RX complex gain to " << rx_gain_val[rx_gain_idx]
                          << " on channel " << chan << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(10));

                // Get current RX gain
                auto current_rx_gain =
                    rx_cgain.get_gain_coeff(rx_usrp->get_rx_radio_channel(chan));
                std::cout << "Current RX complex gain is " << current_rx_gain
                          << std::endl;

            } catch (const std::exception& e) {
                std::cout << "RX Complex Gain not applied on channel " << chan << ": "
                          << e.what() << std::endl;
                return EXIT_FAILURE;
            }
        }
    }

    // Align times in the RX USRP (the TX USRP does not require time-syncing)
    if (rx_usrp->get_num_mboards() > 1) {
        rx_usrp->set_time_unknown_pps(uhd::time_spec_t(0.0));
    }

    // for the const wave, set the wave freq for small samples per period
    if (wave_freq == 0 and wave_type == "CONST") {
        wave_freq = tx_usrp->get_tx_rate() / 2;
    }

    // error when the waveform is not possible to generate
    if (std::abs(wave_freq) > tx_usrp->get_tx_rate() / 2) {
        throw std::runtime_error("wave freq out of Nyquist zone");
    }
    if (tx_usrp->get_tx_rate() / std::abs(wave_freq) > wave_table_len / 2) {
        throw std::runtime_error("wave freq too small for table");
    }

    // pre-compute the waveform values
    const wave_table_class wave_table(wave_type, ampl);
    const size_t step = std::lround(wave_freq / tx_usrp->get_tx_rate() * wave_table_len);
    size_t index      = 0;

    // create a transmit streamer
    // linearly map channels (index0 = channel0, index1 = channel1, ...)
    uhd::stream_args_t stream_args("fc32", "sc16");
    stream_args.channels             = tx_channel_nums;
    uhd::tx_streamer::sptr tx_stream = tx_usrp->get_tx_stream(stream_args);

    // allocate a buffer which we re-use for each channel
    if (spb == 0)
        spb = tx_stream->get_max_num_samps() * 10;
    std::vector<std::complex<float>> buff(spb);

    // setup the metadata flags
    uhd::tx_metadata_t md;
    md.start_of_burst = true;
    md.end_of_burst   = false;
    md.has_time_spec  = true;
    md.time_spec = uhd::time_spec_t(0.5); // give us 0.5 seconds to fill the tx buffers

    std::vector<std::string> tx_sensor_names, rx_sensor_names;
    tx_sensor_names = tx_usrp->get_tx_sensor_names(0);
    if (std::find(tx_sensor_names.begin(), tx_sensor_names.end(), "lo_locked")
        != tx_sensor_names.end()) {
        uhd::sensor_value_t lo_locked = tx_usrp->get_tx_sensor("lo_locked", 0);
        std::cout << boost::format("Checking TX: %s ...") % lo_locked.to_pp_string()
                  << std::endl;
        UHD_ASSERT_THROW(lo_locked.to_bool());
    }
    rx_sensor_names = rx_usrp->get_rx_sensor_names(0);
    if (std::find(rx_sensor_names.begin(), rx_sensor_names.end(), "lo_locked")
        != rx_sensor_names.end()) {
        uhd::sensor_value_t lo_locked = rx_usrp->get_rx_sensor("lo_locked", 0);
        std::cout << boost::format("Checking RX: %s ...") % lo_locked.to_pp_string()
                  << std::endl;
        UHD_ASSERT_THROW(lo_locked.to_bool());
    }

    if (total_num_samps == 0) {
        std::signal(SIGINT, &sig_int_handler);
        std::cout << "Press Ctrl + C to stop streaming..." << std::endl;
    }

    // reset usrp time to prepare for transmit/receive
    std::cout << boost::format("Setting device timestamp to 0...") << std::endl;
    tx_usrp->set_time_now(uhd::time_spec_t(0.0));

    // start transmit worker thread
    std::thread transmit_thread([&]() {
        transmit_worker(
            buff, wave_table, tx_stream, md, step, index, tx_channel_nums.size());
    });

    std::vector<std::vector<std::complex<float>>> recv_buffs;
    recv_buffs = recv_to_file<std::complex<float>>(
        rx_usrp, "fc32", "sc16", file, spb, total_num_samps, settling, rx_channel_nums);

    if (vm.count("verbose")) {
        for (size_t ch = 0; ch < recv_buffs.size(); ch++) {
            std::cout << "Received samples on channel " << rx_channel_nums[ch] << ": ";
            for (size_t n = 0;
                 n < std::min(static_cast<size_t>(100), recv_buffs[ch].size());
                 n++) {
                std::cout << recv_buffs[ch][n] << " ";
            }
            std::cout << std::endl;
        }
    }

    // clean up transmit worker
    stop_signal_called = true;
    transmit_thread.join();

    // finished
    std::cout << std::endl << "Done!" << std::endl << std::endl;
    return EXIT_SUCCESS;
}
