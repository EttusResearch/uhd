//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/device3.hpp>
#include <uhd/convert.hpp>
#include <uhd/utils/thread.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/rfnoc/block_ctrl.hpp>
#include <uhd/rfnoc/null_block_ctrl.hpp>
#include <uhd/rfnoc/ddc_block_ctrl.hpp>
#include <uhd/rfnoc/duc_block_ctrl.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>

namespace po = boost::program_options;

void enable_traffic_counters(
    uhd::property_tree::sptr tree,
    uhd::fs_path noc_block_root
) {
    tree->access<uint64_t>(noc_block_root/"traffic_counter/enable").set(true);
}

void disable_traffic_counters(
    uhd::property_tree::sptr tree,
    uhd::fs_path noc_block_root
) {
    tree->access<uint64_t>(noc_block_root/"traffic_counter/enable").set(false);
}

struct traffic_counter_values {
    uint64_t clock_cycles;

    uint64_t xbar_to_shell_last;
    uint64_t xbar_to_shell_valid;
    uint64_t xbar_to_shell_ready;

    uint64_t shell_to_xbar_last;
    uint64_t shell_to_xbar_valid;
    uint64_t shell_to_xbar_ready;

    uint64_t shell_to_ce_last;
    uint64_t shell_to_ce_valid;
    uint64_t shell_to_ce_ready;

    uint64_t ce_to_shell_last;
    uint64_t ce_to_shell_valid;
    uint64_t ce_to_shell_ready;
};

traffic_counter_values read_traffic_counters(
    uhd::property_tree::sptr tree,
    uhd::fs_path noc_block_root
) {
    uhd::fs_path root = noc_block_root/"traffic_counter";

    traffic_counter_values vals;
    vals.clock_cycles = tree->access<uint64_t>(root/"bus_clock_ticks").get();

    vals.xbar_to_shell_last  = tree->access<uint64_t>(root/"xbar_to_shell_last").get();
    vals.xbar_to_shell_valid = tree->access<uint64_t>(root/"xbar_to_shell_valid").get();
    vals.xbar_to_shell_ready = tree->access<uint64_t>(root/"xbar_to_shell_ready").get();

    vals.shell_to_xbar_last  = tree->access<uint64_t>(root/"shell_to_xbar_last").get();
    vals.shell_to_xbar_valid = tree->access<uint64_t>(root/"shell_to_xbar_valid").get();
    vals.shell_to_xbar_ready = tree->access<uint64_t>(root/"shell_to_xbar_ready").get();

    vals.shell_to_ce_last  = tree->access<uint64_t>(root/"shell_to_ce_last").get();
    vals.shell_to_ce_valid = tree->access<uint64_t>(root/"shell_to_ce_valid").get();
    vals.shell_to_ce_ready = tree->access<uint64_t>(root/"shell_to_ce_ready").get();

    vals.ce_to_shell_last  = tree->access<uint64_t>(root/"ce_to_shell_last").get();
    vals.ce_to_shell_valid = tree->access<uint64_t>(root/"ce_to_shell_valid").get();
    vals.ce_to_shell_ready = tree->access<uint64_t>(root/"ce_to_shell_ready").get();

    return vals;
}

void print_traffic_counters(
    const traffic_counter_values& vals
) {
    std::cout << "Clock cycles:        " << vals.clock_cycles << std::endl;

    std::cout << "Xbar to shell last:  " << vals.xbar_to_shell_last << std::endl;
    std::cout << "Xbar to shell valid: " << vals.xbar_to_shell_valid << std::endl;
    std::cout << "Xbar to shell ready: " << vals.xbar_to_shell_ready << std::endl;

    std::cout << "Shell to xbar last:  " << vals.shell_to_xbar_last << std::endl;
    std::cout << "Shell to xbar valid: " << vals.shell_to_xbar_valid << std::endl;
    std::cout << "Shell to xbar ready: " << vals.shell_to_xbar_ready << std::endl;

    std::cout << "Shell to CE last:    " << vals.shell_to_ce_last << std::endl;
    std::cout << "Shell to CE valid:   " << vals.shell_to_ce_valid << std::endl;
    std::cout << "Shell to CE ready:   " << vals.shell_to_ce_ready << std::endl;

    std::cout << "CE to shell last:    " << vals.ce_to_shell_last << std::endl;
    std::cout << "CE to shell valid:   " << vals.ce_to_shell_valid << std::endl;
    std::cout << "CE to shell ready:   " << vals.ce_to_shell_ready << std::endl;
}

