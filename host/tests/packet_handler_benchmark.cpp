//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// This file contains a set of benchmarks for the various portions of the
// streamer implementation.

// Disable sequence checking for recv packet handler so that the benchmark
// code does not need to create new mock packet contents in every recv call.
// This should have very little effect on packet handler performance.
#define SRPH_DONT_CHECK_SEQUENCE 1

#include "../lib/transport/super_recv_packet_handler.hpp"
#include "../lib/transport/super_send_packet_handler.hpp"
#include "../lib/usrp/device3/device3_flow_ctrl.hpp"
#include "common/mock_zero_copy.hpp"
#include <uhd/convert.hpp>
#include <uhd/transport/chdr.hpp>
#include <uhd/transport/zero_copy.hpp>
#include <uhd/types/sid.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/utils/thread.hpp>
#include <boost/program_options.hpp>
#include <chrono>
#include <vector>

namespace po = boost::program_options;
using namespace uhd::transport;
using namespace uhd::usrp;

void benchmark_recv_packet_handler(const size_t spp, const std::string& format)
{
    const size_t bpi        = uhd::convert::get_bytes_per_item(format);
    const size_t frame_size = bpi * spp + DEVICE3_RX_MAX_HDR_LEN;

    mock_zero_copy::sptr xport(new mock_zero_copy(
        vrt::if_packet_info_t::LINK_TYPE_CHDR, frame_size, frame_size));

    xport->set_reuse_recv_memory(true);

    sph::recv_packet_streamer streamer(spp);
    streamer.set_vrt_unpacker(&vrt::chdr::if_hdr_unpack_be);
    streamer.set_tick_rate(1.0);
    streamer.set_samp_rate(1.0);

    uhd::convert::id_type id;
    id.output_format = format;
    id.num_inputs    = 1;
    id.input_format  = "sc16_item32_be";
    id.num_outputs   = 1;
    streamer.set_converter(id);

    streamer.set_xport_chan_get_buff(0,
        [xport](double timeout) { return xport->get_recv_buff(timeout); },
        false // flush
    );

    // Create packet for packet handler to read
    vrt::if_packet_info_t packet_info;
    packet_info.packet_type         = vrt::if_packet_info_t::PACKET_TYPE_DATA;
    packet_info.num_payload_words32 = spp;
    packet_info.num_payload_bytes   = packet_info.num_payload_words32 * sizeof(uint32_t);
    packet_info.has_tsf             = true;
    packet_info.tsf                 = 1;

    std::vector<uint32_t> recv_data(spp, 0);
    xport->push_back_recv_packet(packet_info, recv_data);

    // Allocate buffer
    std::vector<uint8_t> buffer(spp * bpi);
    std::vector<void*> buffers;
    buffers.push_back(buffer.data());

    // Run benchmark
    uhd::rx_metadata_t md;
    const auto start_time   = std::chrono::steady_clock::now();
    const size_t iterations = 1e7;

    for (size_t i = 0; i < iterations; i++) {
        streamer.recv(buffers, spp, md, 1.0, true);
    }

    const auto end_time = std::chrono::steady_clock::now();
    const std::chrono::duration<double> elapsed_time(end_time - start_time);
    const double time_per_packet = elapsed_time.count() / iterations;

    std::cout << format << ": " << time_per_packet / spp * 1e9 << " ns/sample, "
              << time_per_packet * 1e9 << " ns/packet\n";
}

void benchmark_send_packet_handler(
    const size_t spp, const std::string& format, bool use_time_spec)
{
    const size_t bpi        = uhd::convert::get_bytes_per_item(format);
    const size_t frame_size = bpi * spp + DEVICE3_TX_MAX_HDR_LEN;

    mock_zero_copy::sptr xport(new mock_zero_copy(
        vrt::if_packet_info_t::LINK_TYPE_CHDR, frame_size, frame_size));

    xport->set_reuse_send_memory(true);

    sph::send_packet_streamer streamer(spp);
    streamer.set_vrt_packer(&vrt::chdr::if_hdr_pack_be);

    uhd::convert::id_type id;
    id.input_format  = format;
    id.num_inputs    = 1;
    id.output_format = "sc16_item32_be";
    id.num_outputs   = 1;
    streamer.set_converter(id);
    streamer.set_enable_trailer(false);

    streamer.set_xport_chan_get_buff(
        0, [xport](double timeout) { return xport->get_send_buff(timeout); });

    // Allocate buffer
    std::vector<uint8_t> buffer(spp * bpi);
    std::vector<void*> buffers;
    buffers.push_back(buffer.data());

    // Run benchmark
    uhd::tx_metadata_t md;
    md.has_time_spec = use_time_spec;

    const auto start_time   = std::chrono::steady_clock::now();
    const size_t iterations = 1e7;

    for (size_t i = 0; i < iterations; i++) {
        if (use_time_spec) {
            md.time_spec = uhd::time_spec_t(i, 0.0);
        }
        streamer.send(buffers, spp, md, 1.0);
    }

    const auto end_time = std::chrono::steady_clock::now();
    const std::chrono::duration<double> elapsed_time(end_time - start_time);
    const double time_per_packet = elapsed_time.count() / iterations;

    std::cout << format << ": " << time_per_packet / spp * 1e9 << " ns/sample, "
              << time_per_packet * 1e9 << " ns/packet\n";
}

