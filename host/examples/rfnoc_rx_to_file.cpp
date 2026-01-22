//
// Copyright 2014-2016 Ettus Research LLC
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/rfnoc/ddc_block_control.hpp>
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/mb_controller.hpp>
#include <uhd/rfnoc/radio_control.hpp>
#include <uhd/rfnoc_graph.hpp>
#include <uhd/types/sensors.hpp>
#include <uhd/types/tune_request.hpp>
#include <uhd/utils/graph_utils.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/utils/thread.hpp>
#include <boost/program_options.hpp>
#include <chrono>
#include <complex>
#include <csignal>
#include <fstream>
#include <functional>
#include <iostream>
#include <thread>

namespace po = boost::program_options;

constexpr int64_t UPDATE_INTERVAL = 1; // 1 second update interval for BW summary

static bool stop_signal_called = false;
void sig_int_handler(int)
{
    stop_signal_called = true;
}

template <typename samp_type>
void recv_to_file(uhd::rx_streamer::sptr rx_stream,
    const std::string& file,
    const size_t samps_per_buff,
    const double rx_rate,
    const unsigned long long num_requested_samples,
    double time_requested       = 0.0,
    bool bw_summary             = false,
    bool stats                  = false,
    bool enable_size_map        = false,
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
    uhd::stream_cmd_t stream_cmd((num_requested_samples == 0)
                                     ? uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS
                                     : uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
    stream_cmd.num_samps  = size_t(num_requested_samples);
    stream_cmd.stream_now = true;
    stream_cmd.time_spec  = uhd::time_spec_t();
    std::cout << "Issuing stream cmd" << std::endl;
    rx_stream->issue_stream_cmd(stream_cmd);

    const auto start_time = std::chrono::steady_clock::now();
    const auto stop_time =
        start_time + std::chrono::milliseconds(int64_t(1000 * time_requested));
    // Track time and samps between updating the BW summary
    auto last_update                     = start_time;
    unsigned long long last_update_samps = 0;

    typedef std::map<size_t, size_t> SizeMap;
    SizeMap mapSizes;

    // Run this loop until either time expired (if a duration was given), until
    // the requested number of samples were collected (if such a number was
    // given), or until Ctrl-C was pressed.
    while (not stop_signal_called
           and (num_requested_samples != num_total_samps or num_requested_samples == 0)
           and (time_requested == 0.0 or std::chrono::steady_clock::now() <= stop_time)) {
        const auto now = std::chrono::steady_clock::now();

        size_t num_rx_samps =
            rx_stream->recv(&buff.front(), buff.size(), md, 3.0, enable_size_map);

        if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT) {
            std::cout << "Timeout while streaming" << std::endl;
            break;
        }
        if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_OVERFLOW) {
            if (overflow_message) {
                overflow_message = false;
                std::cerr
                    << "Got an overflow indication. Please consider the following:\n"
                       "  Your write medium must sustain a rate of "
                    << (rx_rate * sizeof(samp_type) / 1e6)
                    << "MB/s.\n"
                       "  Dropped samples will not be written to the file.\n"
                       "  Please modify this example for your purposes.\n"
                       "  This message will not appear again.\n";
            }
            continue;
        }
        if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE) {
            std::string error = std::string("Receiver error: ") + md.strerror();
            if (continue_on_bad_packet) {
                std::cerr << error << std::endl;
                continue;
            } else
                throw std::runtime_error(error);
        }

        if (enable_size_map) {
            SizeMap::iterator it = mapSizes.find(num_rx_samps);
            if (it == mapSizes.end())
                mapSizes[num_rx_samps] = 0;
            mapSizes[num_rx_samps] += 1;
        }

        num_total_samps += num_rx_samps;

        if (outfile.is_open()) {
            outfile.write((const char*)&buff.front(), num_rx_samps * sizeof(samp_type));
        }

        if (bw_summary) {
            last_update_samps += num_rx_samps;
            const auto time_since_last_update = now - last_update;
            if (time_since_last_update > std::chrono::seconds(UPDATE_INTERVAL)) {
                const double time_since_last_update_s =
                    std::chrono::duration<double>(time_since_last_update).count();
                const double rate = double(last_update_samps) / time_since_last_update_s;
                std::cout << "\t" << (rate / 1e6) << " MSps" << std::endl;
                last_update_samps = 0;
                last_update       = now;
            }
        }
    }
    const auto actual_stop_time = std::chrono::steady_clock::now();

    stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
    std::cout << "Issuing stop stream cmd" << std::endl;
    rx_stream->issue_stream_cmd(stream_cmd);

    // Run recv until nothing is left
    int num_post_samps = 0;
    do {
        num_post_samps = rx_stream->recv(&buff.front(), buff.size(), md, 3.0);
    } while (num_post_samps and md.error_code == uhd::rx_metadata_t::ERROR_CODE_NONE);

    if (outfile.is_open())
        outfile.close();

    if (stats) {
        std::cout << std::endl;

        const double actual_duration_seconds =
            std::chrono::duration<float>(actual_stop_time - start_time).count();

        std::cout << "Received " << num_total_samps << " samples in "
                  << actual_duration_seconds << " seconds" << std::endl;
        const double rate = (double)num_total_samps / actual_duration_seconds;
        std::cout << (rate / 1e6) << " MSps" << std::endl;

        if (enable_size_map) {
            std::cout << std::endl;
            std::cout << "Packet size map (bytes: count)" << std::endl;
            for (SizeMap::iterator it = mapSizes.begin(); it != mapSizes.end(); it++)
                std::cout << it->first << ":\t" << it->second << std::endl;
        }
    }
}

