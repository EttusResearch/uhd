//
// Copyright 2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

// Example UHD/RFNoC application: Connect an rx radio to a tx radio and
// run a loopback.

#include <uhd/device3.hpp>
#include <uhd/rfnoc/radio_ctrl.hpp>
#include <uhd/utils/math.hpp>
#include <uhd/utils/safe_main.hpp>
#include <boost/program_options.hpp>
#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

namespace po = boost::program_options;
using uhd::rfnoc::radio_ctrl;

/****************************************************************************
 * SIGINT handling
 ***************************************************************************/
static bool stop_signal_called = false;
void sig_int_handler(int)
{
    stop_signal_called = true;
}

/****************************************************************************
 * main
 ***************************************************************************/
int UHD_SAFE_MAIN(int argc, char* argv[])
{
    // variables to be set by po
    std::string args, rx_args, tx_args, rx_ant, tx_ant, rx_blockid, tx_blockid, ref;
    size_t total_num_samps, spp, rx_chan, tx_chan, tx_delay;
    double rate, rx_freq, tx_freq, rx_gain, tx_gain, bw, total_time, setup_time;
    bool rx_timestamps;

    // setup the program options
    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""), "UHD device address args")
        ("rx_args", po::value<std::string>(&rx_args)->default_value(""), "Block args for the receive radio")
        ("tx_args", po::value<std::string>(&tx_args)->default_value(""), "Block args for the transmit radio")
        ("spp", po::value<size_t>(&spp)->default_value(0), "Samples per packet (reduce for lower latency)")
        ("rx-freq", po::value<double>(&rx_freq)->default_value(0.0), "Rx RF center frequency in Hz")
        ("tx-freq", po::value<double>(&tx_freq)->default_value(0.0), "Tx RF center frequency in Hz")
        ("rx-gain", po::value<double>(&rx_gain)->default_value(0.0), "Rx RF center gain in Hz")
        ("tx-gain", po::value<double>(&tx_gain)->default_value(0.0), "Tx RF center gain in Hz")
        ("rx-ant", po::value<std::string>(&rx_ant), "Receive antenna selection")
        ("tx-ant", po::value<std::string>(&tx_ant), "Transmit antenna selection")
        ("rx-blockid", po::value<std::string>(&rx_blockid)->default_value("0/Radio_0"), "Receive radio block ID")
        ("tx-blockid", po::value<std::string>(&tx_blockid)->default_value("0/Radio_1"), "Transmit radio block ID")
        ("rx-chan", po::value<size_t>(&rx_chan)->default_value(0), "Channel index on receive radio")
        ("tx-chan", po::value<size_t>(&tx_chan)->default_value(0), "Channel index on transmit radio")
        ("rx-timestamps", po::value<bool>(&rx_timestamps)->default_value(false), "Set timestamps on RX")
        ("tx-delay", po::value<size_t>(&tx_delay)->default_value(10), "Ticks delay")
        ("setup", po::value<double>(&setup_time)->default_value(1.0), "seconds of setup time")
        ("nsamps", po::value<size_t>(&total_num_samps)->default_value(0), "total number of samples to receive")
        ("rate", po::value<double>(&rate)->default_value(0.0), "Sampling rate")
        ("duration", po::value<double>(&total_time)->default_value(0), "total number of seconds to receive")
        ("int-n", "Tune USRP with integer-N tuning")
        ("bw", po::value<double>(&bw), "Analog frontend filter bandwidth in Hz (Rx and Tx)")
        ("ref", po::value<std::string>(&ref)->default_value("internal"), "reference source (internal, external, mimo)")
    ;
    // clang-format on
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    // print the help message
    if (vm.count("help")) {
        std::cout << boost::format("RFNoC: Radio loopback test %s") % desc << std::endl;
        std::cout
            << std::endl
            << "This application streams data from one radio to another using RFNoC.\n"
            << std::endl;
        return ~0;
    }

    // Create a device session
    std::cout << std::endl;
    std::cout << boost::format("Creating the usrp device with: %s...") % args
              << std::endl;
    auto dev = boost::dynamic_pointer_cast<uhd::device3>(uhd::device::make(args));
    if (not dev) {
        std::cout << "Error: Could not find an RFNoC-compatible device." << std::endl;
        return EXIT_FAILURE;
    }

    // Access block controllers
    if (not dev->has_block<uhd::rfnoc::radio_ctrl>(rx_blockid)
        or not dev->has_block<uhd::rfnoc::radio_ctrl>(tx_blockid)) {
        std::cout << "Error: Could not access at least one of these blocks:\n"
                  << "- " << rx_blockid << "- " << tx_blockid << std::endl;
        std::cout << "Please confirm these blocks are actually available on the current "
                     "loaded device."
                  << std::endl;
        return EXIT_FAILURE;
    }
    auto rx_radio_ctrl = dev->get_block_ctrl<radio_ctrl>(rx_blockid);
    auto tx_radio_ctrl = dev->get_block_ctrl<radio_ctrl>(tx_blockid);

    // Configure Rx radio
    std::cout << "Configuring Rx radio..." << std::endl;
    rx_radio_ctrl->set_args(rx_args);

    // Lock mboard clocks 
    if (vm.count("ref")) {
        rx_radio_ctrl->set_clock_source(ref);
    }
    
    if (spp) {
        rx_radio_ctrl->set_arg<int>("spp", spp, rx_chan);
    }

    std::cout << "Setting Rx rate: " << (rate / 1e6) << " Msps" << std::endl;
    double actual_rx_rate = rx_radio_ctrl->set_rate(rate);
    std::cout << "Actual  Rx rate: " << (actual_rx_rate / 1e6) << " Msps" << std::endl;

    std::cout << "Setting Rx frequency: " << (rx_freq / 1e6) << " MHz." << std::endl;
    std::cout << "Actual  Rx frequency: "
              << (rx_radio_ctrl->set_rx_frequency(rx_freq, rx_chan) / 1e6) << " MHz."
              << std::endl;
    
    if (rx_gain) {
        std::cout << "Setting Rx gain: " << (rx_gain) << " dB." << std::endl;
        std::cout << "Actual  Rx gain: " << (rx_radio_ctrl->set_rx_gain(rx_gain, rx_chan))
                  << " dB." << std::endl;
    }
    
    if (not rx_ant.empty()) {
        std::cout << "Setting Rx antenna: " << (rx_ant) << "." << std::endl;
        rx_radio_ctrl->set_rx_antenna(rx_ant, rx_chan);
        std::cout << "Actual  Rx antenna: " << rx_radio_ctrl->get_rx_antenna(rx_chan)
                  << "." << std::endl;
    }
    
    if (!rx_timestamps) {
        std::cout << "Disabling timestamps on RX... (direct loopback, may underrun)"
                  << std::endl;
    }
    rx_radio_ctrl->enable_rx_timestamps(rx_timestamps, 0);

    // Configure Tx radio
    std::cout << "Configuring Tx radio..." << std::endl;
    tx_radio_ctrl->set_args(tx_args);

    // Lock mboard clocks 
    if (vm.count("ref")) {
        tx_radio_ctrl->set_clock_source(ref);
    }
    
    std::cout << "Setting Tx rate: " << (rate / 1e6) << " Msps" << std::endl;
    double actual_tx_rate = tx_radio_ctrl->set_rate(rate);
    std::cout << "Actual  Tx rate: " << (actual_tx_rate / 1e6) << " Msps" << std::endl;
    
    std::cout << "Setting Tx frequency: " << (tx_freq / 1e6) << " MHz." << std::endl;
    std::cout << "Actual  Tx frequency: "
              << (tx_radio_ctrl->set_tx_frequency(tx_freq, tx_chan) / 1e6) << " MHz."
              << std::endl;
    
    if (tx_gain) {
        std::cout << "Setting Tx gain: " << (tx_gain) << " dB." << std::endl;
        std::cout << "Actual  Tx gain: " << (tx_radio_ctrl->set_tx_gain(tx_gain, tx_chan))
                  << " dB." << std::endl;
    }
    
    if (not tx_ant.empty()) {
        std::cout << "Setting Tx antenna: " << (tx_ant) << "." << std::endl;
        tx_radio_ctrl->set_tx_antenna(tx_ant, tx_chan);
        std::cout << "Actual  Tx antenna: " << tx_radio_ctrl->get_tx_antenna(tx_chan)
                  << "." << std::endl;
    }

    // Compare rates
    if (not uhd::math::frequencies_are_equal(actual_rx_rate, actual_tx_rate)) {
        std::cout
            << "Error: Failed to set receive and transmit radios to same sampling rate!"
            << std::endl;
        return EXIT_FAILURE;
    }
    // Create graph and connect blocks
    uhd::rfnoc::graph::sptr graph = dev->create_graph("radio_loopback");
    std::cout << "Connecting radios..." << std::endl;
    try {
        graph->connect(rx_blockid, rx_chan, tx_blockid, tx_chan);
    } catch (const uhd::runtime_error& ex) {
        std::cout << "Error connecting blocks: " << std::endl;
        std::cout << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
    tx_radio_ctrl->set_tx_streamer(true, tx_chan);
    rx_radio_ctrl->set_rx_streamer(true, rx_chan);

    // Allow for some setup time
    std::this_thread::sleep_for(std::chrono::milliseconds(int64_t(setup_time * 1000)));

    // Arm SIGINT handler
    std::signal(SIGINT, &sig_int_handler);

    // Calculate timeout and set timers
    if (total_time == 0 and total_num_samps > 0) {
        const double buffer_time = 1.0; // seconds
        total_time               = (1.0 / rate) * total_num_samps + buffer_time;
    }

    // Start streaming
    uhd::stream_cmd_t stream_cmd((total_num_samps == 0)
                                     ? uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS
                                     : uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
    stream_cmd.num_samps  = size_t(total_num_samps);
    stream_cmd.stream_now = true;
    stream_cmd.time_spec  = uhd::time_spec_t();
    std::cout << "Issuing start stream cmd..." << std::endl;
    rx_radio_ctrl->issue_stream_cmd(stream_cmd, rx_chan);
    std::cout << "Wait..." << std::endl;

    // Wait until we can exit
    while (not stop_signal_called) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        // FIXME honour --duration
    }

    // Stop radio
    stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
    std::cout << "Issuing stop stream cmd..." << std::endl;
    rx_radio_ctrl->issue_stream_cmd(stream_cmd);
    std::cout << "Done" << std::endl;

    return EXIT_SUCCESS;
}