void benchmark_device3_rx_flow_ctrl(bool send_flow_control_packet)
{
    // Arbitrary sizes
    constexpr uint32_t fc_window = 10000;

    mock_zero_copy::sptr xport(new mock_zero_copy(vrt::if_packet_info_t::LINK_TYPE_CHDR));

    xport->set_reuse_recv_memory(true);
    xport->set_reuse_send_memory(true);

    boost::shared_ptr<rx_fc_cache_t> fc_cache(new rx_fc_cache_t());
    fc_cache->to_host   = uhd::ntohx<uint32_t>;
    fc_cache->from_host = uhd::htonx<uint32_t>;
    fc_cache->pack      = vrt::chdr::if_hdr_pack_be;
    fc_cache->unpack    = vrt::chdr::if_hdr_unpack_be;
    fc_cache->xport     = xport;
    fc_cache->interval  = fc_window;

    // Create data buffer to pass to flow control function. Number of payload
    // words is arbitrary, just has to fit in the buffer.
    vrt::if_packet_info_t packet_info;
    packet_info.packet_type         = vrt::if_packet_info_t::PACKET_TYPE_DATA;
    packet_info.num_payload_words32 = 100;
    packet_info.num_payload_bytes   = packet_info.num_payload_words32 * sizeof(uint32_t);
    packet_info.has_tsf             = false;

    std::vector<uint32_t> recv_data(packet_info.num_payload_words32, 0);
    xport->push_back_recv_packet(packet_info, recv_data);

    auto recv_buffer = xport->get_recv_buff(1.0);

    // Run benchmark
    const auto start_time = std::chrono::steady_clock::now();

    constexpr size_t iterations = 1e7;

    for (size_t i = 0; i < iterations; i++) {
        fc_cache->total_bytes_consumed = send_flow_control_packet ? fc_window : 0;
        fc_cache->last_byte_count      = 0;

        rx_flow_ctrl(fc_cache, recv_buffer);
    }

    const auto end_time = std::chrono::steady_clock::now();
    const std::chrono::duration<double> elapsed_time(end_time - start_time);

    std::cout << elapsed_time.count() / iterations * 1e9 << " ns per call\n";
}

void benchmark_device3_handle_rx_flow_ctrl_ack()
{
    // Arbitrary sizes
    constexpr uint32_t fc_window = 10000;

    mock_zero_copy::sptr xport(new mock_zero_copy(vrt::if_packet_info_t::LINK_TYPE_CHDR));

    xport->set_reuse_recv_memory(true);
    xport->set_reuse_send_memory(true);

    boost::shared_ptr<rx_fc_cache_t> fc_cache(new rx_fc_cache_t());
    fc_cache->to_host              = uhd::ntohx<uint32_t>;
    fc_cache->from_host            = uhd::htonx<uint32_t>;
    fc_cache->pack                 = vrt::chdr::if_hdr_pack_be;
    fc_cache->unpack               = vrt::chdr::if_hdr_unpack_be;
    fc_cache->xport                = xport;
    fc_cache->interval             = fc_window;
    fc_cache->total_bytes_consumed = 100;

    // Payload should contain packet count and byte count
    std::vector<uint32_t> payload_data;
    payload_data.push_back(fc_cache->to_host(10)); // packet count
    payload_data.push_back(fc_cache->to_host(100)); // byte count

    // Run benchmark
    const auto start_time       = std::chrono::steady_clock::now();
    constexpr size_t iterations = 1e7;

    for (size_t i = 0; i < iterations; i++) {
        handle_rx_flowctrl_ack(fc_cache, payload_data.data());
    }

    const auto end_time = std::chrono::steady_clock::now();
    const std::chrono::duration<double> elapsed_time(end_time - start_time);

    std::cout << elapsed_time.count() / iterations * 1e9 << " ns per call\n";
}

void benchmark_device3_tx_flow_ctrl(bool send_flow_control_packet)
{
    // Arbitrary sizes
    constexpr uint32_t fc_window = 10000;

    mock_zero_copy::sptr xport(new mock_zero_copy(vrt::if_packet_info_t::LINK_TYPE_CHDR));

    xport->set_reuse_recv_memory(true);

    boost::shared_ptr<tx_fc_cache_t> fc_cache(new tx_fc_cache_t(fc_window));

    fc_cache->to_host   = uhd::ntohx<uint32_t>;
    fc_cache->from_host = uhd::htonx<uint32_t>;
    fc_cache->pack      = vrt::chdr::if_hdr_pack_be;
    fc_cache->unpack    = vrt::chdr::if_hdr_unpack_be;

    xport->push_back_flow_ctrl_packet(
        vrt::if_packet_info_t::PACKET_TYPE_FC, 1 /*packet*/, fc_window /*bytes*/);

    // Run benchmark
    const auto start_time                 = std::chrono::steady_clock::now();
    constexpr size_t iterations           = 1e7;
    managed_send_buffer::sptr send_buffer = xport->get_send_buff(0.0);

    for (size_t i = 0; i < iterations; i++) {
        fc_cache->byte_count    = send_flow_control_packet ? fc_window : 0;
        fc_cache->last_byte_ack = 0;

        tx_flow_ctrl(fc_cache, xport, send_buffer);
    }

    const auto end_time = std::chrono::steady_clock::now();
    const std::chrono::duration<double> elapsed_time(end_time - start_time);

    std::cout << elapsed_time.count() / iterations * 1e9 << " ns per call\n";
}

