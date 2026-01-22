//
// Copyright 2019 Ettus Research, a National Instruments Brand
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

// This is a simple example for RFNoC apps written in UHD.
// It connects a null source block to any other block on the
// crossbar (provided it has stream-through capabilities)
// and then streams the result to the host, writing it into a file.

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


template <typename samp_type>
void recv_to_file(uhd::rx_streamer::sptr rx_stream,
    const std::string& file,
    size_t samps_per_buff,
    double time_requested       = 0.0,
    bool bw_summary             = false,
    bool stats                  = false,
    bool continue_on_bad_packet = false)
{
    unsigned long long num_total_samps = 0;

    uhd::rx_metadata_t md;
    std::vector<samp_type> buff(samps_per_buff);
    std::ofstream outfile;
    if (not file.empty()) {
        outfile.open(file.c_str(), std::ofstream::binary);
    }
    bool overflow_message = true;

    // setup streaming
    uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
    stream_cmd.num_samps  = 0;
    stream_cmd.stream_now = true;
    stream_cmd.time_spec  = uhd::time_spec_t();
    std::cout << "Issuing start stream cmd" << std::endl;
    // This actually goes to the null source; the processing block
    // should propagate it.
    rx_stream->issue_stream_cmd(stream_cmd);
    std::cout << "Done" << std::endl;

    const auto start_time = std::chrono::steady_clock::now();
    const auto stop_time =
        start_time + std::chrono::milliseconds(int64_t(1000 * time_requested));
    // Track time and samps between updating the BW summary
    auto last_update                     = start_time;
    unsigned long long last_update_samps = 0;

    while (not stop_signal_called
           and (time_requested == 0.0 or std::chrono::steady_clock::now() <= stop_time)) {
        const auto now = std::chrono::steady_clock::now();

        size_t num_rx_samps = rx_stream->recv(&buff.front(), buff.size(), md, 3.0);

        if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT) {
            std::cout << "Timeout while streaming" << std::endl;
            break;
        }
        if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_OVERFLOW) {
            if (overflow_message) {
                overflow_message = false;
                std::cerr << "Got an overflow indication. If writing to disk, your\n"
                             "write medium may not be able to keep up.\n";
            }
            continue;
        }
        if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE) {
            const auto error = std::string("Receiver error: ") + md.strerror();
            if (continue_on_bad_packet) {
                std::cerr << error << std::endl;
                continue;
            } else {
                throw std::runtime_error(error);
            }
        }
        num_total_samps += num_rx_samps;

        if (outfile.is_open()) {
            outfile.write((const char*)&buff.front(), num_rx_samps * sizeof(samp_type));
        }

        if (bw_summary) {
            last_update_samps += num_rx_samps;
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
    const auto actual_stop_time = std::chrono::steady_clock::now();

    stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
    std::cout << "Issuing stop stream cmd" << std::endl;
    rx_stream->issue_stream_cmd(stream_cmd);
    std::cout << "Done" << std::endl;

    if (outfile.is_open()) {
        outfile.close();
    }

    if (stats) {
        std::cout << std::endl;
        const double actual_duration_seconds =
            std::chrono::duration<float>(actual_stop_time - start_time).count();
        std::cout << boost::format("Received %d samples in %f seconds") % num_total_samps
                         % actual_duration_seconds
                  << std::endl;
        const double rate = (double)num_total_samps / actual_duration_seconds;
        std::cout << (rate / 1e6) << " Msps" << std::endl;
    }
}


void pretty_print_flow_graph(std::vector<std::string> blocks)
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