void print_rx_statistics(
    const traffic_counter_values& vals,
    const double bus_clk_freq
) {
    double bus_time_elapsed = vals.clock_cycles / bus_clk_freq;
    uint64_t num_ce_packets_read = vals.ce_to_shell_last;
    uint64_t num_ce_samples_read = (vals.ce_to_shell_valid - num_ce_packets_read)*2;

    uint64_t num_non_data_packets_read = vals.shell_to_xbar_last - num_ce_packets_read;
    double rx_data_packet_ratio = (double)num_ce_packets_read/num_non_data_packets_read;

    double calculated_throughput = num_ce_samples_read/bus_time_elapsed;

    std::cout << "Time elapsed:          " << bus_time_elapsed << " s" << std::endl;
    std::cout << "Samples read:          " << num_ce_samples_read << std::endl;
    std::cout << "Data packets read:     " << num_ce_packets_read << std::endl;
    std::cout << "RX data packet ratio:  " << rx_data_packet_ratio << " data to non-data packets" << std::endl;
    std::cout << "Calculated throughput: " << calculated_throughput/1e6 << " Msps" << std::endl;
}

void print_tx_statistics(
    const traffic_counter_values& vals,
    const double bus_clk_freq
) {
    double bus_time_elapsed = vals.clock_cycles / bus_clk_freq;
    uint64_t num_ce_packets_written = vals.shell_to_ce_last;
    uint64_t num_ce_samples_written = (vals.shell_to_ce_valid - num_ce_packets_written)*2;

    uint64_t num_non_data_packets_written = vals.xbar_to_shell_last - num_ce_packets_written;
    double tx_data_packet_ratio = (double)num_ce_packets_written/num_non_data_packets_written;

    double calculated_throughput = num_ce_samples_written/bus_time_elapsed;

    std::cout << "Time elapsed:          " << bus_time_elapsed << " s" << std::endl;
    std::cout << "Samples written:       " << num_ce_samples_written << std::endl;
    std::cout << "Data packets written:  " << num_ce_packets_written << std::endl;
    std::cout << "TX data packet ratio:  " << tx_data_packet_ratio << " data to non-data packets" << std::endl;
    std::cout << "Calculated throughput: " << calculated_throughput/1e6 << " Msps" << std::endl;
}

void print_utilization_statistics(
    const traffic_counter_values& vals
) {
    double rx_data_cycles = vals.ce_to_shell_valid - vals.ce_to_shell_last;
    double rx_idle_cycles = vals.clock_cycles - vals.shell_to_xbar_valid;
    double rx_data_header_cycles = vals.ce_to_shell_last;
    double rx_other_cycles = vals.shell_to_xbar_valid - vals.ce_to_shell_valid;

    double rx_data_util = rx_data_cycles / vals.clock_cycles*100;
    double rx_idle_util = rx_idle_cycles / vals.clock_cycles*100;
    double rx_data_header_util = rx_data_header_cycles / vals.clock_cycles * 100;
    double rx_other_util = rx_other_cycles / vals.clock_cycles * 100;

    std::cout << "RX utilization:" << std::endl;
    std::cout << "   data:        " << rx_data_util << " %" << std::endl;
    std::cout << "   idle:        " << rx_idle_util << " %" << std::endl;
    std::cout << "   data header: " << rx_data_header_util << " %" << std::endl;
    std::cout << "   other:       " << rx_other_util << " % (flow control, register I/O)" << std::endl;
    std::cout << std::endl;

    double tx_data_cycles = vals.shell_to_ce_valid - vals.shell_to_ce_last;
    double tx_idle_cycles = vals.clock_cycles - vals.xbar_to_shell_valid;
    double tx_data_header_cycles = vals.shell_to_ce_last;
    double tx_other_cycles = vals.xbar_to_shell_valid - vals.shell_to_ce_valid;

    double tx_data_util = tx_data_cycles / vals.clock_cycles*100;
    double tx_idle_util = tx_idle_cycles / vals.clock_cycles*100;
    double tx_data_header_util = tx_data_header_cycles / vals.clock_cycles * 100;
    double tx_other_util = tx_other_cycles / vals.clock_cycles * 100;

    std::cout << "TX utilization:" << std::endl;
    std::cout << "   data:        " << tx_data_util << " %" << std::endl;
    std::cout << "   idle:        " << tx_idle_util << " %" << std::endl;
    std::cout << "   data header: " << tx_data_header_util << " %" << std::endl;
    std::cout << "   other:       " << tx_other_util << " % (flow control, register I/O)" << std::endl;
}

