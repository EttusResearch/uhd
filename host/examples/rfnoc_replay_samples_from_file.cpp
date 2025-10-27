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
    // clang-format off
    const std::string program_doc =
        "usage: rfnoc_replay_samples_from_file [-h] [--args ARGS] -f FREQ [-r RATE]\n"
        "                                           [--tx-args TX_ARGS]\n"
        "                                           [--radio-id RADIO_ID]\n"
        "                                           [--radio-chan RADIO_CHAN]\n"
        "                                           [--replay-id REPLAY_ID]\n"
        "                                           [--replay-chan REPLAY_CHAN]\n"
        "                                           [--nsamps NSAMPS] [--file FILE]\n"
        "                                           [--lo-offset LO_OFFSET]\n"
        "                                           [--gain GAIN] [--ant ANT]\n"
        "                                           [--bw BW]\n"
        "                                           [--ref {internal,external,mimo,gpsdo}]\n"
        "                                           [--dot]"
        "\n\n"
        "This example program demonstrates how to use the RFNoC C++ API\n"
        "to create an RFNoC graph involving an RFNoC Replay block for playback of\n"
        "baseband samples through the radio.\n"
        "The program loads sample data into the Replay block, configures the RF\n"
        "chain, and plays back the data through the radio. Playback can be\n"
        "continuous or for a specified number of samples.\n"
        "\n"
        "Notes:\n"
        "  - The Replay block is included in most default FPGA images for\n"
        "    RFNoC-capable USRP devices, so custom FPGA builds are usually not\n"
        "    required to use this example program.\n"
        "  - The USRP may round the requested transmit sample rate to the nearest\n"
        "    supported value. For accurate playback, please ensure that the sample\n"
        "    rate of the input data in the file matches the USRP sample rate.\n"
        "    If supported by your USRP device model, you could also change the\n"
        "    master clock rate to an integer multiple of the desired sample rate\n"
        "    to achieve precise sampling. For available master clock rates for a\n"
        "    specific USRP device model, see the UHD manual.\n"
        "\n"
        "Key features:\n"
        "- Loads baseband samples from a binary file and uploads them to the\n"
        "  RFNoC Replay block for playback through the radio.\n"
        "- Dynamically builds and configures an RFNoC graph connecting the Replay\n"
        "  block to the radio block, with automatic integration of a DUC block if\n"
        "  present.\n"
        "- Supports flexible selection of Replay and radio blocks and channels\n"
        "  via program options.\n"
        "- Allows configuration of key transmission parameters, including\n"
        "  frequency, sample rate, gain, bandwidth, LO offset, and antenna.\n"
        "- Enables continuous or finite playback of samples, as specified by the\n"
        "  user.\n"
        "\n"
        "Usage examples:\n"
        "1. Replay a signal from a file on default radio and using default replay\n"
        "   block.\n"
        "     rfnoc_replay_samples_from_file --args \"addr=192.168.10.2\"\n"
        "                                    --freq 2.4e09 --rate 10e06\n"
        "                                    --file usrp_samples.dat\n"
        "2. Replay a signal from a file explicitly selecting and configuring the\n"
        "   radio and replay block:\n"
        "     rfnoc_replay_samples_from_file --args \"addr=192.168.10.2\"\n"
        "                                    --freq 2.4e09 --rate 10e06\n"
        "                                    --file usrp_samples.dat\n"
        "                                    --radio-id 1 --radio-chan 0\n"
        "                                    --replay-id 0 --replay-chan 0\n"
        "3. Replay an LTE signal at a sampling rate of 30.72MSps with master\n"
        "   clock rate changed to an integer multiple of the sampling rate:\n"
        "     rfnoc_replay_samples_from_file --args \"addr=192.168.10.2,master_clock_rate=184.32e6\"\n"
        "                                    --freq 2.4e09 --rate 30.72e06\n"
        "                                    --file LTE_30d72MSps_sc16.dat\n";
    // clang-format on
    // We use sc16 in this example, but the replay block only uses 64-bit words
    // and is not aware of the CPU or wire format.
    std::string wire_format("sc16");
    std::string cpu_format("sc16");

    /************************************************************************
     * Set up the program options
     ***********************************************************************/
    std::string args, tx_args, file, ant, ref;
    double rate, freq, gain, bw, lo_offset;
    size_t radio_id, radio_chan, replay_id, replay_chan, nsamps;

    po::options_description desc("Allowed Options");
    po::options_description alias_options;
    po::options_description all_options;
    // clang-format off
    desc.add_options()
        ("help,h", "Show this help message and exit.")
        ("args", po::value<std::string>(&args)->default_value(""), "Single USRP device selection and "
            "configuration arguments."
            "\nSpecify key-value pairs (e.g., addr, serial, type, master_clock_rate) separated by commas."
            "\nSee the UHD manual for model-specific options."
            "\nExamples:"
            "\n  --args \"addr=192.168.10.2\""
            "\n  --args \"addr=192.168.10.2,master_clock_rate=200e6\""
            "\nIf not specified, UHD connects to the first available device.")
        ("tx-args", po::value<std::string>(&tx_args), "Provides additional configuration options for the TX "
            "radio block as key-value pairs (e.g., mode_n, lo_source). The available options depend on the USRP model. "
            "Refer to the UHD manual for a complete list and details of supported arguments for your hardware.")
        ("radio-id", po::value<size_t>(&radio_id)->default_value(0), "Specifies the RFNoC radio block index "
            "to use for transmission. On devices with multiple radio blocks, this argument selects which RF frontend "
            "(e.g., 0 for the first radio, 1 for the second) is addressed for streaming.")
        ("radio-chan", po::value<size_t>(&radio_chan)->default_value(0), "Selects the channel index within "
            "the chosen RFNoC radio block for data transmission. This determines which physical transmit channel (e.g., 0 "
            "for the first channel, 1 for the second) is used for streaming.")
        ("replay-id", po::value<size_t>(&replay_id)->default_value(0), "Specifies the RFNoC Replay block "
            "index to use for playback (e.g., 0 for the first Replay block).")
        ("replay-chan", po::value<size_t>(&replay_chan)->default_value(0), "Selects the channel index within "
            "the chosen Replay block for data playback.")
        ("nsamps", po::value<size_t>(&nsamps)->default_value(0), "Sets the total number of samples to play "
            "back from the file. If set to 0, playback is continuous; otherwise, playback stops after the specified "
            "number of samples.")
        ("file", po::value<std::string>(&file)->default_value("usrp_samples.dat"), "Specifies the name of the "
            "binary file containing baseband samples to be loaded into the Replay block for playback."
            "\nThis example program requires the input file to use the sc16 format (interleaved 16-bit signed integers for "
            "I/Q), although the Replay block itself can support other data formats.")
        ("freq,f", po::value<double>(&freq)->required(), "RF center frequency in Hz.")
        ("lo-offset", po::value<double>(&lo_offset), "LO offset for the frontend in Hz.")
        ("rate,r", po::value<double>(&rate), "TX sample rate in samples/second. Note that each "
            "USRP device only supports a set of discrete sample rates, which depend on the hardware model and "
            "configuration. If you request a rate that is not supported, the USRP device will automatically select and "
            "use the closest available rate.")
        ("gain", po::value<double>(&gain), "TX gain for the RF chain in dB.")
        ("ant", po::value<std::string>(&ant), "Antenna port selection string selecting a specific antenna "
            "port for USRP daughterboards having multiple antenna connectors per RF channel."
            "\nExample: --ant \"TX/RX\"")
        ("bw", po::value<double>(&bw), "Sets the analog frontend filter bandwidth for the TX path in Hz. Not "
            "all USRP devices support programmable bandwidth; if an unsupported value is requested, the device will use "
            "the nearest supported bandwidth instead.")
        ("ref", po::value<std::string>(&ref), "Sets the source for the frequency reference. Available values "
            "depend on the USRP model. Typical values are 'internal', 'external', 'mimo', and 'gpsdo'.")
        ("dot", "Outputs the RFNoC graph as a DOT-format representation, showing the connections between "
            "blocks for visualization or analysis.")
    ;
    alias_options.add_options()
        ("tx_args", po::value<std::string>(&tx_args), "")    // alias for --tx-args for backward compatibility
        ("radio_id", po::value<size_t>(&radio_id), "")       // alias for --radio-id
        ("radio_chan", po::value<size_t>(&radio_chan), "")   // alias for --radio-chan
        ("replay_id", po::value<size_t>(&replay_id), "")     // alias for --replay-id
        ("replay_chan", po::value<size_t>(&replay_chan), "") // alias for --replay-chan
    ;
    all_options.add(desc).add(alias_options);
    // clang-format on
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, all_options), vm);
    // print the help message
    if (vm.count("help")) {
        std::cout << program_doc << std::endl;
        std::cout << desc << std::endl;
        return ~0;
    }
    po::notify(vm); // only called if --help was not requested

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
    std::cout << "Active connections:" << std::endl;
    if (vm.count("dot")) {
        std::cout << graph->to_dot() << std::endl;
    } else {
        for (auto& edge : graph->enumerate_active_connections()) {
            std::cout << "* " << edge.to_string() << std::endl;
        }
    }

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
    uhd::tune_request_t tune_request(freq);
    if (vm.count("lo-offset")) {
        std::cout << boost::format("Setting TX LO Offset: %f MHz...") % (lo_offset / 1e6)
                  << std::endl;
        tune_request = uhd::tune_request_t(freq, lo_offset);
    }

    if (vm.count("int-n")) {
        tune_request.args = uhd::device_addr_t("mode_n=integer");
    }

    auto tune_req_action = uhd::rfnoc::tune_request_action_info::make(tune_request);
    tune_req_action->tune_request = tune_request;
    replay_ctrl->post_output_action(tune_req_action, 0);

    std::cout << "TX Freq at Radio: " << (radio_ctrl->get_tx_frequency(radio_chan) / 1e6)
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
