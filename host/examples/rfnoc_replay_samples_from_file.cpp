//
// Copyright 2020 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
//
// Description:
//
// This example demonstrates using the Replay block to replay data from a file.
// It streams the file data to the Replay block, where it is recorded, then it
// is played back to the radio.

#include <uhd/rfnoc/block_id.hpp>
#include <uhd/rfnoc/duc_block_control.hpp>
#include <uhd/rfnoc/mb_controller.hpp>
#include <uhd/rfnoc/radio_control.hpp>
#include <uhd/rfnoc/replay_block_control.hpp>
#include <uhd/rfnoc_graph.hpp>
#include <uhd/types/tune_request.hpp>
#include <uhd/utils/graph_utils.hpp>
#include <uhd/utils/math.hpp>
#include <uhd/utils/safe_main.hpp>
#include <boost/program_options.hpp>
#include <chrono>
#include <csignal>
#include <fstream>
#include <iostream>
#include <thread>

namespace po = boost::program_options;

using std::cout;
using std::endl;
using namespace std::chrono_literals;

///////////////////////////////////////////////////////////////////////////////

static volatile bool stop_signal_called = false;

// Ctrl+C handler
void sig_int_handler(int)
{
    stop_signal_called = true;
}


int UHD_SAFE_MAIN(int argc, char* argv[])
{
    // We use sc16 in this example, but the replay block only uses 64-bit words
    // and is not aware of the CPU or wire format.
    std::string wire_format("sc16");
    std::string cpu_format("sc16");

    /************************************************************************
     * Set up the program options
     ***********************************************************************/
    std::string args, tx_args, file, ant, ref;
    double rate, freq, gain, bw;
    size_t radio_id, radio_chan, replay_id, replay_chan, nsamps;

    po::options_description desc("Allowed Options");
    // clang-format off
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""), "multi uhd device address args")
        ("tx_args", po::value<std::string>(&tx_args), "Block args for the transmit radio")
        ("radio_id", po::value<size_t>(&radio_id)->default_value(0), "radio block to use (e.g., 0 or 1).")
        ("radio_chan", po::value<size_t>(&radio_chan)->default_value(0), "radio channel to use")
        ("replay_id", po::value<size_t>(&replay_id)->default_value(0), "replay block to use (e.g., 0 or 1)")
        ("replay_chan", po::value<size_t>(&replay_chan)->default_value(0), "replay channel to use")
        ("nsamps", po::value<size_t>(&nsamps)->default_value(0), "number of samples to play (0 for infinite)")
        ("file", po::value<std::string>(&file)->default_value("usrp_samples.dat"), "name of the file to read binary samples from")
        ("freq", po::value<double>(&freq), "RF center frequency in Hz")
        ("rate", po::value<double>(&rate), "rate of radio block")
        ("gain", po::value<double>(&gain), "gain for the RF chain")
        ("ant", po::value<std::string>(&ant), "antenna selection")
        ("bw", po::value<double>(&bw), "analog front-end filter bandwidth in Hz")
        ("ref", po::value<std::string>(&ref)->default_value("internal"), "reference source (internal, external, mimo)")
    ;
    // clang-format on
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    // Print help message
    if (vm.count("help")) {
        cout << "UHD/RFNoC Replay samples from file " << desc << endl;
        cout << "This application uses the Replay block to playback data from a file to "
                "a radio"
             << endl
             << endl;
        return EXIT_FAILURE;
    }


    /************************************************************************
     * Create device and block controls
     ***********************************************************************/
    std::cout << std::endl;
    std::cout << "Creating the RFNoC graph with args: " << args << "..." << std::endl;
    auto graph = uhd::rfnoc::rfnoc_graph::make(args);

    // Create handle for radio object
    uhd::rfnoc::block_id_t radio_ctrl_id(0, "Radio", radio_id);
    auto radio_ctrl = graph->get_block<uhd::rfnoc::radio_control>(radio_ctrl_id);

    // Check if the replay block exists on this device
    uhd::rfnoc::block_id_t replay_ctrl_id(0, "Replay", replay_id);
    if (!graph->has_block(replay_ctrl_id)) {
        cout << "Unable to find block \"" << replay_ctrl_id << "\"" << endl;
        return EXIT_FAILURE;
    }
    auto replay_ctrl = graph->get_block<uhd::rfnoc::replay_block_control>(replay_ctrl_id);

    // Connect replay to radio
    auto edges = uhd::rfnoc::connect_through_blocks(
        graph, replay_ctrl_id, replay_chan, radio_ctrl_id, radio_chan);

    // Check for a DUC connected to the radio
    uhd::rfnoc::duc_block_control::sptr duc_ctrl;
    size_t duc_chan = 0;
    for (auto& edge : edges) {
        auto blockid = uhd::rfnoc::block_id_t(edge.dst_blockid);
        if (blockid.match("DUC")) {
            duc_ctrl = graph->get_block<uhd::rfnoc::duc_block_control>(blockid);
            duc_chan = edge.dst_port;
            break;
        }
    }

    // Report blocks
    std::cout << "Using Radio Block:  " << radio_ctrl_id << ", channel " << radio_chan
              << std::endl;
    std::cout << "Using Replay Block: " << replay_ctrl_id << ", channel " << replay_chan
              << std::endl;
    if (duc_ctrl) {
        std::cout << "Using DUC Block:    " << duc_ctrl->get_block_id() << ", channel "
                  << duc_chan << std::endl;
    }


    /************************************************************************
     * Set up streamer to Replay block and commit graph
     ***********************************************************************/
    uhd::device_addr_t streamer_args;
    uhd::stream_args_t stream_args(cpu_format, wire_format);
    uhd::tx_streamer::sptr tx_stream;
    uhd::tx_metadata_t tx_md;

    stream_args.args = streamer_args;
    tx_stream        = graph->create_tx_streamer(1, stream_args);
    graph->connect(tx_stream, 0, replay_ctrl->get_block_id(), replay_chan);
    graph->commit();

    /************************************************************************
     * Set up radio
     ***********************************************************************/
    // Set clock reference
    if (vm.count("ref")) {
        // Lock mboard clocks
        for (size_t i = 0; i < graph->get_num_mboards(); ++i) {
            graph->get_mb_controller(i)->set_clock_source(ref);
        }
    }

    // Apply any radio arguments provided
    if (vm.count("tx_args")) {
        radio_ctrl->set_tx_tune_args(tx_args, radio_chan);
    }

    // Set the center frequency
    if (!vm.count("freq")) {
        std::cerr << "Please specify the center frequency with --freq" << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << std::fixed;
    std::cout << "Requesting TX Freq: " << (freq / 1e6) << " MHz..." << std::endl;
    radio_ctrl->set_tx_frequency(freq, radio_chan);
    std::cout << "Actual TX Freq: " << (radio_ctrl->get_tx_frequency(radio_chan) / 1e6)
              << " MHz..." << std::endl
              << std::endl;
    std::cout << std::resetiosflags(std::ios::fixed);

    // Set the sample rate
    if (vm.count("rate")) {
        std::cout << std::fixed;
        std::cout << "Requesting TX Rate: " << (rate / 1e6) << " Msps..." << std::endl;
        if (duc_ctrl) {
            std::cout << "DUC block found." << std::endl;
            duc_ctrl->set_input_rate(rate, duc_chan);
            std::cout << "  Interpolation value is "
                      << duc_ctrl->get_property<int>("interp", duc_chan) << std::endl;
            rate = duc_ctrl->get_input_rate(duc_chan);
        } else {
            rate = radio_ctrl->set_rate(rate);
        }
        std::cout << "Actual TX Rate: " << (rate / 1e6) << " Msps..." << std::endl
                  << std::endl;
        std::cout << std::resetiosflags(std::ios::fixed);
    }

    // Set the RF gain
    if (vm.count("gain")) {
        std::cout << std::fixed;
        std::cout << "Requesting TX Gain: " << gain << " dB..." << std::endl;
        radio_ctrl->set_tx_gain(gain, radio_chan);
        std::cout << "Actual TX Gain: " << radio_ctrl->get_tx_gain(radio_chan) << " dB..."
                  << std::endl
                  << std::endl;
        std::cout << std::resetiosflags(std::ios::fixed);
    }

    // Set the analog front-end filter bandwidth
    if (vm.count("bw")) {
        std::cout << std::fixed;
        std::cout << "Requesting TX Bandwidth: " << (bw / 1e6) << " MHz..." << std::endl;
        radio_ctrl->set_tx_bandwidth(bw, radio_chan);
        std::cout << "Actual TX Bandwidth: "
                  << (radio_ctrl->get_tx_bandwidth(radio_chan) / 1e6) << " MHz..."
                  << std::endl
                  << std::endl;
        std::cout << std::resetiosflags(std::ios::fixed);
    }

    // Set the antenna
    if (vm.count("ant")) {
        radio_ctrl->set_tx_antenna(ant, radio_chan);
    }

    // Allow for some setup time
    std::this_thread::sleep_for(std::chrono::milliseconds(200));



    /************************************************************************
     * Read the data to replay
     ***********************************************************************/
    // Constants related to the Replay block
    const size_t replay_word_size =
        replay_ctrl->get_word_size(); // Size of words used by replay block
    const size_t sample_size = 4; // Complex signed 16-bit is 32 bits per sample


    // Open the file
    std::ifstream infile(file.c_str(), std::ifstream::binary);
    if (!infile.is_open()) {
        std::cerr << "Could not open specified file" << std::endl;
        return EXIT_FAILURE;
    }

    // Get the file size
    infile.seekg(0, std::ios::end);
    size_t file_size = infile.tellg();
    infile.seekg(0, std::ios::beg);

    // Calculate the number of 64-bit words and samples to replay
    size_t words_to_replay   = file_size / replay_word_size;
    size_t samples_to_replay = file_size / sample_size;

    // Create buffer
    std::vector<char> tx_buffer(samples_to_replay * sample_size);
    char* tx_buf_ptr = &tx_buffer[0];

    // Read file into buffer, rounded down to number of words
    infile.read(tx_buf_ptr, samples_to_replay * sample_size);
    infile.close();


    /************************************************************************
     * Configure replay block
     ***********************************************************************/
    // Configure a buffer in the on-board memory at address 0 that's equal in
    // size to the file we want to play back (rounded down to a multiple of
    // 64-bit words). Note that it is allowed to playback a different size or
    // location from what was recorded.
    uint32_t replay_buff_addr = 0;
    uint32_t replay_buff_size = samples_to_replay * sample_size;
    replay_ctrl->record(replay_buff_addr, replay_buff_size, replay_chan);

    // Display replay configuration
    cout << "Replay file size:     " << replay_buff_size << " bytes (" << words_to_replay
         << " qwords, " << samples_to_replay << " samples)" << endl;

    cout << "Record base address:  0x" << std::hex
         << replay_ctrl->get_record_offset(replay_chan) << std::dec << endl;
    cout << "Record buffer size:   " << replay_ctrl->get_record_size(replay_chan)
         << " bytes" << endl;
    cout << "Record fullness:      " << replay_ctrl->get_record_fullness(replay_chan)
         << " bytes" << endl
         << endl;

    // Restart record buffer repeatedly until no new data appears on the Replay
    // block's input. This will flush any data that was buffered on the input.
    uint32_t fullness;
    cout << "Emptying record buffer..." << endl;
    do {
        replay_ctrl->record_restart(replay_chan);

        // Make sure the record buffer doesn't start to fill again
        auto start_time = std::chrono::steady_clock::now();
        do {
            fullness = replay_ctrl->get_record_fullness(replay_chan);
            if (fullness != 0)
                break;
        } while (start_time + 250ms > std::chrono::steady_clock::now());
    } while (fullness);
    cout << "Record fullness:      " << replay_ctrl->get_record_fullness(replay_chan)
         << " bytes" << endl
         << endl;

    /************************************************************************
     * Send data to replay (== record the data)
     ***********************************************************************/
    cout << "Sending data to be recorded..." << endl;
    tx_md.start_of_burst = true;
    tx_md.end_of_burst   = true;
    // We use a very big timeout here, any network buffering issue etc. is not
    // a problem for this application, and we want to upload all the data in one
    // send() call.
    size_t num_tx_samps = tx_stream->send(tx_buf_ptr, samples_to_replay, tx_md, 5.0);
    if (num_tx_samps != samples_to_replay) {
        cout << "ERROR: Unable to send " << samples_to_replay << " samples (sent "
             << num_tx_samps << ")" << endl;
        return EXIT_FAILURE;
    }

    /************************************************************************
     * Wait for data to be stored in on-board memory
     ***********************************************************************/
    cout << "Waiting for recording to complete..." << endl;
    while (replay_ctrl->get_record_fullness(replay_chan) < replay_buff_size) {
        std::this_thread::sleep_for(50ms);
    }
    cout << "Record fullness:      " << replay_ctrl->get_record_fullness(replay_chan)
         << " bytes" << endl
         << endl;


    /************************************************************************
     * Start replay of data
     ***********************************************************************/
    if (nsamps <= 0) {
        // Replay the entire buffer over and over
        const bool repeat = true;
        cout << "Issuing replay command for " << samples_to_replay
             << " samps in continuous mode..." << endl;
        uhd::time_spec_t time_spec = uhd::time_spec_t(0.0);
        replay_ctrl->play(
            replay_buff_addr, replay_buff_size, replay_chan, time_spec, repeat);
        /** Wait until user says to stop **/
        // Setup SIGINT handler (Ctrl+C)
        std::signal(SIGINT, &sig_int_handler);
        cout << "Replaying data (Press Ctrl+C to stop)..." << endl;
        while (not stop_signal_called) {
            std::this_thread::sleep_for(100ms);
        }
        // Remove SIGINT handler
        std::signal(SIGINT, SIG_DFL);
        cout << endl << "Stopping replay..." << endl;
        replay_ctrl->stop(replay_chan);
        std::cout << "Letting device settle..." << std::endl;
        std::this_thread::sleep_for(1s);
    } else {
        // Replay nsamps, wrapping back to the start of the buffer if nsamps is
        // larger than the buffer size.
        replay_ctrl->config_play(replay_buff_addr, replay_buff_size, replay_chan);
        uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
        stream_cmd.num_samps = nsamps;
        cout << "Issuing replay command for " << nsamps << " samps..." << endl;
        stream_cmd.stream_now = true;
        replay_ctrl->issue_stream_cmd(stream_cmd, replay_chan);
        std::cout << "Waiting until replay buffer is clear..." << std::endl;
        const double stream_duration = static_cast<double>(nsamps) / rate;
        std::this_thread::sleep_for(
            std::chrono::milliseconds(static_cast<int64_t>(stream_duration * 1000))
            + 500ms); // Slop factor
    }

    return EXIT_SUCCESS;
}