void benchmark_device3_tx_flow_ctrl_ack()
{
    // Arbitrary sizes
    constexpr uint32_t fc_window = 10000;

    mock_zero_copy::sptr xport(new mock_zero_copy(vrt::if_packet_info_t::LINK_TYPE_CHDR));

    xport->set_reuse_send_memory(true);

    boost::shared_ptr<tx_fc_cache_t> fc_cache(new tx_fc_cache_t(fc_window));

    fc_cache->to_host   = uhd::ntohx<uint32_t>;
    fc_cache->from_host = uhd::htonx<uint32_t>;
    fc_cache->pack      = vrt::chdr::if_hdr_pack_be;
    fc_cache->unpack    = vrt::chdr::if_hdr_unpack_be;

    // Run benchmark
    const auto start_time       = std::chrono::steady_clock::now();
    constexpr size_t iterations = 1e7;
    uhd::sid_t send_sid;

    for (size_t i = 0; i < iterations; i++) {
        // Setup fc_cache to require an ack
        fc_cache->fc_received = true;

        tx_flow_ctrl_ack(fc_cache, xport, send_sid);
    }

    const auto end_time = std::chrono::steady_clock::now();
    const std::chrono::duration<double> elapsed_time(end_time - start_time);

    std::cout << elapsed_time.count() / iterations * 1e9 << " ns per call\n";
}

int UHD_SAFE_MAIN(int argc, char* argv[])
{
    po::options_description desc("Allowed options");
    desc.add_options()("help", "help message");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    // Print the help message
    if (vm.count("help")) {
        std::cout << boost::format("UHD Packet Handler Benchmark %s") % desc << std::endl;
        std::cout
            << "    Benchmark of send and receive packet handlers and flow control\n"
               "    functions. All benchmarks use mock transport objects. No\n"
               "    parameters are needed to run this benchmark.\n"
            << std::endl;
        return EXIT_FAILURE;
    }

    uhd::set_thread_priority_safe();

    const char* formats[]   = {"sc16", "fc32", "fc64"};
    constexpr size_t rx_spp = 2000;
    constexpr size_t tx_spp = 1000;

    std::cout << "----------------------------------------------------------\n";
    std::cout << "Benchmark of recv with no flow control and mock transport \n";
    std::cout << "----------------------------------------------------------\n";
    std::cout << "spp: " << rx_spp << "\n";

    for (size_t i = 0; i < std::extent<decltype(formats)>::value; i++) {
        benchmark_recv_packet_handler(rx_spp, formats[i]);
    }

    std::cout << "\n";

    std::cout << "----------------------------------------------------------\n";
    std::cout << "Benchmark of send with no flow control and mock transport \n";
    std::cout << "----------------------------------------------------------\n";
    std::cout << "spp: " << tx_spp << "\n";

    std::cout << "*** without timespec ***\n";
    for (size_t i = 0; i < std::extent<decltype(formats)>::value; i++) {
        benchmark_send_packet_handler(tx_spp, formats[i], false);
    }
    std::cout << "\n";

    std::cout << "*** with timespec ***\n";
    for (size_t i = 0; i < std::extent<decltype(formats)>::value; i++) {
        benchmark_send_packet_handler(tx_spp, formats[i], true);
    }
    std::cout << "\n";

    std::cout << "----------------------------------------------------------\n";
    std::cout << "  Benchmark of flow control functions with mock transport \n";
    std::cout << "----------------------------------------------------------\n";
    std::cout << "*** device3_tx_flow_ctrl with no flow control packet ***\n";
    benchmark_device3_tx_flow_ctrl(false);
    std::cout << "\n";

    std::cout << "*** device3_tx_flow_ctrl with flow control packet ***\n";
    benchmark_device3_tx_flow_ctrl(true);
    std::cout << "\n";

    std::cout << "*** device3_tx_flow_ctrl_ack ***\n";
    benchmark_device3_tx_flow_ctrl_ack();
    std::cout << "\n";

    std::cout << "*** device3_rx_flow_ctrl with no flow control packet ***\n";
    benchmark_device3_rx_flow_ctrl(false);
    std::cout << "\n";

    std::cout << "*** device3_rx_flow_ctrl with flow control packet ***\n";
    benchmark_device3_rx_flow_ctrl(true);
    std::cout << "\n";

    std::cout << "*** device3_handle_rx_flow_ctrl_ack ***\n";
    benchmark_device3_handle_rx_flow_ctrl_ack();
    std::cout << "\n";

    return EXIT_SUCCESS;
}
