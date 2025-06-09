//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

// This is a simple example for RFNoC apps written in UHD.
// It connects a host TX stream to a nullsrcsink block on the FPGA
// and demonstrates sending data from the host to the FPGA.

#include <uhd/exception.hpp>
#include <uhd/rfnoc/block_control.hpp>
#include <uhd/rfnoc/null_block_control.hpp>
#include <uhd/rfnoc_graph.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/utils/thread.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <chrono>
#include <complex>
#include <csignal>
#include <fstream>
#include <iostream>
#include <thread>

namespace po = boost::program_options;

static bool stop_signal_called = false;
void sig_int_handler(int)
{
    stop_signal_called = true;
}

// Function to send data from host to FPGA using tx_streamer
template <typename samp_type>
void send_from_file(uhd::tx_streamer::sptr tx_stream,
    const std::string& file,
    size_t samps_per_buff,
    double time_requested       = 0.0,
    bool bw_summary             = false,
    bool stats                  = false,
    bool continue_on_bad_packet = false)
{
    if (samps_per_buff == 0) {
        throw std::runtime_error("The host data buffer size must be greater than zero.");
    }
    std::vector<samp_type> buff(samps_per_buff);
    std::ifstream infile;
    if (!file.empty()) {
        infile.open(file.c_str(), std::ifstream::binary);
    } else {
        // Fill buffer with a ramp pattern if no file is provided
        for (size_t i = 0; i < buff.size(); i++) {
            buff[i] = samp_type(i);
        }
    }

    uhd::tx_metadata_t md;
    md.start_of_burst = true;
    md.end_of_burst   = false;
    md.has_time_spec  = false;

    const auto start_time = std::chrono::steady_clock::now();
    const auto stop_time =
        start_time + std::chrono::milliseconds(int64_t(1000 * time_requested));
    auto last_update         = start_time;
    size_t last_update_samps = 0;
    size_t total_samps_sent  = 0;
    while (!stop_signal_called
           && (time_requested == 0.0 || std::chrono::steady_clock::now() <= stop_time)) {
        size_t num_samps = buff.size();
        if (infile.is_open()) {
            infile.read(
                reinterpret_cast<char*>(&buff.front()), num_samps * sizeof(samp_type));
            num_samps = infile.gcount() / sizeof(samp_type);
            if (num_samps == 0)
                break;
        }
        size_t samps_sent = 0;
        try {
            samps_sent = tx_stream->send(&buff.front(), num_samps, md);
        } catch (const std::exception& ex) {
            if (continue_on_bad_packet) {
                std::cerr << "TX error: " << ex.what() << std::endl;
                continue;
            } else {
                throw;
            }
        }
        total_samps_sent += samps_sent;
        last_update_samps += samps_sent;
        md.start_of_burst = false;
        if (infile.is_open() && num_samps < buff.size())
            break;
        if (bw_summary) {
            const auto now                    = std::chrono::steady_clock::now();
            const auto time_since_last_update = now - last_update;
            if (time_since_last_update > std::chrono::seconds(1)) {
                const double time_since_last_update_s =
                    std::chrono::duration<double>(time_since_last_update).count();
                const double rate = double(last_update_samps) / time_since_last_update_s;
                std::cout << "\t" << (rate / 1e6) << " Msps" << std::endl;
                last_update_samps = 0;
                last_update       = now;
            }
        }
    }
    // Send end-of-burst
    md.end_of_burst = true;
    tx_stream->send("", 0, md);
    const auto actual_stop_time = std::chrono::steady_clock::now();
    if (stats) {
        std::cout << std::endl;
        const double actual_duration_seconds =
            std::chrono::duration<float>(actual_stop_time - start_time).count();
        std::cout << boost::format("Sent %d samples in %f seconds") % total_samps_sent
                         % actual_duration_seconds
                  << std::endl;
        const double rate = (double)total_samps_sent / actual_duration_seconds;
        std::cout << (rate / 1e6) << " Msps" << std::endl;
    } else {
        std::cout << boost::format("Sent %d samples to FPGA") % total_samps_sent
                  << std::endl;
    }
}

