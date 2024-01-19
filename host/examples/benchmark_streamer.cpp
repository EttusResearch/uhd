//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/convert.hpp>
#include <uhd/rfnoc/block_ctrl.hpp>
#include <uhd/rfnoc/ddc_block_ctrl.hpp>
#include <uhd/rfnoc/duc_block_ctrl.hpp>
#include <uhd/rfnoc/null_block_ctrl.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/utils/thread.hpp>
#include <boost/algorithm/string/trim_all.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <chrono>
#include <deque>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

namespace po = boost::program_options;

struct traffic_counter_values
{
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

struct host_measurement_values
{
    double seconds;
    uint64_t num_samples;
    uint64_t num_packets;
    uint64_t spp;
};

struct test_results
{
    std::vector<traffic_counter_values> traffic_counter;
    host_measurement_values host;
};

struct noc_block_endpoint
{
    std::string block_id;
    size_t port;
};

void enable_traffic_counters(uhd::property_tree::sptr tree, uhd::fs_path noc_block_root)
{
    tree->access<uint64_t>(noc_block_root / "traffic_counter/enable").set(true);
}

void disable_traffic_counters(uhd::property_tree::sptr tree, uhd::fs_path noc_block_root)
{
    tree->access<uint64_t>(noc_block_root / "traffic_counter/enable").set(false);
}

traffic_counter_values read_traffic_counters(
    uhd::property_tree::sptr tree, uhd::fs_path noc_block_root)
{
    uhd::fs_path root = noc_block_root / "traffic_counter";

    traffic_counter_values vals;
    vals.clock_cycles = tree->access<uint64_t>(root / "bus_clock_ticks").get();

    vals.xbar_to_shell_pkt_count =
        tree->access<uint64_t>(root / "xbar_to_shell_pkt_count").get();
    vals.xbar_to_shell_xfer_count =
        tree->access<uint64_t>(root / "xbar_to_shell_xfer_count").get();

    vals.shell_to_xbar_pkt_count =
        tree->access<uint64_t>(root / "shell_to_xbar_pkt_count").get();
    vals.shell_to_xbar_xfer_count =
        tree->access<uint64_t>(root / "shell_to_xbar_xfer_count").get();

    vals.shell_to_ce_pkt_count =
        tree->access<uint64_t>(root / "shell_to_ce_pkt_count").get();
    vals.shell_to_ce_xfer_count =
        tree->access<uint64_t>(root / "shell_to_ce_xfer_count").get();

    vals.ce_to_shell_pkt_count =
        tree->access<uint64_t>(root / "ce_to_shell_pkt_count").get();
    vals.ce_to_shell_xfer_count =
        tree->access<uint64_t>(root / "ce_to_shell_xfer_count").get();

    return vals;
}

void print_traffic_counters(const traffic_counter_values& vals)
{
    std::cout << "Clock cycles:        " << vals.clock_cycles << std::endl;

    std::cout << "Xbar to shell pkt count:  " << vals.xbar_to_shell_pkt_count
              << std::endl;
    std::cout << "Xbar to shell xfer count: " << vals.xbar_to_shell_xfer_count
              << std::endl;

    std::cout << "Shell to xbar pkt count:  " << vals.shell_to_xbar_pkt_count
              << std::endl;
    std::cout << "Shell to xbar xfer count: " << vals.shell_to_xbar_xfer_count
              << std::endl;

    std::cout << "Shell to CE pkt count:    " << vals.shell_to_ce_pkt_count << std::endl;
    std::cout << "Shell to CE xfer count:   " << vals.shell_to_ce_xfer_count << std::endl;

    std::cout << "CE to shell pkt count:    " << vals.ce_to_shell_pkt_count << std::endl;
    std::cout << "CE to shell xfer count:   " << vals.ce_to_shell_xfer_count << std::endl;
}

void print_rx_statistics(const traffic_counter_values& vals, const double bus_clk_freq)
{
    const double bus_time_elapsed      = vals.clock_cycles / bus_clk_freq;
    const uint64_t num_ce_packets_read = vals.ce_to_shell_pkt_count;
    const uint64_t num_ce_samples_read =
        (vals.ce_to_shell_xfer_count - num_ce_packets_read) * 2;

    const uint64_t num_non_data_packets_read =
        vals.shell_to_xbar_pkt_count - num_ce_packets_read;
    const double rx_data_packet_ratio =
        (double)num_ce_packets_read / num_non_data_packets_read;

    const double calculated_throughput = num_ce_samples_read / bus_time_elapsed;

    std::cout << "Time elapsed:          " << bus_time_elapsed << " s" << std::endl;
    std::cout << "Samples read:          " << num_ce_samples_read << std::endl;
    std::cout << "Data packets read:     " << num_ce_packets_read << std::endl;
    std::cout << "RX data packet ratio:  " << rx_data_packet_ratio
              << " data to non-data packets" << std::endl;
    std::cout << "Calculated throughput: " << calculated_throughput / 1e6 << " Msps"
              << std::endl;
}

void print_tx_statistics(const traffic_counter_values& vals, const double bus_clk_freq)
{
    const double bus_time_elapsed         = vals.clock_cycles / bus_clk_freq;
    const uint64_t num_ce_packets_written = vals.shell_to_ce_pkt_count;
    const uint64_t num_ce_samples_written =
        (vals.shell_to_ce_xfer_count - num_ce_packets_written) * 2;

    const uint64_t num_non_data_packets_written =
        vals.xbar_to_shell_pkt_count - num_ce_packets_written;
    const double tx_data_packet_ratio =
        (double)num_ce_packets_written / num_non_data_packets_written;

    const double calculated_throughput = num_ce_samples_written / bus_time_elapsed;

    std::cout << "Time elapsed:          " << bus_time_elapsed << " s" << std::endl;
    std::cout << "Samples written:       " << num_ce_samples_written << std::endl;
    std::cout << "Data packets written:  " << num_ce_packets_written << std::endl;
    std::cout << "TX data packet ratio:  " << tx_data_packet_ratio
              << " data to non-data packets" << std::endl;
    std::cout << "Calculated throughput: " << calculated_throughput / 1e6 << " Msps"
              << std::endl;
}

void print_utilization_statistics(const traffic_counter_values& vals)
{
    const double rx_data_cycles =
        vals.ce_to_shell_xfer_count - vals.ce_to_shell_pkt_count;
    const double rx_idle_cycles = vals.clock_cycles - vals.shell_to_xbar_xfer_count;
    const double rx_data_header_cycles = vals.ce_to_shell_pkt_count;
    const double rx_other_cycles =
        vals.shell_to_xbar_xfer_count - vals.ce_to_shell_xfer_count;

    const double rx_data_util        = rx_data_cycles / vals.clock_cycles * 100;
    const double rx_idle_util        = rx_idle_cycles / vals.clock_cycles * 100;
    const double rx_data_header_util = rx_data_header_cycles / vals.clock_cycles * 100;
    const double rx_other_util       = rx_other_cycles / vals.clock_cycles * 100;

    std::cout << "RX utilization:" << std::endl;
    std::cout << "   data:        " << rx_data_util << " %" << std::endl;
    std::cout << "   idle:        " << rx_idle_util << " %" << std::endl;
    std::cout << "   data header: " << rx_data_header_util << " %" << std::endl;
    std::cout << "   other:       " << rx_other_util << " % (flow control, register I/O)"
              << std::endl;
    std::cout << std::endl;

    const double tx_data_cycles =
        vals.shell_to_ce_xfer_count - vals.shell_to_ce_pkt_count;
    const double tx_idle_cycles = vals.clock_cycles - vals.xbar_to_shell_xfer_count;
    const double tx_data_header_cycles = vals.shell_to_ce_pkt_count;
    const double tx_other_cycles =
        vals.xbar_to_shell_xfer_count - vals.shell_to_ce_xfer_count;

    const double tx_data_util        = tx_data_cycles / vals.clock_cycles * 100;
    const double tx_idle_util        = tx_idle_cycles / vals.clock_cycles * 100;
    const double tx_data_header_util = tx_data_header_cycles / vals.clock_cycles * 100;
    const double tx_other_util       = tx_other_cycles / vals.clock_cycles * 100;

    std::cout << "TX utilization:" << std::endl;
    std::cout << "   data:        " << tx_data_util << " %" << std::endl;
    std::cout << "   idle:        " << tx_idle_util << " %" << std::endl;
    std::cout << "   data header: " << tx_data_header_util << " %" << std::endl;
    std::cout << "   other:       " << tx_other_util << " % (flow control, register I/O)"
              << std::endl;
}

void print_rx_results(const test_results& results, double bus_clk_freq)
{
    std::cout << "------------------------------------------------------------------"
              << std::endl;
    std::cout << "------------------- Benchmarking rx stream -----------------------"
              << std::endl;
    std::cout << "------------------------------------------------------------------"
              << std::endl;
    std::cout << "RX samples per packet: " << results.host.spp << std::endl;
    std::cout << std::endl;

    for (const auto& tc : results.traffic_counter) {
        std::cout << "------------------ Traffic counter values ------------------------"
                  << std::endl;
        print_traffic_counters(tc);
        std::cout << std::endl;

        std::cout << "------------ Values calculated from traffic counters -------------"
                  << std::endl;
        print_rx_statistics(tc, bus_clk_freq);
        std::cout << std::endl;
        print_utilization_statistics(tc);
        std::cout << std::endl;
    }

    std::cout << "--------------------- Host measurements --------------------------"
              << std::endl;
    std::cout << "Time elapsed:          " << results.host.seconds << " s" << std::endl;
    std::cout << "Samples read:          " << results.host.num_samples << std::endl;
    std::cout << "Data packets read:     " << results.host.num_packets << std::endl;
    std::cout << "Calculated throughput: "
              << results.host.num_samples / results.host.seconds / 1e6 << " Msps"
              << std::endl;
}

void print_tx_results(const test_results& results, const double bus_clk_freq)
{
    std::cout << "------------------------------------------------------------------"
              << std::endl;
    std::cout << "------------------- Benchmarking tx stream -----------------------"
              << std::endl;
    std::cout << "------------------------------------------------------------------"
              << std::endl;
    std::cout << "TX samples per packet: " << results.host.spp << std::endl;
    std::cout << std::endl;

    for (const auto& tc : results.traffic_counter) {
        std::cout << "------------------ Traffic counter values ------------------------"
                  << std::endl;
        print_traffic_counters(tc);

        std::cout << std::endl;
        std::cout << "------------ Values calculated from traffic counters -------------"
                  << std::endl;
        print_tx_statistics(tc, bus_clk_freq);
        std::cout << std::endl;
        print_utilization_statistics(tc);
        std::cout << std::endl;
    }

    std::cout << "--------------------- Host measurements --------------------------"
              << std::endl;
    std::cout << "Time elapsed:          " << results.host.seconds << " s" << std::endl;
    std::cout << "Samples written:       " << results.host.num_samples << std::endl;
    std::cout << "Data packets written:  " << results.host.num_packets << std::endl;
    std::cout << "Calculated throughput: "
              << results.host.num_samples / results.host.seconds / 1e6 << " Msps"
              << std::endl;
}

void configure_ddc(uhd::device3::sptr usrp, const std::string& ddcid, double ddc_decim)
{
    auto ddc_ctrl = usrp->get_block_ctrl<uhd::rfnoc::ddc_block_ctrl>(ddcid);
    ddc_ctrl->set_arg<double>("input_rate", 1, 0);
    ddc_ctrl->set_arg<double>("output_rate", 1 / ddc_decim, 0);
    double actual_rate = ddc_ctrl->get_arg<double>("output_rate", 0);
    std::cout << "Actual DDC decimation: " << 1 / actual_rate << std::endl;
}

void configure_duc(uhd::device3::sptr usrp, const std::string& ducid, double duc_interp)
{
    auto duc_ctrl = usrp->get_block_ctrl<uhd::rfnoc::duc_block_ctrl>(ducid);
    duc_ctrl->set_arg<double>("output_rate", 1, 0);
    duc_ctrl->set_arg<double>("input_rate", 1 / duc_interp, 0);
    double actual_rate = duc_ctrl->get_arg<double>("input_rate", 0);
    std::cout << "Actual DUC interpolation: " << 1 / actual_rate << std::endl;
}

uhd::rx_streamer::sptr configure_rx_streamer(uhd::device3::sptr usrp,
    const std::string null_id,
    const std::string splitter_id,
    const std::vector<std::vector<noc_block_endpoint>>& noc_blocks,
    const size_t spp,
    const std::string& format)
{
    std::cout << "Configuring rx stream with" << std::endl;
    std::cout << "    Null ID: " << null_id << std::endl;
    if (not splitter_id.empty()) {
        std::cout << "    Splitter ID: " << splitter_id << std::endl;
    }
    for (size_t i = 0; i < noc_blocks.size(); i++) {
        if (noc_blocks[i].size() > 0) {
            std::cout << "    Channel " << i << std::endl;
            for (const auto& b : noc_blocks[i]) {
                std::cout << "        Block ID: " << b.block_id << ", port: " << b.port
                          << std::endl;
            }
        }
    }

    auto rx_graph = usrp->create_graph("rx_graph");
    uhd::stream_args_t stream_args(format, "sc16");
    std::vector<size_t> channels;
    for (size_t i = 0; i < noc_blocks.size(); i++) {
        channels.push_back(i);
    }
    stream_args.channels = channels;

    std::vector<noc_block_endpoint> endpoints;

    if (noc_blocks.size() == 1) {
        // No splitter required
        endpoints = {{null_id, 0}};
    } else {
        // Connect to splitter
        rx_graph->connect(null_id, splitter_id);

        for (size_t i = 0; i < noc_blocks.size(); i++) {
            endpoints.push_back({splitter_id, i});
        }
    }

    for (size_t i = 0; i < noc_blocks.size(); i++) {
        std::string endpoint_id = endpoints[i].block_id;
        size_t endpoint_port    = endpoints[i].port;

        for (size_t j = 0; j < noc_blocks[i].size(); j++) {
            const auto& noc_block = noc_blocks[i][j];

            rx_graph->connect(
                endpoint_id, endpoint_port, noc_block.block_id, noc_block.port);

            endpoint_id   = noc_block.block_id;
            endpoint_port = noc_block.port;
        }

        const std::string id_str   = str(boost::format("block_id%d") % i);
        const std::string port_str = str(boost::format("block_port%d") % i);
        stream_args.args[id_str]   = endpoint_id;
        stream_args.args[port_str] = str(boost::format("%d") % endpoint_port);
    }

    if (spp != 0) {
        stream_args.args["spp"] = std::to_string(spp);
    }
    uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);