typedef std::function<uhd::sensor_value_t(const std::string&)> get_sensor_fn_t;

bool check_locked_sensor(std::vector<std::string> sensor_names,
    const char* sensor_name,
    get_sensor_fn_t get_sensor_fn,
    double setup_time)
{
    if (std::find(sensor_names.begin(), sensor_names.end(), sensor_name)
        == sensor_names.end())
        return false;

    auto setup_timeout = std::chrono::steady_clock::now()
                         + std::chrono::milliseconds(int64_t(setup_time * 1000));
    bool lock_detected = false;

    std::cout << "Waiting for \"" << sensor_name << "\": ";
    std::cout.flush();

    while (true) {
        if (lock_detected and (std::chrono::steady_clock::now() > setup_timeout)) {
            std::cout << " locked." << std::endl;
            break;
        }
        if (get_sensor_fn(sensor_name).to_bool()) {
            std::cout << "+";
            std::cout.flush();
            lock_detected = true;
        } else {
            if (std::chrono::steady_clock::now() > setup_timeout) {
                std::cout << std::endl;
                throw std::runtime_error(
                    std::string("timed out waiting for consecutive locks on sensor \"")
                    + sensor_name + "\"");
            }
            std::cout << "_";
            std::cout.flush();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << std::endl;
    return true;
}

int UHD_SAFE_MAIN(int argc, char* argv[])
{
    const std::string program_doc =
        "usage: rfnoc_rx_to_file [--help] [--args ARGS] [--file FILE]\n"
        "                        [--format FORMAT] [--duration DURATION]\n"
        "                        [--nsamps NSAMPS] [--spb SPB] [--spp SPP]\n"
        "                        [--streamargs STREAMARGS] [--progress]\n"
        "                        [--stats] [--sizemap] [--null] [--continue]\n"
        "                        [--setup SETUP] [--radio-id RADIO_ID]\n"
        "                        [--radio-chan RADIO_CHAN] [--rate RATE]\n"
        "                        [--freq FREQ] [--lo-offset LO_OFFSET]\n"
        "                        [--gain GAIN] [--ant ANT] [--bw BW]\n"
        "                        [--ref {internal,external,mimo,gpsdo}]\n"
        "                        [--skip-lo] [--int-n] [--block-id BLOCK_ID]\n"
        "                        [--block-port BLOCK_PORT]\n"
        "                        [--block-props BLOCK_PROPS]"
        "\n\n"
        "This example demonstrates how to use the RFNoC C++ API to\n"
        "receive samples from a single channel of a single USRP device.\n"
        "It builds and configures an RFNoC graph that connects the radio block\n"
        "(RF frontend) to the DDC (digital downconverter, if present), and then\n"
        "to the host RxStreamer.\n"
        "If specified, an additional RFNoC processing block can be inserted into\n"
        "the chain; the graph is built dynamically so that received samples are\n"
        "processed by this block, enabling customized RX sample processing.\n"
        "The received data is streamed to a raw binary file on the host.\n"
        "\n"
        "Note: The additional RFNoC block must be available in the FPGA, which\n"
        "typically requires a custom FPGA build synthesizing extra RFNoC blocks\n"
        "such as FFT or FIR into the design.\n"
        "\n"
        "Key features:\n"
        "- Builds an RFNoC graph that connects the RxStreamer, Radio, and DDC (if\n"
        "  present) RFNoC blocks for sample acquisition.\n"
        "- Supports flexible insertion of an optional RFNoC processing block into\n"
        "  the signal chain, as specified by the --block-id option, allowing\n"
        "  users to process samples with custom FPGA logic prior to file capture.\n"
        "- Configures all involved RFNoC blocks, including the RxStreamer, Radio,\n"
        "  and DDC. If an additional RFNoC processing block is inserted via the\n"
        "  --block-id option, it is configured with the block properties\n"
        "  specified by the --block-props option.\n"
        "- Automatically sets up connections between RFNoC blocks and configures\n"
        "  streaming parameters according to user-specified options, ensuring\n"
        "  that all selected blocks are properly integrated into the signal\n"
        "  chain.\n"
        "\n"
        "Usage examples:\n"
        "  1. Receive 10 ms data at 2.4 GHz using a sample rate of 10 MSps and\n"
        "     save it to file:\n"
        "       rfnoc_rx_to_file --args \"addr=192.168.10.2\" --freq 2.4e09\n"
        "                        --rate 10e06 --duration 0.01\n"
        "                        --file \"usrp_samples.dat\"\n"
        "         The program will print the RFNoC connections, that were created\n"
        "         to propagate the data from the radio to the RX streamer. E.g.\n"
        "         this might look like:\n"
        "           Active connections:\n"
        "             * 0/Radio#0:0==>0/DDC#0:0\n"
        "             * 0/DDC#0:0-->RxStreamer#0:0\n"
        "  2. Use Radio 1 to receive 10 ms data at 2.4 GHz using a sample rate of\n"
        "     10 MSps and save it to file:\n"
        "       rfnoc_rx_to_file --args \"addr=192.168.10.2\" --radio-id 1\n"
        "                        --radio-chan 0 --freq 2.4e09 --rate 10e06\n"
        "                        --duration 0.01 --file usrp_samples.dat\"\n"
        "         The printed RFNoC connections now show that Radio 1 is used:\n"
        "           Active connections:\n"
        "              * 0/Radio#1:0==>0/DDC#1:0\n"
        "              * 0/DDC#1:0-->RxStreamer#0:0\n"
        "  3. Insert an RFNoC FFT block into the RFNoC block chain:\n"
        "       rfnoc_rx_to_file --args \"addr=192.168.10.2\"\n"
        "                        --block-id \"0/FFT#0\" --block-port 0\n"
        "                        --block-props \"bypass_mode=1\"\n"
        "                        --freq 2.4e09 --rate 10e06 --duration 0.01\n"
        "         The printed RFNoC connections now show that the FFT block got\n"
        "         inserted in between the DDC and the RxStreamer:\n"
        "           Active connections:\n"
        "             * 0/Radio#0:0==>0/DDC#0:0\n"
        "             * 0/DDC#0:0-->0/FFT#0:0\n"
        "             * 0/FFT#0:0-->RxStreamer#0:0"
        "\n";
    // variables to be set by po
    std::string args, file, format, ant, subdev, ref, wirefmt, streamargs, block_id,
        block_props;
    size_t total_num_samps, spb, spp, radio_id, radio_chan, block_port;
    double rate, freq, gain, bw, total_time, setup_time, lo_offset;

    // setup the program options
    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()
        ("help", "Show this help message and exit.")
        ("file", po::value<std::string>(&file)->default_value("usrp_samples.dat"), "Name of the raw binary "
            "file to which received data will be written.")
        ("format", po::value<std::string>(&format)->default_value("sc16"), "Specifies the data format of the "
            "file. The data will be written as interleaved IQ samples in one of the following numeric formats: 'double' "
            "(64-bit float, fc64), 'float' (32-bit float, fc32), or 'short' (16-bit integer, sc16, scaled to int16 range "
            "-32768 to 32767)."
            "\nChoosing 'short' as the file format matches the sc16 over-the-wire format and is usually sufficient. Using "
            "'float' or 'double' does not improve precision, but may be more convenient for post-processing or for "
            "compatibility with certain analysis tools.")
        ("duration", po::value<double>(&total_time)->default_value(0), "Total number of seconds to receive. "
            "The program stops when this number is reached.")
        ("nsamps", po::value<size_t>(&total_num_samps)->default_value(0), "Total number of samples to "
            "receive. The program stops when this number is reached.")
        ("spb", po::value<size_t>(&spb)->default_value(10000), "Size of the host data buffer that is "
            "allocated for each Rx channel."
            "\nLarger values may help to improve throughput.")
        ("spp", po::value<size_t>(&spp), "Specifies the number of samples per packet sent from the FPGA to "
            "the host. Adjusting this value can affect streaming efficiency and packetization behavior.")
        ("streamargs", po::value<std::string>(&streamargs)->default_value(""), "Specifies additional "
            "key-value arguments for configuring the RX streamer. These options allow advanced control over streaming "
            "behavior, such as buffer sizes or data formats.")
        ("progress", "Periodically display the estimated short-term USRP device to host streaming rate in "
            "samples per second.")
        ("stats", "Show the total number of samples received and the elapsed time when the program exits.")
        ("sizemap", "Track and display a breakdown of received packet sizes on exit. This helps diagnose "
            "streaming performance and packetization issues.")
        ("null", "Run without writing to file.")
        ("continue", "Continue streaming even if a bad packet is received.")
        ("args", po::value<std::string>(&args)->default_value(""), "Single USRP device selection and "
            "configuration arguments."
            "\nSpecify key-value pairs (e.g., addr, serial, type, master_clock_rate) separated by commas."
            "\nSee the UHD manual for model-specific options."
            "\nExamples:"
            "\n  --args \"addr=192.168.10.2\""
            "\n  --args \"addr=192.168.10.2,master_clock_rate=200e6\""
            "\nIf not specified, UHD connects to the first available device.")
        ("setup", po::value<double>(&setup_time)->default_value(1.0), "Sets the amount of time (in seconds) "
            "the program waits for hardware locks (e.g. LO) to ensure the device is properly synchronized before starting "
            "reception.")
        ("radio-id", po::value<size_t>(&radio_id)->default_value(0), "Specifies the RFNoC radio block index "
            "to use for reception. On devices with multiple radio blocks, this argument selects which RF frontend (e.g., "
            "0 for the first radio, 1 for the second) is addressed for streaming.")
        ("radio-chan", po::value<size_t>(&radio_chan)->default_value(0), "Selects the channel index within "
            "the chosen RFNoC radio block for data acquisition. This determines which physical receive channel (e.g., 0 "
            "for the first channel, 1 for the second) is used for streaming.")
        ("rate", po::value<double>(&rate)->default_value(1e6), "RX sample rate in samples/second. Note that "
            "each USRP device only supports a set of discrete sample rates, which depend on the hardware model and "
            "configuration. If you request a rate that is not supported, the USRP device will automatically select and "
            "use the closest available rate.")
        ("freq", po::value<double>(&freq)->default_value(0.0), "RF center frequency in Hz.")
        ("lo-offset", po::value<double>(&lo_offset), "LO offset for the frontend in Hz.")
        ("gain", po::value<double>(&gain), "RX gain for the RF chain in dB.")
        ("ant", po::value<std::string>(&ant), "Antenna port selection string selecting a specific antenna "
            "port for USRP daughterboards having multiple antenna connectors per RF channel."
            "\nExample: --ant \"TX/RX\"")
        ("bw", po::value<double>(&bw), "Sets the analog frontend filter bandwidth for the RX path in Hz. Not "
            "all USRP devices support programmable bandwidth; if an unsupported value is requested, the device will use "
            "the nearest supported bandwidth instead.")
        ("ref", po::value<std::string>(&ref), "Sets the source for the frequency reference. Available values "
            "depend on the USRP model. Typical values are 'internal', 'external', 'mimo', and 'gpsdo'.")
        ("skip-lo", "Skip checking and waiting for hardware locks (LO, reference, MIMO synchronization) "
            "before starting reception.")
        ("int-n", "Use integer-N tuning for USRP RF synthesizers. With this mode, the LO can only be tuned in "
            "discrete steps, which are integer multiples of the reference frequency. This mode can improve phase noise "
            "and spurious performance at the cost of coarser frequency resolution.")
        ("block-id", po::value<std::string>(&block_id), "Specifies an RFNoC block to insert between the radio "
            "and host (e.g., if your FPGA image includes a routable FFT or FIR block, use the RFNoC block identifiers "
            "\"0/FFT0\" or \"0/FIR0\" to address them)."
            "\nThe specified block must be present in the FPGA image; a list of available RFNoC blocks can be obtained "
            "using the uhd_usrp_probe command.")
        ("block-port", po::value<size_t>(&block_port)->default_value(0), "Selects which input port of the "
            "RFNoC block specified by --block-id receives data from the radio block. Use the port index (e.g., 0 or 1) to "
            "choose the connection point.")
        ("block-props", po::value<std::string>(&block_props), "Sets RFNoC-specific key-value properties, "
            "which are passed directly to the block specified by --block-id. For RFNoC blocks shipped with UHD, "
            "block-specific options are documented in the UHD manual.")
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

    bool bw_summary = vm.count("progress") > 0;
    bool stats      = vm.count("stats") > 0;
    if (vm.count("null") > 0) {
        file = "";
    }
    bool enable_size_map        = vm.count("sizemap") > 0;
    bool continue_on_bad_packet = vm.count("continue") > 0;

    if (enable_size_map) {
        std::cout << "Packet size tracking enabled - will only recv one packet at a time!"
                  << std::endl;
    }

    if (format != "sc16" and format != "fc32" and format != "fc64") {
        std::cout << "Invalid sample format: " << format << std::endl;
        return EXIT_FAILURE;
    }

    /************************************************************************
     * Create device and block controls
     ***********************************************************************/
    std::cout << std::endl;
    std::cout << "Creating the RFNoC graph with args: " << args << std::endl;
    auto graph = uhd::rfnoc::rfnoc_graph::make(args);

    // Create handle for radio object (e.g. 0/Radio#0)
    uhd::rfnoc::block_id_t radio_ctrl_id(0, "Radio", radio_id);
    // This next line will fail if the radio is not actually available
    auto radio_ctrl = graph->get_block<uhd::rfnoc::radio_control>(radio_ctrl_id);
    std::cout << "Using radio " << radio_id << ", channel " << radio_chan << std::endl;

    uhd::rfnoc::block_id_t last_block_in_chain;
    size_t last_port_in_chain;
    uhd::rfnoc::ddc_block_control::sptr ddc_ctrl;
    size_t ddc_chan       = 0;
    bool user_block_found = false;

    { // First, connect everything dangling off of the radio
        auto edges = uhd::rfnoc::get_block_chain(graph, radio_ctrl_id, radio_chan, true);
        last_block_in_chain = edges.back().src_blockid;
        last_port_in_chain  = edges.back().src_port;
        if (edges.size() > 1) {
            uhd::rfnoc::connect_through_blocks(graph,
                radio_ctrl_id,
                radio_chan,
                last_block_in_chain,
                last_port_in_chain);
            for (auto& edge : edges) {
                if (uhd::rfnoc::block_id_t(edge.dst_blockid).get_block_name() == "DDC") {
                    ddc_ctrl =
                        graph->get_block<uhd::rfnoc::ddc_block_control>(edge.dst_blockid);
                    ddc_chan = edge.dst_port;
                }
                if (vm.count("block-id") && edge.dst_blockid == block_id) {
                    user_block_found = true;
                }
            }
        }
    }

    // If the user block is not in the chain yet, see if we can connect that
    // separately
    if (vm.count("block-id") && !user_block_found) {
        const auto user_block_id = uhd::rfnoc::block_id_t(block_id);
        if (!graph->has_block(user_block_id)) {
            std::cout << "ERROR! No such block: " << block_id << std::endl;
            return EXIT_FAILURE;
        }
        std::cout << "Attempting to connect " << block_id << ":" << last_port_in_chain
                  << " to " << last_block_in_chain << ":" << block_port << "..."
                  << std::endl;
        uhd::rfnoc::connect_through_blocks(
            graph, last_block_in_chain, last_port_in_chain, user_block_id, block_port);
        last_block_in_chain = uhd::rfnoc::block_id_t(block_id);
        last_port_in_chain  = block_port;
        // Now we have to make sure that there are no more static connections
        // after the user-defined block
        auto edges = uhd::rfnoc::get_block_chain(
            graph, last_block_in_chain, last_port_in_chain, true);
        if (edges.size() > 1) {
            uhd::rfnoc::connect_through_blocks(graph,
                last_block_in_chain,
                last_port_in_chain,
                edges.back().src_blockid,
                edges.back().src_port);
            last_block_in_chain = edges.back().src_blockid;
            last_port_in_chain  = edges.back().src_port;
        }
    }
    /************************************************************************
     * Set up radio
     ***********************************************************************/
    // Lock mboard clocks
    if (vm.count("ref")) {
        graph->get_mb_controller(0)->set_clock_source(ref);
    }

    // set the rf gain
    if (vm.count("gain")) {
        std::cout << "Requesting RX Gain: " << gain << " dB..." << std::endl;
        radio_ctrl->set_rx_gain(gain, radio_chan);
        std::cout << "Actual RX Gain: " << radio_ctrl->get_rx_gain(radio_chan) << " dB..."
                  << std::endl
                  << std::endl;
    }

    // set the IF filter bandwidth
    if (vm.count("bw")) {
        std::cout << "Requesting RX Bandwidth: " << (bw / 1e6) << " MHz..." << std::endl;
        radio_ctrl->set_rx_bandwidth(bw, radio_chan);
        std::cout << "Actual RX Bandwidth: "
                  << (radio_ctrl->get_rx_bandwidth(radio_chan) / 1e6) << " MHz..."
                  << std::endl
                  << std::endl;
    }

    // set the antenna
    if (vm.count("ant")) {
        radio_ctrl->set_rx_antenna(ant, radio_chan);
    }

    if (vm.count("spp")) {
        std::cout << "Requesting samples per packet of: " << spp << std::endl;
        radio_ctrl->set_property<int>("spp", spp, radio_chan);
        spp = radio_ctrl->get_property<int>("spp", radio_chan);
        std::cout << "Actual samples per packet = " << spp << std::endl;
    }

    /************************************************************************
     * Set up streaming
     ***********************************************************************/
    uhd::device_addr_t streamer_args(streamargs);

    // create a receive streamer
    uhd::stream_args_t stream_args(
        format, "sc16"); // We should read the wire format from the blocks
    stream_args.args = streamer_args;
    std::cout << "Using streamer args: " << stream_args.args.to_string() << std::endl;
    auto rx_stream = graph->create_rx_streamer(1, stream_args);

    // Connect streamer to last block and commit the graph
    graph->connect(last_block_in_chain, last_port_in_chain, rx_stream, 0);
    graph->commit();
    std::cout << "Active connections:" << std::endl;
    for (auto& edge : graph->enumerate_active_connections()) {
        std::cout << "* " << edge.to_string() << std::endl;
    }

    /************************************************************************
     * Set up sampling rate and (optional) user block properties. We do this
     * after commit() so we can use the property propagation.
     ***********************************************************************/

    // set the center frequency
    if (vm.count("freq")) {
        std::cout << "Requesting RX Freq: " << (freq / 1e6) << " MHz..." << std::endl;
        uhd::tune_request_t tune_request(freq);

        if (vm.count("lo-offset")) {
            std::cout << boost::format("Setting RX LO Offset: %f MHz...")
                             % (lo_offset / 1e6)
                      << std::endl;
            tune_request = uhd::tune_request_t(freq, lo_offset);
        }

        if (vm.count("int-n")) {
            tune_request.args = uhd::device_addr_t("mode_n=integer");
        }
        auto tune_req_action = uhd::rfnoc::tune_request_action_info::make(tune_request);
        tune_req_action->tune_request = tune_request;
        rx_stream->post_input_action(tune_req_action, 0);

        std::cout << "Actual RX Freq: "
                  << (radio_ctrl->get_rx_frequency(radio_chan) / 1e6) << " MHz..."
                  << std::endl
                  << std::endl;
    }

    // set the sample rate
    if (rate <= 0.0) {
        std::cerr << "Please specify a valid sample rate" << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "Requesting RX Rate: " << (rate / 1e6) << " Msps..." << std::endl;
    if (ddc_ctrl) {
        std::cout << "Setting rate on DDC block!" << std::endl;
        rate = ddc_ctrl->set_output_rate(rate, ddc_chan);
    } else {
        std::cout << "Setting rate on radio block!" << std::endl;
        rate = radio_ctrl->set_rate(rate);
    }
    std::cout << "Actual RX Rate: " << (rate / 1e6) << " Msps..." << std::endl
              << std::endl;

    std::this_thread::sleep_for(std::chrono::milliseconds(int64_t(1000 * setup_time)));

    // check Ref and LO Lock detect
    if (not vm.count("skip-lo")) {
        check_locked_sensor(
            radio_ctrl->get_rx_sensor_names(radio_chan),
            "lo_locked",
            [&](const std::string& sensor_name) {
                return radio_ctrl->get_rx_sensor(sensor_name, radio_chan);
            },
            setup_time);
        if (ref == "external") {
            check_locked_sensor(
                graph->get_mb_controller(0)->get_sensor_names(),
                "ref_locked",
                [&](const std::string& sensor_name) {
                    return graph->get_mb_controller(0)->get_sensor(sensor_name);
                },
                setup_time);
        }
    }

    if (vm.count("block-props")) {
        std::cout << "Setting block properties to: " << block_props << std::endl;
        graph->get_block(uhd::rfnoc::block_id_t(block_id))
            ->set_properties(uhd::device_addr_t(block_props));
    }

    if (total_num_samps == 0) {
        std::signal(SIGINT, &sig_int_handler);
        std::cout << "Press Ctrl + C to stop streaming..." << std::endl;
    }
#define recv_to_file_args() \
    (rx_stream,             \
        file,               \
        spb,                \
        rate,               \
        total_num_samps,    \
        total_time,         \
        bw_summary,         \
        stats,              \
        enable_size_map,    \
        continue_on_bad_packet)
    // recv to file
    if (format == "fc64")
        recv_to_file<std::complex<double>> recv_to_file_args();
    else if (format == "fc32")
        recv_to_file<std::complex<float>> recv_to_file_args();
    else if (format == "sc16")
        recv_to_file<std::complex<short>> recv_to_file_args();
    else
        throw std::runtime_error("Unknown data format: " + format);

    // finished
    std::cout << std::endl << "Done!" << std::endl << std::endl;

    return EXIT_SUCCESS;
}