// Print a simple ASCII-art flow graph
void pretty_print_flow_graph(const std::vector<std::string>& blocks)
{
    std::string sep_str = "==>";
    std::cout << std::endl;
    // Line 1
    for (size_t n = 0; n < blocks.size(); n++) {
        const std::string name = blocks[n];
        std::cout << "+";
        for (size_t i = 0; i < name.size() + 2; i++) {
            std::cout << "-";
        }
        std::cout << "+";
        if (n == blocks.size() - 1) {
            break;
        }
        for (size_t i = 0; i < sep_str.size(); i++) {
            std::cout << " ";
        }
    }
    std::cout << std::endl;
    // Line 2
    for (size_t n = 0; n < blocks.size(); n++) {
        const std::string name = blocks[n];
        std::cout << "| " << name << " |";
        if (n == blocks.size() - 1) {
            break;
        }
        std::cout << sep_str;
    }
    std::cout << std::endl;
    // Line 3
    for (size_t n = 0; n < blocks.size(); n++) {
        const std::string name = blocks[n];
        std::cout << "+";
        for (size_t i = 0; i < name.size() + 2; i++) {
            std::cout << "-";
        }
        std::cout << "+";
        if (n == blocks.size() - 1) {
            break;
        }
        for (size_t i = 0; i < sep_str.size(); i++) {
            std::cout << " ";
        }
    }
    std::cout << std::endl << std::endl;
}