    // Configure null source
    auto null_ctrl = usrp->get_block_ctrl<uhd::rfnoc::null_block_ctrl>(null_id);
    const size_t otw_bytes_per_item =
        uhd::convert::get_bytes_per_item(stream_args.otw_format);
    const size_t samps_per_packet = rx_stream->get_max_num_samps();
    null_ctrl->set_arg<int>("line_rate", 0);
    null_ctrl->set_arg<int>("bpp", samps_per_packet * otw_bytes_per_item);

    return rx_stream;
}

test_results benchmark_rx_streamer(uhd::device3::sptr usrp,
    uhd::rx_streamer::sptr rx_stream,
    const std::string& nullid,
    const double duration,
    const std::string& format)
{
    auto null_src_ctrl = usrp->get_block_ctrl<uhd::rfnoc::null_block_ctrl>(nullid);

    // Allocate buffer
    const size_t cpu_bytes_per_item = uhd::convert::get_bytes_per_item(format);
    const size_t samps_per_packet   = rx_stream->get_max_num_samps();

    const size_t num_channels = rx_stream->get_num_channels();
    std::vector<std::vector<uint8_t>> buffer(num_channels);
    std::vector<void*> buffers;
    for (size_t i = 0; i < num_channels; i++) {
        buffer[i].resize(samps_per_packet * cpu_bytes_per_item);
        buffers.push_back(&buffer[i].front());
    }

    enable_traffic_counters(
        usrp->get_tree(), null_src_ctrl->get_block_id().get_tree_root());

    // Stream some packets
    uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
    stream_cmd.stream_now = true;
    null_src_ctrl->issue_stream_cmd(stream_cmd);

    const std::chrono::duration<double> requested_duration(duration);
    const auto start_time = std::chrono::steady_clock::now();
    auto current_time     = start_time;

    uint64_t num_rx_samps   = 0;
    uint64_t num_rx_packets = 0;
    uhd::rx_metadata_t md;

    while (current_time - start_time < requested_duration) {
        const size_t packets_per_iteration = 1000;

        for (size_t i = 0; i < packets_per_iteration; i++) {
            num_rx_samps += rx_stream->recv(buffers, samps_per_packet, md, 1.0);

            if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE) {
                if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_OVERFLOW) {
                    continue;
                } else if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_TIMEOUT) {
                    std::cout << "[ERROR] Receive timeout, aborting." << std::endl;
                    break;
                } else {
                    std::cout << std::string("[ERROR] Receiver error: ") << md.strerror()
                              << std::endl;
                    break;
                }
            }
        }
        num_rx_packets += packets_per_iteration;
        current_time = std::chrono::steady_clock::now();
    }

    disable_traffic_counters(
        usrp->get_tree(), null_src_ctrl->get_block_id().get_tree_root());

    null_src_ctrl->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);

    test_results results;
    results.traffic_counter.push_back(read_traffic_counters(
        usrp->get_tree(), null_src_ctrl->get_block_id().get_tree_root()));

    const std::chrono::duration<double> elapsed_time(current_time - start_time);
    results.host.seconds     = elapsed_time.count();
    results.host.num_samples = num_rx_samps;
    results.host.num_packets = num_rx_packets;
    results.host.spp         = samps_per_packet;

    return results;
}

uhd::tx_streamer::sptr configure_tx_streamer(uhd::device3::sptr usrp,
    const std::vector<std::vector<noc_block_endpoint>> noc_blocks,
    const size_t spp,
    const std::string& format)
{
    std::cout << "Configuring tx stream with" << std::endl;
    for (size_t i = 0; i < noc_blocks.size(); i++) {
        std::cout << "    Channel " << i << std::endl;
        for (const auto& b : noc_blocks[i]) {
            std::cout << "        Block ID: " << b.block_id << ", port: " << b.port
                      << std::endl;
        }
    }

    // Configure rfnoc
    auto tx_graph = usrp->create_graph("tx_graph");
    uhd::stream_args_t stream_args(format, "sc16");
    std::vector<size_t> channels;
    for (size_t i = 0; i < noc_blocks.size(); i++) {
        channels.push_back(i);
    }
    stream_args.channels = channels;

    for (size_t i = 0; i < noc_blocks.size(); i++) {
        std::string endpoint_id;
        size_t endpoint_port;

        for (size_t j = 0; j < noc_blocks[i].size(); j++) {
            const auto& noc_block = noc_blocks[i][j];

            if (j != 0) {
                tx_graph->connect(
                    noc_block.block_id, noc_block.port, endpoint_id, endpoint_port);
            }
            endpoint_id   = noc_block.block_id;
            endpoint_port = noc_block.port;
        }

        const std::string id_str   = str(boost::format("block_id%d") % i);
        const std::string port_str = str(boost::format("block_port%d") % i);
        stream_args.args[id_str]   = endpoint_id;
        stream_args.args[port_str] = str(boost::format("%d") % endpoint_port);
    }

    // Configure streamer
    if (spp != 0) {
        stream_args.args["spp"] = std::to_string(spp);
    }
    uhd::tx_streamer::sptr tx_stream = usrp->get_tx_stream(stream_args);

    return tx_stream;
}

test_results benchmark_tx_streamer(uhd::device3::sptr usrp,
    uhd::tx_streamer::sptr tx_stream,
    const std::vector<std::string>& null_ids,
    const double duration,
    const std::string& format)
{
    std::vector<std::shared_ptr<uhd::rfnoc::null_block_ctrl>> null_ctrls;
    for (const auto& id : null_ids) {
        null_ctrls.push_back(usrp->get_block_ctrl<uhd::rfnoc::null_block_ctrl>(id));
    }

    // Allocate buffer
    const size_t cpu_bytes_per_item = uhd::convert::get_bytes_per_item(format);
    const size_t samps_per_packet   = tx_stream->get_max_num_samps();

    const size_t num_channels = tx_stream->get_num_channels();
    std::vector<std::vector<uint8_t>> buffer(num_channels);
    std::vector<void*> buffers;
    for (size_t i = 0; i < num_channels; i++) {
        buffer[i].resize(samps_per_packet * cpu_bytes_per_item);
        buffers.push_back(&buffer[i].front());
    }

    for (auto& null_ctrl : null_ctrls) {
        enable_traffic_counters(
            usrp->get_tree(), null_ctrl->get_block_id().get_tree_root());
    }

    // Stream some packets
    uint64_t num_tx_samps   = 0;
    uint64_t num_tx_packets = 0;
    uhd::tx_metadata_t md;

    const std::chrono::duration<double> requested_duration(duration);
    const auto start_time = std::chrono::steady_clock::now();
    auto current_time     = start_time;

    while (current_time - start_time < requested_duration) {
        const size_t packets_per_iteration = 1000;

        for (size_t i = 0; i < packets_per_iteration; i++) {
            num_tx_samps += tx_stream->send(buffers, samps_per_packet, md);
        }

        num_tx_packets += packets_per_iteration;
        current_time = std::chrono::steady_clock::now();
    }

    for (auto& null_ctrl : null_ctrls) {
        disable_traffic_counters(
            usrp->get_tree(), null_ctrl->get_block_id().get_tree_root());
    }

    // Stop
    md.end_of_burst = true;
    tx_stream->send(buffers, 0, md);

    test_results results;
    for (auto& null_ctrl : null_ctrls) {
        results.traffic_counter.push_back(read_traffic_counters(
            usrp->get_tree(), null_ctrl->get_block_id().get_tree_root()));
    }

    const std::chrono::duration<double> elapsed_time(current_time - start_time);
    results.host.seconds     = elapsed_time.count();
    results.host.num_samples = num_tx_samps;
    results.host.num_packets = num_tx_packets;
    results.host.spp         = samps_per_packet;

    return results;
}