void benchmark_rx_streamer(
    uhd::device3::sptr usrp,
    const std::string& nullid,
    const std::string& fifoid,
    const std::string& ddcid,
    const double ddc_decim,
    const double duration,
    const size_t spp,
    const std::string& format,
    const double bus_clk_freq
) {
    usrp->clear();

    // Configure rfnoc
    std::string endpoint_id = nullid;
    auto rx_graph = usrp->create_graph("rx_graph");
    if (not ddcid.empty()) {
        rx_graph->connect(endpoint_id, ddcid);
        endpoint_id = ddcid;
    }

    if (not fifoid.empty()) {
        rx_graph->connect(endpoint_id, fifoid);
        endpoint_id = fifoid;
    }

    // Configure streamer
    uhd::stream_args_t stream_args(format, "sc16");
    stream_args.args["block_id"] = endpoint_id;
    if (spp != 0) {
        stream_args.args["spp"] = std::to_string(spp);
    }
    uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);

    // Allocate buffer
    const size_t cpu_bytes_per_item = uhd::convert::get_bytes_per_item(stream_args.cpu_format);
    const size_t otw_bytes_per_item = uhd::convert::get_bytes_per_item(stream_args.otw_format);
    const size_t samps_per_packet = rx_stream->get_max_num_samps();
    std::vector<uint8_t> buffer(samps_per_packet*cpu_bytes_per_item);
    std::vector<void *> buffers;
    buffers.push_back(&buffer.front());

    // Configure null source
    auto null_src_ctrl = usrp->get_block_ctrl<uhd::rfnoc::null_block_ctrl>(nullid);
    null_src_ctrl->set_arg<int>("line_rate", 0);
    null_src_ctrl->set_arg<int>("bpp", samps_per_packet*otw_bytes_per_item);

    // Configure DDC
    if (not ddcid.empty()) {
        auto ddc_ctrl = usrp->get_block_ctrl<uhd::rfnoc::ddc_block_ctrl>(ddcid);
        ddc_ctrl->set_arg<double>("input_rate", 1, 0);
        ddc_ctrl->set_arg<double>("output_rate", 1/ddc_decim, 0);
        double actual_rate = ddc_ctrl->get_arg<double>("output_rate", 0);
        std::cout << "Actual DDC decimation: " << 1/actual_rate << std::endl;
    }

    enable_traffic_counters(
        usrp->get_tree(), null_src_ctrl->get_block_id().get_tree_root());

    // Stream some packets
    uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
    stream_cmd.stream_now = true;
    rx_stream->issue_stream_cmd(stream_cmd);

    const std::chrono::duration<double> requested_duration(duration);
    const auto start_time = std::chrono::steady_clock::now();
    auto current_time = start_time;

    uint64_t num_rx_samps = 0;
    uint64_t num_rx_packets = 0;
    uhd::rx_metadata_t md;

    while (current_time - start_time < requested_duration) {
        const size_t packets_per_iteration = 1000;

        for (size_t i = 0; i < packets_per_iteration; i++){
            num_rx_samps += rx_stream->recv(buffers, samps_per_packet, md, 1.0);

            if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE) {
                if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_OVERFLOW) {
                    continue;
                }
                else if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_TIMEOUT) {
                    std::cout << "[ERROR] Receive timeout, aborting." << std::endl;
                    break;
                }
                else {
                    std::cout << std::string("[ERROR] Receiver error: ")
                            << md.strerror() << std::endl;
                    break;
                }
            }
        }
        num_rx_packets += packets_per_iteration;
        current_time = std::chrono::steady_clock::now();
    }

    disable_traffic_counters(
        usrp->get_tree(), null_src_ctrl->get_block_id().get_tree_root());

    rx_stream->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);

    traffic_counter_values vals = read_traffic_counters(
        usrp->get_tree(), null_src_ctrl->get_block_id().get_tree_root());

    std::cout << "------------------------------------------------------------------" << std::endl;
    std::cout << "------------------- Benchmarking rx stream -----------------------" << std::endl;
    std::cout << "------------------------------------------------------------------" << std::endl;
    std::cout << "RX samples per packet: " << samps_per_packet << std::endl;

    std::cout << std::endl;
    std::cout << "------------------ Traffic counter values ------------------------" << std::endl;
    print_traffic_counters(vals);

    std::cout << std::endl;
    std::cout << "------------ Values calculated from traffic counters -------------" << std::endl;
    print_rx_statistics(vals, bus_clk_freq);
    std::cout << std::endl;
    print_utilization_statistics(vals);

    const std::chrono::duration<double> elapsed_time(current_time-start_time);

    std::cout << std::endl;
    std::cout << "--------------------- Host measurements --------------------------" << std::endl;
    std::cout << "Time elapsed:          " << elapsed_time.count() << " s" << std::endl;
    std::cout << "Samples read:          " << num_rx_samps << std::endl;
    std::cout << "Data packets read:     " << num_rx_packets << std::endl;
    std::cout << "Calculated throughput: " << num_rx_samps/elapsed_time.count()/1e6 << " Msps" << std::endl;
}

void benchmark_tx_streamer(
    uhd::device3::sptr usrp,
    const std::string& nullid,
    const std::string& fifoid,
    const std::string& ducid,
    const double duc_interp,
    const double duration,
    const size_t spp,
    const std::string& format,
    const double bus_clk_freq
) {
    usrp->clear();

    // Configure rfnoc
    std::string endpoint_id = nullid;
    auto tx_graph = usrp->create_graph("tx_graph");
    if (not ducid.empty()) {
        tx_graph->connect(ducid, endpoint_id);
        endpoint_id = ducid;
    }

    if (not fifoid.empty()) {
        tx_graph->connect(fifoid, endpoint_id);
        endpoint_id = fifoid;
    }
    // Configure streamer
    uhd::stream_args_t stream_args(format, "sc16");
    stream_args.args["block_id"] = endpoint_id;
    if (spp != 0) {
        stream_args.args["spp"] = std::to_string(spp);
    }
    uhd::tx_streamer::sptr tx_stream = usrp->get_tx_stream(stream_args);

    // Allocate buffer
    const size_t cpu_bytes_per_item = uhd::convert::get_bytes_per_item(stream_args.cpu_format);
    const size_t samps_per_packet = tx_stream->get_max_num_samps();
    std::vector<uint8_t> buffer(samps_per_packet*cpu_bytes_per_item);
    std::vector<void *> buffers;
    buffers.push_back(&buffer.front());

    // Configure null sink
    auto null_sink_ctrl = usrp->get_block_ctrl<uhd::rfnoc::null_block_ctrl>(nullid);
    null_sink_ctrl->set_arg<int>("line_rate", 0);

    // Configure DUC
    if (not ducid.empty()) {
        auto duc_ctrl = usrp->get_block_ctrl<uhd::rfnoc::duc_block_ctrl>(ducid);
        duc_ctrl->set_arg<double>("output_rate", 1, 0);
        duc_ctrl->set_arg<double>("input_rate", 1/duc_interp, 0);
        double actual_rate = duc_ctrl->get_arg<double>("input_rate", 0);
        std::cout << "Actual DUC interpolation: " << 1/actual_rate << std::endl;
    }

    enable_traffic_counters(
        usrp->get_tree(), null_sink_ctrl->get_block_id().get_tree_root());

    // Stream some packets
    uint64_t num_tx_samps = 0;
    uint64_t num_tx_packets = 0;
    uhd::tx_metadata_t md;

    const std::chrono::duration<double> requested_duration(duration);
    const auto start_time = std::chrono::steady_clock::now();
    auto current_time = start_time;

    while (current_time - start_time < requested_duration) {
        const size_t packets_per_iteration = 1000;

        for (size_t i = 0; i < packets_per_iteration; i++){
            num_tx_samps += tx_stream->send(buffers, samps_per_packet, md);
        }

        num_tx_packets += packets_per_iteration;
        current_time = std::chrono::steady_clock::now();
    }

    disable_traffic_counters(
        usrp->get_tree(), null_sink_ctrl->get_block_id().get_tree_root());

    // Stop
    md.end_of_burst = true;
    tx_stream->send(buffers, 0, md);

    traffic_counter_values vals = read_traffic_counters(
        usrp->get_tree(), null_sink_ctrl->get_block_id().get_tree_root());

    std::cout << "------------------------------------------------------------------" << std::endl;
    std::cout << "------------------- Benchmarking tx stream -----------------------" << std::endl;
    std::cout << "------------------------------------------------------------------" << std::endl;
    std::cout << "TX samples per packet: " << samps_per_packet << std::endl;

    std::cout << std::endl;
    std::cout << "------------------ Traffic counter values ------------------------" << std::endl;
    print_traffic_counters(vals);

    std::cout << std::endl;
    std::cout << "------------ Values calculated from traffic counters -------------" << std::endl;
    print_tx_statistics(vals, bus_clk_freq);
    std::cout << std::endl;
    print_utilization_statistics(vals);

    const std::chrono::duration<double> elapsed_time(current_time-start_time);

    std::cout << std::endl;
    std::cout << "--------------------- Host measurements --------------------------" << std::endl;
    std::cout << "Time elapsed:          " << elapsed_time.count() << " s" << std::endl;
    std::cout << "Samples written:       " << num_tx_samps << std::endl;
    std::cout << "Data packets written:  " << num_tx_packets << std::endl;
    std::cout << "Calculated throughput: " << num_tx_samps/elapsed_time.count()/1e6 << " Msps" << std::endl;
}

int UHD_SAFE_MAIN(int argc, char *argv[]){
    //variables to be set by po
    std::string args, format, nullid, fifoid, ddcid, ducid;
    double rx_duration, tx_duration, ddc_decim, duc_interp, bus_clk_freq;
    size_t spp;

    //setup the program options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("args",   po::value<std::string>(&args)->default_value(""), "single uhd device address args")
        ("rx_duration", po::value<double>(&rx_duration)->default_value(0.0), "duration for the rx test in seconds")
        ("tx_duration", po::value<double>(&tx_duration)->default_value(0.0), "duration for the tx test in seconds")
        ("spp", po::value<size_t>(&spp)->default_value(0), "samples per packet (on FPGA and wire)")
        ("format", po::value<std::string>(&format)->default_value("sc16"), "Host sample type: sc16, fc32, or fc64")
        ("nullid", po::value<std::string>(&nullid)->default_value("0/NullSrcSink_0"), "The block ID for the null source.")
        ("fifoid", po::value<std::string>(&fifoid)->default_value(""), "Optional: The block ID for a FIFO.")
        ("ddcid", po::value<std::string>(&ddcid)->default_value(""), "Optional: The block ID for a DDC for the RX stream.")
        ("ddc_decim", po::value<double>(&ddc_decim)->default_value(1), "DDC decimation, between 1 and max decimation (default: 1, no decimation)")
        ("ducid", po::value<std::string>(&ducid)->default_value(""), "Optional: The block ID for a DUC for the TX stream.")
        ("duc_interp", po::value<double>(&duc_interp)->default_value(1), "Rate of DUC, between 1 and max interpolation (default: 1, no interpolation)")
        ("bus_clk_freq", po::value<double>(&bus_clk_freq)->default_value(187.5e6), "Bus clock frequency for throughput calculation (default: 187.5e6)")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //print the help message
    if (vm.count("help") or (rx_duration == 0.0 and tx_duration == 0.0)) {
        std::cout << boost::format("UHD - Benchmark Streamer") << std::endl;
        std::cout <<
        "    Benchmark streamer connects a null source to a streamer and\n"
        "    measures maximum throughput.\n\n"
        "    Specify --rx_duration=<seconds> to run benchmark of rx streamer.\n"
        "    Specify --tx_duration=<seconds> to run benchmark of tx streamer.\n"
        << std::endl << desc << std::endl;
        return EXIT_SUCCESS;
    }

    uhd::set_thread_priority_safe();

    std::cout << boost::format("Creating the usrp device with: %s...") % args << std::endl;
    uhd::device3::sptr usrp = uhd::device3::make(args);

    // Check the block ids
    if (not usrp->has_block(nullid)) {
        std::cout << "[Error] Device has no null source/sink block." << std::endl;
        return EXIT_FAILURE;
    }

    if (not fifoid.empty() and not usrp->has_block(fifoid)) {
        std::cout << "[Error] Invalid FIFO ID." << std::endl;
        return EXIT_FAILURE;
    }

    if (not ddcid.empty() and not usrp->has_block(ddcid)) {
        std::cout << "[Error] Invalid DDC ID." << std::endl;
        return EXIT_FAILURE;
    }

    if (rx_duration != 0.0) {
        benchmark_rx_streamer(usrp, nullid, fifoid, ddcid, ddc_decim,
            rx_duration, spp, format, bus_clk_freq);
    }

    if (tx_duration != 0.0) {
        benchmark_tx_streamer(usrp, nullid, fifoid, ducid, duc_interp,
            tx_duration, spp, format, bus_clk_freq);
    }

    return EXIT_SUCCESS;
}