///////////////////// MAIN ////////////////////////////////////////////////////
int UHD_SAFE_MAIN(int argc, char* argv[])
{
    const std::string program_doc =
        "usage: rfnoc_nullsource_ce_rx [-h] [--args ARGS] [--file FILE] [--null]\n"
        "                              [--time TIME] [--spb SPB] [--spp SPP]\n"
        "                              [--throttle-cycles THROTTLE_CYCLES]\n"
        "                              [--format {double,float,short}]\n"
        "                              [--progress] [--stats] [--continue]\n"
        "                              [--nullid NULLID]"
        "\n\n"
        "This example demonstrates how to use the RFNoC Null Source/Sink block to\n"
        "generate and stream test data. The program requires an FPGA image that\n"
        "includes the RFNoC Null Source/Sink block. It is useful for verifying\n"
        "RFNoC data paths and testing system connectivity without external\n"
        "signals. Generated data is also written to a file for validation\n"
        "purposes.\n"
        "\n"
        "Usage example:\n"
        "  Receive 1 second of samples from the RFNoC Null Source/Sink block\n"
        "  and display statistics:\n"
        "    rfnoc_nullsource_ce_rx --args \"addr=192.168.10.2\"\n"
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
        ("file", po::value<std::string>(&file)->default_value("graph_samples.dat"), "Name of the raw binary "
            "file to which received data will be written.")
        ("null", "Run without writing to file.")
        ("time", po::value<double>(&total_time)->default_value(0), "Total number of seconds to receive.")
        ("spb", po::value<size_t>(&spb)->default_value(10000), "Size of the host data buffer in samples.")
        ("spp", po::value<size_t>(&spp)->default_value(64), "Specifies the number of samples per packet sent "
            "from the FPGA to the host. Adjusting this value can affect streaming efficiency and packetization behavior.")
        ("throttle-cycles", po::value<size_t>(&throttle_cycles)->default_value(0), "Number of cycles to force "
            "in between packets.")
        ("format", po::value<std::string>(&format)->default_value("sc16"), "Specifies the data format of the "
            "file. The data will be written as interleaved IQ samples in one of the following numeric formats: 'double' "
            "(64-bit float, fc64), 'float' (32-bit float, fc32), or 'short' (16-bit integer, sc16, scaled to int16 range "
            "-32768 to 32767)."
            "\nChoosing 'short' as the file format matches the sc16 over-the-wire format and is usually sufficient. Using "
            "'float' or 'double' does not improve precision, but may be more convenient for post-processing or for "
            "compatibility with certain analysis tools.")
        ("progress", "Periodically display the estimated short-term USRP device to host streaming rate in "
            "samples per second.")
        ("stats", "Show the total number of samples received and the elapsed time when the program exits.")
        ("continue", "Continue streaming even if a bad packet is received.")
        ("nullid", po::value<std::string>(&nullid)->default_value("0/NullSrcSink#0"), "The block ID for the "
            "null source. Format is typically \"mboard_index/NullSrcSink#N\" (e.g. \"0/NullSrcSink#0\").")
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

    bool bw_summary             = vm.count("progress") > 0;
    bool stats                  = vm.count("stats") > 0;
    bool continue_on_bad_packet = vm.count("continue") > 0;

    // Check settings
    if (not uhd::rfnoc::block_id_t::is_valid_block_id(nullid)) {
        std::cout << "Must specify a valid block ID for the null source." << std::endl;
        return ~0;
    }

    // Set up SIGINT handler. For indefinite streaming, display info on how to stop.
    std::signal(SIGINT, &sig_int_handler);

    /////////////////////////////////////////////////////////////////////////
    //////// 1. Setup a USRP device /////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////
    std::cout << std::endl;
    std::cout << "Creating the USRP device with args: " << args << std::endl;
    uhd::rfnoc::rfnoc_graph::sptr graph = uhd::rfnoc::rfnoc_graph::make(args);

    /////////////////////////////////////////////////////////////////////////
    //////// 2. Get block control objects ///////////////////////////////////
    /////////////////////////////////////////////////////////////////////////
    std::vector<std::string> blocks;

    // For the null source control, we want to use the subclassed access,
    // so we create a null_block_control:
    uhd::rfnoc::null_block_control::sptr null_src_ctrl;
    if (graph->has_block<uhd::rfnoc::null_block_control>(nullid)) {
        null_src_ctrl = graph->get_block<uhd::rfnoc::null_block_control>(nullid);
        blocks.push_back(null_src_ctrl->get_block_id());
    } else {
        std::cout << "Error: Device has no null block." << std::endl;
        return ~0;
    }

    blocks.push_back("HOST");
    pretty_print_flow_graph(blocks);

    /////////////////////////////////////////////////////////////////////////
    //////// 3. Set channel definitions /////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////
    uhd::device_addr_t stream_args_args;
    stream_args_args["block_id"] = nullid;

    /////////////////////////////////////////////////////////////////////////
    //////// 4. Configure blocks (packet size) //////////////////////////////
    /////////////////////////////////////////////////////////////////////////
    std::cout << "Samples per packet coming from null source: " << spp << std::endl;
    const size_t BYTES_PER_SAMPLE = 4;
    null_src_ctrl->set_bytes_per_packet(uint32_t(spp * BYTES_PER_SAMPLE));
    if (null_src_ctrl->get_bytes_per_packet() != uint32_t(spp * BYTES_PER_SAMPLE)) {
        std::cout << "[ERROR] Could not set samples per packet!" << std::endl;
        return ~0;
    }
    null_src_ctrl->set_throttle_cycles(throttle_cycles);

    /////////////////////////////////////////////////////////////////////////
    //////// 5. Spawn receiver //////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////
    uhd::stream_args_t stream_args(format, "sc16");
    stream_args.args        = stream_args_args;
    stream_args.args["spp"] = std::to_string(spp);
    UHD_LOGGER_DEBUG("RFNOC") << "Using streamer args: " << stream_args.args.to_string()
                              << std::endl;
    uhd::rx_streamer::sptr rx_stream = graph->create_rx_streamer(1, stream_args);

    /////////////////////////////////////////////////////////////////////////
    //////// 6. Connect blocks //////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////
    std::cout << "Connecting blocks..." << std::endl;
    graph->connect( // Yes, it's that easy!
        null_src_ctrl->get_block_id(),
        0,
        rx_stream,
        0);
    graph->commit();

    if (total_time == 0) {
        std::cout << "Press Ctrl + C to stop streaming..." << std::endl;
    }

#define recv_to_file_args()           \
    (rx_stream,                       \
        vm.count("null") ? "" : file, \
        spb,                          \
        total_time,                   \
        bw_summary,                   \
        stats,                        \
        continue_on_bad_packet)
    // recv to file
    if (format == "fc64")
        recv_to_file<std::complex<double>> recv_to_file_args();
    else if (format == "fc32")
        recv_to_file<std::complex<float>> recv_to_file_args();
    else if (format == "sc16")
        recv_to_file<std::complex<short>> recv_to_file_args();
    else
        throw std::runtime_error("Unknown type sample type: " + format);

    // Finished!
    std::cout << std::endl << "Done!" << std::endl << std::endl;

    return EXIT_SUCCESS;
}
// vim: sw=4 expandtab:
