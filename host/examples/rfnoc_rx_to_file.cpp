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
#include <uhd/utils/safe_main.hpp>
#include <uhd/utils/thread.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <chrono>
#include <complex>
#include <csignal>
#include <fstream>
#include <functional>
#include <iostream>
#include <thread>

namespace po = boost::program_options;

const int64_t UPDATE_INTERVAL = 1; // 1 second update interval for BW summary

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
            std::cout << boost::format("Timeout while streaming") << std::endl;
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
                           % (rx_rate * sizeof(samp_type) / 1e6);
            }
            continue;
        }
        if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE) {
            std::string error = str(boost::format("Receiver error: %s") % md.strerror());
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

        std::cout << boost::format("Received %d samples in %f seconds") % num_total_samps
                         % actual_duration_seconds
                  << std::endl;
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

    std::cout << boost::format("Waiting for \"%s\": ") % sensor_name;
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
                    str(boost::format(
                            "timed out waiting for consecutive locks on sensor \"%s\"")
                        % sensor_name));
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
    // variables to be set by po
    std::string args, file, format, ant, subdev, ref, wirefmt, streamargs, block_id,
        block_args;
    size_t total_num_samps, spb, spp, radio_id, radio_chan;
    double rate, freq, gain, bw, total_time, setup_time;

    // setup the program options
    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()
        ("help", "help message")
        ("file", po::value<std::string>(&file)->default_value("usrp_samples.dat"), "name of the file to write binary samples to")
        ("format", po::value<std::string>(&format)->default_value("sc16"), "File sample format: sc16, fc32, or fc64")
        ("duration", po::value<double>(&total_time)->default_value(0), "total number of seconds to receive")
        ("nsamps", po::value<size_t>(&total_num_samps)->default_value(0), "total number of samples to receive")
        ("spb", po::value<size_t>(&spb)->default_value(10000), "samples per buffer")
        ("spp", po::value<size_t>(&spp)->default_value(64), "samples per packet (on FPGA and wire)")
        ("streamargs", po::value<std::string>(&streamargs)->default_value(""), "stream args")
        ("progress", "periodically display short-term bandwidth")
        ("stats", "show average bandwidth on exit")
        ("sizemap", "track packet size and display breakdown on exit")
        ("null", "run without writing to file")
        ("continue", "don't abort on a bad packet")

        ("args", po::value<std::string>(&args)->default_value(""), "USRP device address args")
        ("setup", po::value<double>(&setup_time)->default_value(1.0), "seconds of setup time")

        ("radio-id", po::value<size_t>(&radio_id)->default_value(0), "Radio ID to use (0 or 1).")
        ("radio-chan", po::value<size_t>(&radio_chan)->default_value(0), "Radio channel")
        ("rate", po::value<double>(&rate)->default_value(1e6), "RX rate of the radio block")
        ("freq", po::value<double>(&freq)->default_value(0.0), "RF center frequency in Hz")
        ("gain", po::value<double>(&gain), "gain for the RF chain")
        ("ant", po::value<std::string>(&ant), "antenna selection")
        ("bw", po::value<double>(&bw), "analog frontend filter bandwidth in Hz")
        ("ref", po::value<std::string>(&ref), "reference source (internal, external, mimo)")
        ("skip-lo", "skip checking LO lock status")
        ("int-n", "tune USRP with integer-N tuning")

        ("block-id", po::value<std::string>(&block_id)->default_value(""), "If block ID is specified, this block is inserted between radio and host.")
        ("block-args", po::value<std::string>(&block_args)->default_value(""), "These args are passed straight to the block.")
    ;
    // clang-format on
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    // print the help message
    if (vm.count("help")) {
        std::cout << boost::format("UHD/RFNoC RX samples to file %s") % desc << std::endl;
        std::cout << std::endl
                  << "This application streams data from a single channel of a USRP "
                     "device to a file.\n"
                  << std::endl;
        return ~0;
    }

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
    std::cout << boost::format("Creating the RFNoC graph with args: %s...") % args
              << std::endl;
    uhd::rfnoc::rfnoc_graph::sptr graph = uhd::rfnoc::rfnoc_graph::make(args);

    // Create handle for radio object
    uhd::rfnoc::block_id_t radio_ctrl_id(0, "Radio", radio_id);
    // This next line will fail if the radio is not actually available
    uhd::rfnoc::radio_control::sptr radio_ctrl =
        graph->get_block<uhd::rfnoc::radio_control>(radio_ctrl_id);
    std::cout << "Using radio " << radio_id << ", channel " << radio_chan << std::endl;

    // Enumerate blocks in the chain
    auto edges = graph->enumerate_static_connections();

    std::string source_block = radio_ctrl->get_block_id();
    size_t source_port       = radio_chan;
    auto chain               = std::vector<uhd::rfnoc::graph_edge_t>();
    uhd::rfnoc::ddc_block_control::sptr ddc_ctrl;
    size_t ddc_chan = 0;
    while (true) {
        std::cout << "Looking for source block " << source_block << ", port "
                  << source_port << std::endl;
        bool src_found = false;
        for (auto& edge : edges) {
            if (edge.src_blockid == source_block && edge.src_port == source_port) {
                auto blockid = uhd::rfnoc::block_id_t(source_block);
                if (blockid.match("DDC")) {
                    ddc_ctrl = graph->get_block<uhd::rfnoc::ddc_block_control>(blockid);
                    ddc_chan = edge.src_port;
                }
                src_found = true;
                chain.push_back(edge);
                source_block = edge.dst_blockid;
                source_port  = edge.dst_port;
            }
        }
        if (not src_found) {
            std::cerr << "ERROR: Failed to find target source block" << std::endl;
            break;
        }
        if (uhd::rfnoc::block_id_t(source_block).match(uhd::rfnoc::NODE_ID_SEP)) {
            break;
        }
    }

    /************************************************************************
     * Set up radio
     ***********************************************************************/
    // Lock mboard clocks
    if (vm.count("ref")) {
        graph->get_mb_controller(0)->set_clock_source(ref);
    }

    // set the sample rate
    if (rate <= 0.0) {
        std::cerr << "Please specify a valid sample rate" << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << boost::format("Setting RX Rate: %f Msps...") % (rate / 1e6) << std::endl;
    double radio_rate = radio_ctrl->get_rate();
    if (ddc_ctrl) {
        std::cout << "DDC block found" << std::endl;
        int decim = (int)(radio_rate / rate);
        std::cout << boost::format("Setting decimation value to %d") % decim << std::endl;
        ddc_ctrl->set_property<int>("decim", decim, ddc_chan);
        decim = ddc_ctrl->get_property<int>("decim", ddc_chan);
        std::cout << boost::format("Actual decimation value is %d") % decim << std::endl;
        rate = radio_rate / decim;
    } else {
        rate = radio_ctrl->set_rate(rate);
    }
    std::cout << boost::format("Actual RX Rate: %f Msps...") % (rate / 1e6) << std::endl
              << std::endl;

    // set the center frequency
    if (vm.count("freq")) {
        std::cout << boost::format("Setting RX Freq: %f MHz...") % (freq / 1e6)
                  << std::endl;
        uhd::tune_request_t tune_request(freq);
        if (vm.count("int-n")) {
            // tune_request.args = uhd::device_addr_t("mode_n=integer"); TODO
        }
        radio_ctrl->set_rx_frequency(freq, radio_chan);
        std::cout << boost::format("Actual RX Freq: %f MHz...")
                         % (radio_ctrl->get_rx_frequency(radio_chan) / 1e6)
                  << std::endl
                  << std::endl;
    }

    // set the rf gain
    if (vm.count("gain")) {
        std::cout << boost::format("Setting RX Gain: %f dB...") % gain << std::endl;
        radio_ctrl->set_rx_gain(gain, radio_chan);
        std::cout << boost::format("Actual RX Gain: %f dB...")
                         % radio_ctrl->get_rx_gain(radio_chan)
                  << std::endl
                  << std::endl;
    }

    // set the IF filter bandwidth
    if (vm.count("bw")) {
        // std::cout << boost::format("Setting RX Bandwidth: %f MHz...") % (bw/1e6) <<
        // std::endl; radio_ctrl->set_rx_bandwidth(bw, radio_chan); // TODO std::cout <<
        // boost::format("Actual RX Bandwidth: %f MHz...") %
        // (radio_ctrl->get_rx_bandwidth(radio_chan)/1e6) << std::endl << std::endl;
    }

    // set the antenna
    if (vm.count("ant")) {
        radio_ctrl->set_rx_antenna(ant, radio_chan);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(int64_t(1000 * setup_time)));

    // check Ref and LO Lock detect
    if (not vm.count("skip-lo")) {
        // TODO
        // check_locked_sensor(usrp->get_rx_sensor_names(0), "lo_locked",
        // std::bind(&uhd::usrp::multi_usrp::get_rx_sensor, usrp, _1, radio_id),
        // setup_time); if (ref == "external")
        // check_locked_sensor(usrp->get_mboard_sensor_names(0), "ref_locked",
        // std::bind(&uhd::usrp::multi_usrp::get_mboard_sensor, usrp, _1, radio_id),
        // setup_time);
    }

    std::cout << "Setting samples per packet to: " << spp << std::endl;
    radio_ctrl->set_property<int>("spp", spp, 0);
    spp = radio_ctrl->get_property<int>("spp", 0);
    std::cout << "Actual samples per packet = " << spp << std::endl;

    /************************************************************************
     * Set up streaming
     ***********************************************************************/
    uhd::device_addr_t streamer_args(streamargs);

    // create a receive streamer
    // std::cout << "Samples per packet: " << spp << std::endl;
    uhd::stream_args_t stream_args(
        format, "sc16"); // We should read the wire format from the blocks
    stream_args.args = streamer_args;
    // TODO?
    // stream_args.args["spp"] = boost::lexical_cast<std::string>(spp);
    std::cout << "Using streamer args: " << stream_args.args.to_string() << std::endl;
    uhd::rx_streamer::sptr rx_stream = graph->create_rx_streamer(1, stream_args);

    // Set the stream args on the radio:
    // if (block_id.empty()) {
    //    // If no extra block is required, connect to the radio:
    //    streamer_args["block_id"]   = radio_ctrl_id.to_string();
    //    streamer_args["block_port"] = str(boost::format("%d") % radio_chan);
    //} else {
    //    // Otherwise, see if the requested block exists and connect it to the radio:
    //    if (not usrp->has_block(block_id)) {
    //        std::cout << "Block does not exist on current device: " << block_id
    //                  << std::endl;
    //        return EXIT_FAILURE;
    //    }

    //    uhd::rfnoc::source_block_ctrl_base::sptr blk_ctrl =
    //        usrp->get_block_ctrl<uhd::rfnoc::source_block_ctrl_base>(block_id);

    //    if (not block_args.empty()) {
    //        // Set the block args on the other block:
    //        blk_ctrl->set_args(uhd::device_addr_t(block_args));
    //    }
    //    // Connect:
    //    std::cout << "Connecting " << radio_ctrl_id << " ==> " <<
    //    blk_ctrl->get_block_id()
    //              << std::endl;
    //    rx_graph->connect(
    //        radio_ctrl_id, radio_chan, blk_ctrl->get_block_id(), uhd::rfnoc::ANY_PORT);
    //    streamer_args["block_id"] = blk_ctrl->get_block_id().to_string();

    //    spp = blk_ctrl->get_args().cast<size_t>("spp", spp);
    //}

    // Connect blocks and commit the graph
    for (auto& edge : chain) {
        if (uhd::rfnoc::block_id_t(edge.dst_blockid).match(uhd::rfnoc::NODE_ID_SEP)) {
            graph->connect(edge.src_blockid, edge.src_port, rx_stream, 0);
        } else {
            graph->connect(
                edge.src_blockid, edge.src_port, edge.dst_blockid, edge.dst_port);
        }
    }
    graph->commit();


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