int UHD_SAFE_MAIN(int argc, char* argv[])
{
    const std::string program_doc =
        "usage: rfnoc_nullsource_ce_tx [-h] [--args ARGS] [--file FILE]\n"
        "                              [--time TIME] [--spb SPB] [--spp SPP]\n"
        "                              [--throttle-cycles THROTTLE_CYCLES]\n"
        "                              [--format {sc16,fc32,fc64}]\n"
        "                              [--progress] [--stats] [--continue]\n"
        "                              [--nullid NULLID]"
        "\n\n"
        "This example demonstrates how to use the RFNoC Null Source/Sink block as\n"
        "a sink for data streamed from the host. The program requires an FPGA\n"
        "image that includes the RFNoC Null Source/Sink block. It is useful for\n"
        "verifying RFNoC data paths and testing host-to-FPGA streaming without\n"
        "external signals. Samples are read from a file or, if no file is\n"
        "given, a generated ramp pattern is transmitted.\n"
        "\n"
        "Usage example:\n"
        "  Transmit 1 second of samples to the RFNoC Null Source/Sink block\n"
        "  and display statistics:\n"
        "    rfnoc_nullsource_ce_tx --args \"addr=192.168.10.2\"\n"
        "                           --nullid \"0/NullSrcSink#0\"\n"
        "                           --spb 10000 --spp 256 --throttle-cycles 0\n"
        "                           --progress --stats --time 1\n";

    // variables to be set by po
    std::string args, file, format, nullid;
    size_t spb, spp, throttle_cycles;
    double total_time;

    // setup the program options
    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()
        ("help,h", "Show this help message and exit.")
        ("args", po::value<std::string>(&args)->default_value("type=x300"), "Single USRP device selection and "
            "configuration arguments."
            "\nSpecify key-value pairs (e.g., addr, serial, type, master_clock_rate) separated by commas."
            "\nSee the UHD manual for model-specific options."
            "\nExamples:"
            "\n  --args \"addr=192.168.10.2\""
            "\n  --args \"addr=192.168.10.2,master_clock_rate=200e6\""
            "\nIf not specified, UHD connects to the first available device.")
        ("file", po::value<std::string>(&file)->default_value(""), "Name of the raw binary file from which "
            "samples will be read. If not specified, a generated ramp pattern is transmitted.")
        ("time", po::value<double>(&total_time)->default_value(0), "Total number of seconds to transmit. "
            "A value of 0 transmits until the file ends or Ctrl+C is pressed.")
        ("spb", po::value<size_t>(&spb)->default_value(10000), "Size of the host data buffer in samples.")
        ("spp", po::value<size_t>(&spp)->default_value(64), "Specifies the number of samples per packet sent "
            "from the host to the FPGA. Adjusting this value can affect streaming efficiency and packetization behavior.")
        ("throttle-cycles", po::value<size_t>(&throttle_cycles)->default_value(0), "Number of cycles to force "
            "in between packets.")
        ("format", po::value<std::string>(&format)->default_value("sc16"), "Specifies the data format of the "
            "file. The data is read as interleaved IQ samples in one of the following numeric formats: 'fc64' "
            "(64-bit float), 'fc32' (32-bit float), or 'sc16' (16-bit integer)."
            "\nChoosing 'sc16' as the file format matches the sc16 over-the-wire format and is usually sufficient.")
        ("progress", "Periodically display the estimated short-term host to USRP device streaming rate in "
            "samples per second.")
        ("stats", "Show the total number of samples sent and the elapsed time when the program exits.")
        ("continue", "Continue streaming even if a send error occurs.")
        ("nullid", po::value<std::string>(&nullid)->default_value("0/NullSrcSink#0"), "The block ID for the "
            "null sink. Format is typically \"mboard_index/NullSrcSink#N\" (e.g. \"0/NullSrcSink#0\").")
    ;
    // clang-format on
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    // print the help message
    if (vm.count("help")) {
        std::cout << program_doc << std::endl;
        std::cout << desc << std::endl;
        return ~0;
    }
    po::notify(vm); // only called if --help was not requested

    if (!uhd::rfnoc::block_id_t::is_valid_block_id(nullid)) {
        std::cout << "Must specify a valid block ID for the null sink." << std::endl;
        return ~0;
    }

    // Declare progress/stats/continue variables before macro use
    bool bw_summary             = vm.count("progress") > 0;
    bool stats                  = vm.count("stats") > 0;
    bool continue_on_bad_packet = vm.count("continue") > 0;

    std::signal(SIGINT, &sig_int_handler);

    std::cout << std::endl;
    std::cout << "Creating the USRP device with args: " << args << std::endl;
    uhd::rfnoc::rfnoc_graph::sptr graph = uhd::rfnoc::rfnoc_graph::make(args);

    uhd::rfnoc::null_block_control::sptr null_sink_ctrl;
    if (graph->has_block<uhd::rfnoc::null_block_control>(nullid)) {
        null_sink_ctrl = graph->get_block<uhd::rfnoc::null_block_control>(nullid);
    } else {
        std::cout << "Error: Device has no null block." << std::endl;
        return ~0;
    }

    // Print the flow graph
    std::vector<std::string> blocks;
    blocks.push_back("HOST");
    blocks.push_back(null_sink_ctrl->get_block_id());
    pretty_print_flow_graph(blocks);

    uhd::device_addr_t stream_args_args;
    stream_args_args["block_id"] = nullid;

    std::cout << "Samples per packet going to null sink: " << spp << std::endl;
    const size_t BYTES_PER_SAMPLE = 4;
    null_sink_ctrl->set_bytes_per_packet(uint32_t(spp * BYTES_PER_SAMPLE));
    if (null_sink_ctrl->get_bytes_per_packet() != uint32_t(spp * BYTES_PER_SAMPLE)) {
        std::cout << "[ERROR] Could not set samples per packet!" << std::endl;
        return ~0;
    }
    null_sink_ctrl->set_throttle_cycles(throttle_cycles);

    uhd::stream_args_t stream_args(format, "sc16");
    stream_args.args                 = stream_args_args;
    stream_args.args["spp"]          = std::to_string(spp);
    uhd::tx_streamer::sptr tx_stream = graph->create_tx_streamer(1, stream_args);

    std::cout << "Connecting blocks..." << std::endl;
    graph->connect(tx_stream, 0, null_sink_ctrl->get_block_id(), 0);
    graph->commit();

    if (total_time == 0) {
        std::cout << "Press Ctrl + C to stop transmitting..." << std::endl;
    }

#define send_from_file_args() \
    (tx_stream, file, spb, total_time, bw_summary, stats, continue_on_bad_packet)
    if (format == "fc64")
        send_from_file<std::complex<double>> send_from_file_args();
    else if (format == "fc32")
        send_from_file<std::complex<float>> send_from_file_args();
    else if (format == "sc16")
        send_from_file<std::complex<short>> send_from_file_args();
    else
        throw std::runtime_error("Unknown type sample type: " + format);

    std::cout << std::endl << "Done!" << std::endl << std::endl;
    return EXIT_SUCCESS;
}
// vim: sw=4 expandtab:
