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

struct traffic_counter_values {
    uint64_t clock_cycles;

    uint64_t xbar_to_shell_xfer_count;
    uint64_t xbar_to_shell_pkt_count;

    uint64_t shell_to_xbar_xfer_count;
    uint64_t shell_to_xbar_pkt_count;

    uint64_t shell_to_ce_xfer_count;
    uint64_t shell_to_ce_pkt_count;

    uint64_t ce_to_shell_xfer_count;
    uint64_t ce_to_shell_pkt_count;
};

struct host_measurement_values {
    double seconds;
    uint64_t num_samples;
    uint64_t num_packets;
    uint64_t spp;
};

struct test_results {
    traffic_counter_values traffic_counter;
    host_measurement_values host;
};

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

traffic_counter_values read_traffic_counters(
    uhd::property_tree::sptr tree,
    uhd::fs_path noc_block_root
) {
    uhd::fs_path root = noc_block_root/"traffic_counter";

    traffic_counter_values vals;
    vals.clock_cycles = tree->access<uint64_t>(root/"bus_clock_ticks").get();

    vals.xbar_to_shell_pkt_count  = tree->access<uint64_t>(root/"xbar_to_shell_pkt_count").get();
    vals.xbar_to_shell_xfer_count = tree->access<uint64_t>(root/"xbar_to_shell_xfer_count").get();

    vals.shell_to_xbar_pkt_count  = tree->access<uint64_t>(root/"shell_to_xbar_pkt_count").get();
    vals.shell_to_xbar_xfer_count = tree->access<uint64_t>(root/"shell_to_xbar_xfer_count").get();

    vals.shell_to_ce_pkt_count  = tree->access<uint64_t>(root/"shell_to_ce_pkt_count").get();
    vals.shell_to_ce_xfer_count = tree->access<uint64_t>(root/"shell_to_ce_xfer_count").get();

    vals.ce_to_shell_pkt_count  = tree->access<uint64_t>(root/"ce_to_shell_pkt_count").get();
    vals.ce_to_shell_xfer_count = tree->access<uint64_t>(root/"ce_to_shell_xfer_count").get();

    return vals;
}

void print_traffic_counters(
    const traffic_counter_values& vals
) {
    std::cout << "Clock cycles:        " << vals.clock_cycles << std::endl;

    std::cout << "Xbar to shell pkt count:  " << vals.xbar_to_shell_pkt_count << std::endl;
    std::cout << "Xbar to shell xfer count: " << vals.xbar_to_shell_xfer_count << std::endl;

    std::cout << "Shell to xbar pkt count:  " << vals.shell_to_xbar_pkt_count << std::endl;
    std::cout << "Shell to xbar xfer count: " << vals.shell_to_xbar_xfer_count << std::endl;

    std::cout << "Shell to CE pkt count:    " << vals.shell_to_ce_pkt_count << std::endl;
    std::cout << "Shell to CE xfer count:   " << vals.shell_to_ce_xfer_count << std::endl;

    std::cout << "CE to shell pkt count:    " << vals.ce_to_shell_pkt_count << std::endl;
    std::cout << "CE to shell xfer count:   " << vals.ce_to_shell_xfer_count << std::endl;
}

void print_rx_statistics(
    const traffic_counter_values& vals,
    const double bus_clk_freq
) {
    const double bus_time_elapsed = vals.clock_cycles / bus_clk_freq;
    const uint64_t num_ce_packets_read = vals.ce_to_shell_pkt_count;
    const uint64_t num_ce_samples_read = (vals.ce_to_shell_xfer_count - num_ce_packets_read)*2;

    const uint64_t num_non_data_packets_read = vals.shell_to_xbar_pkt_count - num_ce_packets_read;
    const double rx_data_packet_ratio = (double)num_ce_packets_read/num_non_data_packets_read;

    const double calculated_throughput = num_ce_samples_read/bus_time_elapsed;

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
    const double bus_time_elapsed = vals.clock_cycles / bus_clk_freq;
    const uint64_t num_ce_packets_written = vals.shell_to_ce_pkt_count;
    const uint64_t num_ce_samples_written = (vals.shell_to_ce_xfer_count - num_ce_packets_written)*2;

    const uint64_t num_non_data_packets_written = vals.xbar_to_shell_pkt_count - num_ce_packets_written;
    const double tx_data_packet_ratio = (double)num_ce_packets_written/num_non_data_packets_written;

    const double calculated_throughput = num_ce_samples_written/bus_time_elapsed;

    std::cout << "Time elapsed:          " << bus_time_elapsed << " s" << std::endl;
    std::cout << "Samples written:       " << num_ce_samples_written << std::endl;
    std::cout << "Data packets written:  " << num_ce_packets_written << std::endl;
    std::cout << "TX data packet ratio:  " << tx_data_packet_ratio << " data to non-data packets" << std::endl;
    std::cout << "Calculated throughput: " << calculated_throughput/1e6 << " Msps" << std::endl;
}

void print_utilization_statistics(
    const traffic_counter_values& vals
) {
    const double rx_data_cycles = vals.ce_to_shell_xfer_count - vals.ce_to_shell_pkt_count;
    const double rx_idle_cycles = vals.clock_cycles - vals.shell_to_xbar_xfer_count;
    const double rx_data_header_cycles = vals.ce_to_shell_pkt_count;
    const double rx_other_cycles = vals.shell_to_xbar_xfer_count - vals.ce_to_shell_xfer_count;

    const double rx_data_util = rx_data_cycles / vals.clock_cycles*100;
    const double rx_idle_util = rx_idle_cycles / vals.clock_cycles*100;
    const double rx_data_header_util = rx_data_header_cycles / vals.clock_cycles * 100;
    const double rx_other_util = rx_other_cycles / vals.clock_cycles * 100;

    std::cout << "RX utilization:" << std::endl;
    std::cout << "   data:        " << rx_data_util << " %" << std::endl;
    std::cout << "   idle:        " << rx_idle_util << " %" << std::endl;
    std::cout << "   data header: " << rx_data_header_util << " %" << std::endl;
    std::cout << "   other:       " << rx_other_util << " % (flow control, register I/O)" << std::endl;
    std::cout << std::endl;

    const double tx_data_cycles = vals.shell_to_ce_xfer_count - vals.shell_to_ce_pkt_count;
    const double tx_idle_cycles = vals.clock_cycles - vals.xbar_to_shell_xfer_count;
    const double tx_data_header_cycles = vals.shell_to_ce_pkt_count;
    const double tx_other_cycles = vals.xbar_to_shell_xfer_count - vals.shell_to_ce_xfer_count;

    const double tx_data_util = tx_data_cycles / vals.clock_cycles*100;
    const double tx_idle_util = tx_idle_cycles / vals.clock_cycles*100;
    const double tx_data_header_util = tx_data_header_cycles / vals.clock_cycles * 100;
    const double tx_other_util = tx_other_cycles / vals.clock_cycles * 100;

    std::cout << "TX utilization:" << std::endl;
    std::cout << "   data:        " << tx_data_util << " %" << std::endl;
    std::cout << "   idle:        " << tx_idle_util << " %" << std::endl;
    std::cout << "   data header: " << tx_data_header_util << " %" << std::endl;
    std::cout << "   other:       " << tx_other_util << " % (flow control, register I/O)" << std::endl;
}

void print_rx_results(
    const test_results& results,
    double bus_clk_freq
) {
    std::cout << "------------------------------------------------------------------" << std::endl;
    std::cout << "------------------- Benchmarking rx stream -----------------------" << std::endl;
    std::cout << "------------------------------------------------------------------" << std::endl;
    std::cout << "RX samples per packet: " << results.host.spp << std::endl;

    std::cout << std::endl;
    std::cout << "------------------ Traffic counter values ------------------------" << std::endl;
    print_traffic_counters(results.traffic_counter);

    std::cout << std::endl;
    std::cout << "------------ Values calculated from traffic counters -------------" << std::endl;
    print_rx_statistics(results.traffic_counter, bus_clk_freq);
    std::cout << std::endl;
    print_utilization_statistics(results.traffic_counter);

    std::cout << std::endl;
    std::cout << "--------------------- Host measurements --------------------------" << std::endl;
    std::cout << "Time elapsed:          " << results.host.seconds << " s" << std::endl;
    std::cout << "Samples read:          " << results.host.num_samples << std::endl;
    std::cout << "Data packets read:     " << results.host.num_packets << std::endl;
    std::cout << "Calculated throughput: " << results.host.num_samples / results.host.seconds / 1e6 << " Msps" << std::endl;
}

void print_tx_results(
    const test_results& results,
    const double bus_clk_freq
) {
    std::cout << "------------------------------------------------------------------" << std::endl;
    std::cout << "------------------- Benchmarking tx stream -----------------------" << std::endl;
    std::cout << "------------------------------------------------------------------" << std::endl;
    std::cout << "TX samples per packet: " << results.host.spp << std::endl;

    std::cout << std::endl;
    std::cout << "------------------ Traffic counter values ------------------------" << std::endl;
    print_traffic_counters(results.traffic_counter);

    std::cout << std::endl;
    std::cout << "------------ Values calculated from traffic counters -------------" << std::endl;
    print_tx_statistics(results.traffic_counter, bus_clk_freq);
    std::cout << std::endl;
    print_utilization_statistics(results.traffic_counter);

    std::cout << std::endl;
    std::cout << "--------------------- Host measurements --------------------------" << std::endl;
    std::cout << "Time elapsed:          " << results.host.seconds << " s" << std::endl;
    std::cout << "Samples written:       " << results.host.num_samples << std::endl;
    std::cout << "Data packets written:  " << results.host.num_packets << std::endl;
    std::cout << "Calculated throughput: " << results.host.num_samples / results.host.seconds / 1e6 << " Msps" << std::endl;
}

uhd::rx_streamer::sptr configure_rx_streamer(
    uhd::device3::sptr usrp,
    const std::string& nullid,
    const std::string& fifoid,
    const size_t fifo_port,
    const std::string& ddcid,
    const double ddc_decim,
    const size_t spp,
    const std::string& format
) {
    // Configure rfnoc
    std::string endpoint_id = nullid;
    size_t endpoint_port = 0;
    auto rx_graph = usrp->create_graph("rx_graph");
    if (not ddcid.empty()) {
        rx_graph->connect(endpoint_id, ddcid);
        endpoint_id = ddcid;
    }

    if (not fifoid.empty()) {
        rx_graph->connect(endpoint_id, 0, fifoid, fifo_port);
        endpoint_id = fifoid;
        endpoint_port = fifo_port;
    }

    // Configure streamer
    uhd::stream_args_t stream_args(format, "sc16");
    stream_args.args["block_id"] = endpoint_id;
    stream_args.args["block_port"] = str(boost::format("%d") % endpoint_port);
    if (spp != 0) {
        stream_args.args["spp"] = std::to_string(spp);
    }
    uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);

    // Configure null source
    const size_t otw_bytes_per_item = uhd::convert::get_bytes_per_item(stream_args.otw_format);
    const size_t samps_per_packet = rx_stream->get_max_num_samps();
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

    return rx_stream;
}

test_results benchmark_rx_streamer(
    uhd::device3::sptr usrp,
    uhd::rx_streamer::sptr rx_stream,
    const std::string& nullid,
    const double duration,
    const std::string& format
) {
    auto null_src_ctrl = usrp->get_block_ctrl<uhd::rfnoc::null_block_ctrl>(nullid);

    // Allocate buffer
    const size_t cpu_bytes_per_item = uhd::convert::get_bytes_per_item(format);
    const size_t samps_per_packet = rx_stream->get_max_num_samps();
    std::vector<uint8_t> buffer(samps_per_packet*cpu_bytes_per_item);
    std::vector<void *> buffers;
    buffers.push_back(&buffer.front());

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

    test_results results;
    results.traffic_counter = read_traffic_counters(
        usrp->get_tree(), null_src_ctrl->get_block_id().get_tree_root());

    const std::chrono::duration<double> elapsed_time(current_time-start_time);
    results.host.seconds = elapsed_time.count();
    results.host.num_samples = num_rx_samps;
    results.host.num_packets = num_rx_packets;
    results.host.spp = samps_per_packet;

    return results;
}

uhd::tx_streamer::sptr configure_tx_streamer(
    uhd::device3::sptr usrp,
    const std::string& nullid,
    const std::string& fifoid,
    const size_t fifo_port,
    const std::string& ducid,
    const double duc_interp,
    const size_t spp,
    const std::string& format
) {
    // Configure rfnoc
    std::string endpoint_id = nullid;
    size_t endpoint_port = 0;
    auto tx_graph = usrp->create_graph("tx_graph");
    if (not ducid.empty()) {
        tx_graph->connect(ducid, endpoint_id);
        endpoint_id = ducid;
    }

    if (not fifoid.empty()) {
        tx_graph->connect(fifoid, fifo_port, endpoint_id, 0);
        endpoint_id = fifoid;
        endpoint_port = fifo_port;
    }
    // Configure streamer
    uhd::stream_args_t stream_args(format, "sc16");
    stream_args.args["block_id"] = endpoint_id;
    stream_args.args["block_port"] = str(boost::format("%d") % endpoint_port);
    if (spp != 0) {
        stream_args.args["spp"] = std::to_string(spp);
    }
    uhd::tx_streamer::sptr tx_stream = usrp->get_tx_stream(stream_args);

    // Configure null sink
    auto null_sink_ctrl = usrp->get_block_ctrl<uhd::rfnoc::null_block_ctrl>(nullid);

    // Configure DUC
    if (not ducid.empty()) {
        auto duc_ctrl = usrp->get_block_ctrl<uhd::rfnoc::duc_block_ctrl>(ducid);
        duc_ctrl->set_arg<double>("output_rate", 1, 0);
        duc_ctrl->set_arg<double>("input_rate", 1/duc_interp, 0);
        double actual_rate = duc_ctrl->get_arg<double>("input_rate", 0);
        std::cout << "Actual DUC interpolation: " << 1/actual_rate << std::endl;
    }

    return tx_stream;
}

test_results benchmark_tx_streamer(
    uhd::device3::sptr usrp,
    uhd::tx_streamer::sptr tx_stream,
    const std::string& nullid,
    const double duration,
    const std::string& format
) {
    auto null_sink_ctrl = usrp->get_block_ctrl<uhd::rfnoc::null_block_ctrl>(nullid);

    // Allocate buffer
    const size_t cpu_bytes_per_item = uhd::convert::get_bytes_per_item(format);
    const size_t samps_per_packet = tx_stream->get_max_num_samps();
    std::vector<uint8_t> buffer(samps_per_packet*cpu_bytes_per_item);
    std::vector<void *> buffers;
    buffers.push_back(&buffer.front());

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

    test_results results;
    results.traffic_counter = read_traffic_counters(
        usrp->get_tree(), null_sink_ctrl->get_block_id().get_tree_root());

    const std::chrono::duration<double> elapsed_time(current_time-start_time);
    results.host.seconds = elapsed_time.count();
    results.host.num_samples = num_tx_samps;
    results.host.num_packets = num_tx_packets;
    results.host.spp = samps_per_packet;

    return results;
}

int UHD_SAFE_MAIN(int argc, char *argv[]){
    //variables to be set by po
    std::string args, format, fifoid0, ddcid0, ducid0, ddcid1, ducid1;
    std::string nullid0, nullid1, nullid2, nullid3;
    double rx_duration, tx_duration, dual_rx_duration, dual_tx_duration;
    double full_duplex_duration, dual_full_duplex_duration;
    double ddc_decim, duc_interp, bus_clk_freq;
    size_t spp;

    //setup the program options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("args",   po::value<std::string>(&args)->default_value(""), "single uhd device address args")
        ("rx_duration", po::value<double>(&rx_duration)->default_value(0.0), "duration for the rx test in seconds")
        ("tx_duration", po::value<double>(&tx_duration)->default_value(0.0), "duration for the tx test in seconds")
        ("dual_rx_duration", po::value<double>(&dual_rx_duration)->default_value(0.0), "duration for the dual rx test in seconds")
        ("dual_tx_duration", po::value<double>(&dual_tx_duration)->default_value(0.0), "duration for the dual tx test in seconds")
        ("full_duplex_duration", po::value<double>(&full_duplex_duration)->default_value(0.0), "duration for the full duplex test in seconds")
        ("dual_full_duplex_duration", po::value<double>(&dual_full_duplex_duration)->default_value(0.0), "duration for the dual full duplex test in seconds")
        ("spp", po::value<size_t>(&spp)->default_value(0), "samples per packet (on FPGA and wire)")
        ("format", po::value<std::string>(&format)->default_value("sc16"), "Host sample type: sc16, fc32, or fc64")
        ("bus_clk_freq", po::value<double>(&bus_clk_freq)->default_value(187.5e6), "Bus clock frequency for throughput calculation (default: 187.5e6)")
        ("nullid0", po::value<std::string>(&nullid0)->default_value("0/NullSrcSink_0"), "The block ID for the null source.")
        ("nullid1", po::value<std::string>(&nullid1)->default_value("0/NullSrcSink_1"), "The block ID for the second null source in measurements with two streamers.")
        ("nullid2", po::value<std::string>(&nullid2)->default_value("0/NullSrcSink_2"), "The block ID for the third null source in measuremetns with three streamers")
        ("nullid3", po::value<std::string>(&nullid3)->default_value("0/NullSrcSink_3"), "The block ID for the fourth null source in measurements with four streamers.")
        ("fifoid0", po::value<std::string>(&fifoid0)->default_value(""), "Optional: The block ID for a FIFO.")
        ("ddcid0", po::value<std::string>(&ddcid0)->default_value(""), "Optional: The block ID for a DDC for the rx stream.")
        ("ddcid1", po::value<std::string>(&ddcid1)->default_value(""), "Optional: The block ID for the second DDC in dual rx measurements.")
        ("ddc_decim", po::value<double>(&ddc_decim)->default_value(1), "DDC decimation, between 1 and max decimation (default: 1, no decimation)")
        ("ducid0", po::value<std::string>(&ducid0)->default_value(""), "Optional: The block ID for a DUC for the tx stream.")
        ("ducid1", po::value<std::string>(&ducid1)->default_value(""), "Optional: The block ID for the second DUC in dual tx measurements.")
        ("duc_interp", po::value<double>(&duc_interp)->default_value(1), "Rate of DUC, between 1 and max interpolation (default: 1, no interpolation)")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //print the help message
    const bool at_least_one_test_specified =
        rx_duration != 0.0 or tx_duration != 0.0 or
        dual_rx_duration != 0.0 or dual_tx_duration != 0.0 or
        full_duplex_duration != 0.0 or dual_full_duplex_duration != 0.0;

    if (vm.count("help") or (not at_least_one_test_specified)) {
        std::cout << boost::format("UHD - Benchmark Streamer") << std::endl;
        std::cout <<
        "    Benchmark streamer connects a null sink/source to a streamer and\n"
        "    measures maximum throughput. The null sink/source must be compiled\n"
        "    with traffic counters enabled. Optionally, a DMA FIFO and a DUC\n"
        "    can be inserted in the tx data path and a DMA FIFO and a DDC can\n"
        "    be inserted in the rx data path. The benchmark can be run with\n"
        "    multiple tx and rx streams concurrently.\n\n"
        "    Specify --rx_duration=<seconds> to run benchmark of rx streamer.\n"
        "    Specify --tx_duration=<seconds> to run benchmark of tx streamer.\n"
        "    Specify --dual_rx_duration=<seconds> to run benchmark of dual rx streamers.\n"
        "    Specify --dual_tx_duration=<seconds> to run benchmark of dual tx streamers.\n"
        "    Specify --full_duplex_duration=<seconds> to run benchmark of full duplex streamers.\n"
        "    Specify --dual_full_duplex_duration=<seconds> to run benchmark of dual full duplex streamers.\n"
        "    Note: for full duplex tests, if a DMA FIFO is specified, it is\n"
        "    inserted in the tx data path only.\n"
        << std::endl << desc << std::endl;
        return EXIT_SUCCESS;
    }

    uhd::set_thread_priority_safe();

    std::cout << boost::format("Creating the usrp device with: %s...") % args << std::endl;
    uhd::device3::sptr usrp = uhd::device3::make(args);

    if (rx_duration != 0.0) {
        usrp->clear();
        auto rx_stream = configure_rx_streamer(usrp, nullid0, fifoid0, 0,
             ddcid0, ddc_decim, spp, format);
        auto results = benchmark_rx_streamer(usrp, rx_stream, nullid0,
            rx_duration, format);
        print_rx_results(results, bus_clk_freq);
    }

    if (tx_duration != 0.0) {
        usrp->clear();
        auto tx_stream = configure_tx_streamer(usrp, nullid0, fifoid0, 0,
            ducid0, duc_interp, spp, format);
        auto results = benchmark_tx_streamer(usrp, tx_stream, nullid0,
            tx_duration, format);
        print_tx_results(results, bus_clk_freq);
    }

    if (dual_rx_duration != 0.0) {
        usrp->clear();
        auto rx_stream0 = configure_rx_streamer(usrp, nullid0, fifoid0, 0,
             ddcid0, ddc_decim, spp, format);
        auto rx_stream1 = configure_rx_streamer(usrp, nullid1, fifoid0, 1,
             ddcid1, ddc_decim, spp, format);

        test_results results0, results1;

        std::thread t0(
            [&results0, usrp, rx_stream0, nullid0, dual_rx_duration, format]() {
            results0 = benchmark_rx_streamer(usrp, rx_stream0, nullid0,
                dual_rx_duration, format);
        });
        std::thread t1(
            [&results1, usrp, rx_stream1, nullid1, dual_rx_duration, format]() {
            results1 = benchmark_rx_streamer(usrp, rx_stream1, nullid1,
                dual_rx_duration, format);
        });
        t0.join();
        t1.join();

        print_rx_results(results0, bus_clk_freq);
        print_rx_results(results1, bus_clk_freq);
    }

    if (dual_tx_duration != 0.0) {
        usrp->clear();
        auto tx_stream0 = configure_tx_streamer(usrp, nullid0, fifoid0, 0,
            ducid0, duc_interp, spp, format);
        auto tx_stream1 = configure_tx_streamer(usrp, nullid1, fifoid0, 1,
            ducid1, duc_interp, spp, format);

        test_results results0, results1;

        std::thread t0(
            [&results0, usrp, tx_stream0, nullid0, dual_tx_duration, format]() {
            results0 = benchmark_tx_streamer(usrp, tx_stream0, nullid0,
                dual_tx_duration, format);
        });
        std::thread t1(
            [&results1, usrp, tx_stream1, nullid1, dual_tx_duration, format]() {
            results1 = benchmark_tx_streamer(usrp, tx_stream1, nullid1,
                dual_tx_duration, format);
        });
        t0.join();
        t1.join();

        print_tx_results(results0, bus_clk_freq);
        print_tx_results(results1, bus_clk_freq);
    }

    if (full_duplex_duration != 0.0) {
        usrp->clear();
        auto tx_stream = configure_tx_streamer(usrp, nullid0, fifoid0, 0,
            ducid0, duc_interp, spp, format);
        auto rx_stream = configure_rx_streamer(usrp, nullid1, "", 0,
            ddcid0, ddc_decim, spp, format);

        test_results tx_results, rx_results;

        std::thread t0(
            [&tx_results, usrp, tx_stream, nullid0, full_duplex_duration, format]() {
            tx_results = benchmark_tx_streamer(usrp, tx_stream, nullid0,
                full_duplex_duration, format);
        });
        std::thread t1(
            [&rx_results, usrp, rx_stream, nullid1, full_duplex_duration, format]() {
            rx_results = benchmark_rx_streamer(usrp, rx_stream, nullid1,
                full_duplex_duration, format);
        });
        t0.join();
        t1.join();

        print_tx_results(tx_results, bus_clk_freq);
        print_rx_results(rx_results, bus_clk_freq);
    }

    if (dual_full_duplex_duration != 0.0) {
        usrp->clear();
        auto tx_stream0 = configure_tx_streamer(usrp, nullid0, fifoid0, 0,
            ducid0, duc_interp, spp, format);
        auto tx_stream1 = configure_tx_streamer(usrp, nullid1, fifoid0, 1,
            ducid1, duc_interp, spp, format);
        auto rx_stream0 = configure_rx_streamer(usrp, nullid2, "", 0,
            ddcid0, ddc_decim, spp, format);
        auto rx_stream1 = configure_rx_streamer(usrp, nullid3, "", 0,
            ddcid1, ddc_decim, spp, format);

        test_results tx_results0, tx_results1;
        test_results rx_results0, rx_results1;
        std::thread t0(
            [&tx_results0, usrp, tx_stream0, nullid0, dual_full_duplex_duration, format]() {
            tx_results0 = benchmark_tx_streamer(usrp, tx_stream0, nullid0,
                dual_full_duplex_duration, format);
        });
        std::thread t1(
            [&tx_results1, usrp, tx_stream1, nullid1, dual_full_duplex_duration, format]() {
            tx_results1 = benchmark_tx_streamer(usrp, tx_stream1, nullid1,
                dual_full_duplex_duration, format);
        });
        std::thread t2(
            [&rx_results0, usrp, rx_stream0, nullid2, dual_full_duplex_duration, format]() {
            rx_results0 = benchmark_rx_streamer(usrp, rx_stream0, nullid2,
                dual_full_duplex_duration, format);
        });
        std::thread t3(
            [&rx_results1, usrp, rx_stream1, nullid3, dual_full_duplex_duration, format]() {
            rx_results1 = benchmark_rx_streamer(usrp, rx_stream1, nullid3,
                dual_full_duplex_duration, format);
        });
        t0.join();
        t1.join();
        t2.join();
        t3.join();

        print_tx_results(tx_results0, bus_clk_freq);
        print_tx_results(tx_results1, bus_clk_freq);
        print_rx_results(rx_results0, bus_clk_freq);
        print_rx_results(rx_results1, bus_clk_freq);
    }

    return EXIT_SUCCESS;
}