std::vector<std::string> parse_csv(const std::string& list)
{
    std::vector<std::string> result;
    std::istringstream input(list);

    std::string str;
    while (std::getline(input, str, ',')) {
        boost::algorithm::trim_all(str);
        if (not str.empty()) {
            result.push_back(str);
        }
    }
    return result;
}

std::deque<noc_block_endpoint> create_noc_block_queue(const size_t num_blocks,
    const std::string& user_override_id_list,
    const std::string& prefix,
    const size_t num_ports)
{
    const std::vector<std::string> overrides = parse_csv(user_override_id_list);
    std::deque<noc_block_endpoint> result;

    for (size_t i = 0; i < num_blocks; i++) {
        if (i < overrides.size()) {
            result.push_back({overrides[i], (i % num_ports)});
        } else {
            const std::string format_str = prefix + "_%d";
            noc_block_endpoint block     = {
                    str(boost::format(format_str) % (i / num_ports)), i % num_ports};
            result.push_back(block);
        }
    }
    return result;
}

int UHD_SAFE_MAIN(int argc, char* argv[])
{
    // Variables to be set by po
    bool dma_fifo, ddc, duc, tx_loopback_fifo, rx_loopback_fifo;
    std::string args, format;
    std::string null_ids, fifo_ids, ddc_ids, duc_ids, split_stream_ids;
    double duration;
    double ddc_decim, duc_interp, bus_clk_freq;
    size_t spp;
    size_t num_tx_streamers, num_rx_streamers, num_tx_channels, num_rx_channels;

    // Setup the program options
    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()
        ("help", "help message")
        ("args",   po::value<std::string>(&args)->default_value(""), "single uhd device address args")
        ("num_tx_streamers", po::value<size_t>(&num_tx_streamers)->default_value(0), "number of tx streamers to benchmark")
        ("num_rx_streamers", po::value<size_t>(&num_rx_streamers)->default_value(0), "number of rx streamers to benchmark")
        ("num_tx_channels", po::value<size_t>(&num_tx_channels)->default_value(1), "number of tx channels per streamer")
        ("num_rx_channels", po::value<size_t>(&num_rx_channels)->default_value(1), "number of rx channels per streamer")
        ("duration", po::value<double>(&duration)->default_value(10.0), "duration for the test in seconds")
        ("spp", po::value<size_t>(&spp)->default_value(0), "samples per packet (on FPGA and wire)")
        ("format", po::value<std::string>(&format)->default_value("sc16"), "host sample type: sc16, fc32, or fc64")
        ("bus_clk_freq", po::value<double>(&bus_clk_freq)->default_value(187.5e6), "bus clock frequency for throughput calculation (default: 187.5e6)")
        ("dma_fifo", po::bool_switch(&dma_fifo)->default_value(false), "whether to insert a DMA FIFO in the streaming path")
        ("tx_loopback_fifo", po::bool_switch(&tx_loopback_fifo)->default_value(false), "whether to insert a loopback FIFO in the tx streaming path")
        ("rx_loopback_fifo", po::bool_switch(&rx_loopback_fifo)->default_value(false), "whether to insert a loopback FIFO in the rx streaming path")
        ("ddc", po::bool_switch(&ddc)->default_value(false), "whether to insert a DDC in the rx streaming path")
        ("duc", po::bool_switch(&duc)->default_value(false), "whether to insert a DUC in the tx streaming path")
        ("ddc_decim", po::value<double>(&ddc_decim)->default_value(1), "DDC decimation, between 1 and max decimation (default: 1, no decimation)")
        ("duc_interp", po::value<double>(&duc_interp)->default_value(1), "DUC interpolation, between 1 and max interpolation (default: 1, no interpolation)")
        ("null_ids", po::value<std::string>(&null_ids)->default_value(""), "optional: list of block IDs for the null sources")
        ("fifo_ids", po::value<std::string>(&fifo_ids)->default_value(""), "optional: list of block IDs for the loopback FIFOs")
        ("ddc_ids", po::value<std::string>(&ddc_ids)->default_value(""), "optional: list of block IDs for the DDCs")
        ("duc_ids", po::value<std::string>(&duc_ids)->default_value(""), "optional: list of block IDs for the DUCs")
        ("split_stream_ids", po::value<std::string>(&split_stream_ids)->default_value(""), "optional: list of block IDs for rx data splitters")
    ;
    // clang-format on
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    // Print the help message
    const size_t num_streamers = num_rx_streamers + num_tx_streamers;

    if (vm.count("help") or (num_streamers == 0)) {
        std::cout << boost::format("UHD - Benchmark Streamer") << std::endl;
        std::cout
            << "    Benchmark streamer connects a null sink/source to a streamer and\n"
               "    measures maximum throughput. You can benchmark the operation of\n"
               "    multiple streamers concurrently. Each streamer executes in a\n"
               "    separate thread. The FPGA image on the device must contain a\n"
               "    null source for each channel in the test.\n"
               "    Benchmarks of common use-cases:\n"
               "    Specify --num_tx_streamers=1 to test tx streamer.\n"
               "    Specify --num_rx_streamers=1 to test rx streamer.\n"
               "    Specify --num_tx_streamers=1 --num_tx_channels-2 to test tx\n"
               "    streamer with two channels.\n"
               "    Specify --num_rx_streamers=1 --num_rx_channels=2 to test rx\n"
               "    rx streamer with two channels. This requires a split_stream\n"
               "    RFNOC block.\n"
               "    Specify --num_rx_streamers=1 --num_tx_streams=1 to test full\n"
               "    duplex data transfer.\n"
               "    Specify --num_rx_streamers=2 --num_rx_streams=2 to test full\n"
               "    duplex data tranfser with two streamers in each direction.\n"
               "    Benchmarks streamer allows DMA FIFOs, loopback FIFOs, DDCs, and\n"
               "    DUCs to be added to the data path. Enable these by setting the\n"
               "    corresponding Boolean option to true. The order of the blocks\n"
               "    is fixed. If present, the DMA FIFO is connected to the host bus\n"
               "    interface, followed by the loopback FIFOs, and then DUC on a tx\n"
               "    stream or a DDC on an rx stream.\n"
               "    Note: for full duplex tests, if a DMA FIFO is specified, it is\n"
               "    inserted in the tx data path only.\n"
               "    Testing multiple rx channels in a single streamer requires a\n"
               "    split stream RFNOC block with the number of outputs equal to the\n"
               "    number of channels. Each streamer connects to a single null\n"
               "    source through the split stream block.\n"
               "    In order to allow testing of blocks with different compilation\n"
               "    parameters, such as the block FIFO size, this example provides\n"
               "    options to override RFNOC block IDs. Block IDs can be specified\n"
               "    as a comma-delimited list for each type of block. If the block\n"
               "    type is used in both tx and rx streams, block IDs are assigned\n"
               "    to tx streams first, followed by rx streams. For example, a test\n"
               "    with two tx and two rx streams will assign the first two IDs in\n"
               "    the null_ids list to the tx streams and the next two IDs to the\n"
               "    rx streams.\n"
            << std::endl
            << desc << std::endl;
        return EXIT_SUCCESS;
    }

    std::cout << boost::format("Creating the usrp device with: %s...") % args
              << std::endl;
    uhd::device3::sptr usrp = uhd::device3::make(args);

    // For each block type, calculate the number of blocks needed by the test
    // and create block IDs, accounting for user overrides in program options.
    // Note that for null sources, rx only uses one NULL block per streamer
    // rather than one per channel, since the test uses split_stream blocks to
    // ensure packets on the same streamer have matching timestamps. Also, for
    // DMA FIFOs, if the test contains both tx and rx channels, we only insert
    // the FIFOs in the tx data path since that is the primary use-case.
    const size_t total_tx_channels   = num_tx_streamers * num_tx_channels;
    const size_t total_rx_channels   = num_rx_streamers * num_rx_channels;
    const size_t num_null_blocks     = total_tx_channels + num_rx_streamers;
    const size_t num_duc_blocks      = duc ? total_tx_channels : 0;
    const size_t num_ddc_blocks      = ddc ? total_rx_channels : 0;
    const size_t num_tx_fifo_blocks  = tx_loopback_fifo ? total_tx_channels : 0;
    const size_t num_rx_fifo_blocks  = rx_loopback_fifo ? total_rx_channels : 0;
    const size_t num_fifo_blocks     = num_tx_fifo_blocks + num_rx_fifo_blocks;
    const size_t num_splitter_blocks = num_rx_channels > 1 ? num_rx_streamers : 0;

    size_t num_dma_fifo_blocks = 0;
    bool tx_dma_fifo           = false;
    bool rx_dma_fifo           = false;

    if (dma_fifo) {
        if (total_tx_channels == 0) {
            num_dma_fifo_blocks = total_rx_channels;
            rx_dma_fifo         = true;
        } else {
            num_dma_fifo_blocks = total_tx_channels;
            tx_dma_fifo         = true;
        }
    }

    // Create block IDs
    std::deque<noc_block_endpoint> null_blocks =
        create_noc_block_queue(num_null_blocks, null_ids, "0/NullSrcSink", 1);

    std::deque<noc_block_endpoint> duc_blocks =
        create_noc_block_queue(num_duc_blocks, duc_ids, "0/DUC", 1);

    std::deque<noc_block_endpoint> ddc_blocks =
        create_noc_block_queue(num_ddc_blocks, ddc_ids, "0/DDC", 1);

    std::deque<noc_block_endpoint> fifo_blocks =
        create_noc_block_queue(num_fifo_blocks, fifo_ids, "0/FIFO", 1);

    std::deque<noc_block_endpoint> dma_fifo_blocks =
        create_noc_block_queue(num_dma_fifo_blocks, "", "0/DmaFIFO", 2);

    std::deque<noc_block_endpoint> splitter_blocks =
        create_noc_block_queue(num_splitter_blocks, split_stream_ids, "0/SplitStream", 1);

    // Configure all streamers
    usrp->clear();

    std::vector<uhd::tx_streamer::sptr> tx_streamers;
    std::vector<std::vector<std::string>> tx_null_ids;

    for (size_t i = 0; i < num_tx_streamers; i++) {
        std::vector<std::vector<noc_block_endpoint>> blocks(num_tx_channels);
        std::vector<std::string> null_ids;

        for (size_t ch = 0; ch < num_tx_channels; ch++) {
            // Store the null ids to read traffic counters later
            null_ids.push_back(null_blocks.front().block_id);

            // Add block IDs to create the graph for each channel
            blocks[ch].push_back(null_blocks.front());
            null_blocks.pop_front();
            if (duc) {
                configure_duc(usrp, duc_blocks.front().block_id, duc_interp);
                blocks[ch].push_back(duc_blocks.front());
                duc_blocks.pop_front();
            }
            if (tx_loopback_fifo) {
                blocks[ch].push_back(fifo_blocks.front());
                fifo_blocks.pop_front();
            }
            if (tx_dma_fifo) {
                blocks[ch].push_back(dma_fifo_blocks.front());
                dma_fifo_blocks.pop_front();
            }
        };

        tx_streamers.push_back(configure_tx_streamer(usrp, blocks, spp, format));

        tx_null_ids.push_back(null_ids);
    }

    std::vector<uhd::rx_streamer::sptr> rx_streamers;
    std::vector<std::string> rx_null_ids;

    for (size_t i = 0; i < num_rx_streamers; i++) {
        std::vector<std::vector<noc_block_endpoint>> blocks(num_rx_channels);

        for (size_t ch = 0; ch < num_rx_channels; ch++) {
            if (ddc) {
                configure_ddc(usrp, ddc_blocks.front().block_id, ddc_decim);
                blocks[ch].push_back(ddc_blocks.front());
                ddc_blocks.pop_front();
            }
            if (rx_loopback_fifo) {
                blocks[ch].push_back(fifo_blocks.front());
                fifo_blocks.pop_front();
            }
            if (rx_dma_fifo) {
                blocks[ch].push_back(dma_fifo_blocks.front());
                dma_fifo_blocks.pop_front();
            }
        };

        std::string splitter_id;
        if (num_rx_channels > 1) {
            splitter_id = splitter_blocks.front().block_id;
            splitter_blocks.pop_front();
        }

        rx_streamers.push_back(configure_rx_streamer(
            usrp, null_blocks.front().block_id, splitter_id, blocks, spp, format));

        // Store the null ids to read traffic counters later
        rx_null_ids.push_back(null_blocks.front().block_id);
        null_blocks.pop_front();
    }

    // Start threads
    std::vector<std::thread> threads;
    std::vector<test_results> tx_results(num_tx_streamers);
    for (size_t i = 0; i < num_tx_streamers; i++) {
        test_results& results             = tx_results[i];
        uhd::tx_streamer::sptr streamer   = tx_streamers[i];
        std::vector<std::string> null_ids = tx_null_ids[i];
        threads.push_back(
            std::thread([&results, usrp, streamer, null_ids, duration, format]() {
                results =
                    benchmark_tx_streamer(usrp, streamer, null_ids, duration, format);
            }));
    }

    std::vector<test_results> rx_results(num_rx_streamers);
    for (size_t i = 0; i < num_rx_streamers; i++) {
        test_results& results           = rx_results[i];
        uhd::rx_streamer::sptr streamer = rx_streamers[i];
        std::string null_id             = rx_null_ids[i];
        threads.push_back(
            std::thread([&results, usrp, streamer, null_id, duration, format]() {
                results =
                    benchmark_rx_streamer(usrp, streamer, null_id, duration, format);
            }));
    }

    // Join threads
    for (std::thread& t : threads) {
        t.join();
    }

    // Print results
    for (const test_results& result : tx_results) {
        print_tx_results(result, bus_clk_freq);
    }

    for (const test_results& result : rx_results) {
        print_rx_results(result, bus_clk_freq);
    }

    return EXIT_SUCCESS;
}
