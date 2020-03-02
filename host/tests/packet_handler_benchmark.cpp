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
#include "common/mock_zero_copy.hpp"
#include <uhd/convert.hpp>
#include <uhd/transport/chdr.hpp>
#include <uhd/transport/zero_copy.hpp>
#include <uhd/transport/zero_copy_flow_ctrl.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/utils/thread.hpp>
#include <boost/program_options.hpp>
#include <chrono>
#include <vector>

namespace po = boost::program_options;
using namespace uhd::transport;

static constexpr size_t MAX_HEADER_LEN = 16;
static constexpr size_t LINE_SIZE      = 8;

//
// Old device3 rx flow control cache and procedures
//
struct rx_fc_cache_t
{
    //! Flow control interval in bytes
    size_t interval = 0;
    //! Byte count at last flow control packet
    uint32_t last_byte_count = 0;
    //! This will wrap around, but that's OK, because math.
    uint32_t total_bytes_consumed = 0;
    //! This will wrap around, but that's OK, because math.
    uint32_t total_packets_consumed = 0;
    //! Sequence number of next flow control packet
    uint64_t seq_num = 0;
    uhd::transport::zero_copy_if::sptr xport;
    std::function<uint32_t(uint32_t)> to_host;
    std::function<uint32_t(uint32_t)> from_host;
    std::function<void(
        const uint32_t* packet_buff, uhd::transport::vrt::if_packet_info_t&)>
        unpack;
    std::function<void(uint32_t* packet_buff, uhd::transport::vrt::if_packet_info_t&)>
        pack;
};

inline bool rx_flow_ctrl(
    std::shared_ptr<rx_fc_cache_t> fc_cache, uhd::transport::managed_buffer::sptr buff)
{
    // If the caller supplied a buffer
    if (buff) {
        // Unpack the header
        uhd::transport::vrt::if_packet_info_t packet_info;
        packet_info.num_packet_words32 = buff->size() / sizeof(uint32_t);
        const uint32_t* pkt            = buff->cast<const uint32_t*>();
        try {
            fc_cache->unpack(pkt, packet_info);
        } catch (const std::exception& ex) {
            // Log and ignore
            UHD_LOGGER_ERROR("RX FLOW CTRL")
                << "Error unpacking packet: " << ex.what() << std::endl;
            return true;
        }

        // Update counters assuming the buffer is a consumed packet
        if (not packet_info.error) {
            const size_t bytes =
                4 * (packet_info.num_header_words32 + packet_info.num_payload_words32);
            fc_cache->total_bytes_consumed += bytes;
            fc_cache->total_packets_consumed++;
        }
    }

    // Just return if there is no need to send a flow control packet
    if (fc_cache->total_bytes_consumed - fc_cache->last_byte_count < fc_cache->interval) {
        return true;
    }

    // Time to send a flow control packet. For the benchmark, we should never
    // reach this point.
    UHD_THROW_INVALID_CODE_PATH();
}

inline void handle_rx_flowctrl_ack(
    std::shared_ptr<rx_fc_cache_t> /*fc_cache*/, const uint32_t* /*payload*/)
{
    // For the benchmark, we should never reach this
    UHD_THROW_INVALID_CODE_PATH();
}

//
// Old device3 tx flow control cache and procedures
//
struct tx_fc_cache_t
{
    uint32_t last_byte_ack = 0;
    uint32_t last_seq_ack  = 0;
    uint32_t byte_count    = 0;
    uint32_t pkt_count     = 0;
    uint32_t window_size   = 0;
    uint32_t fc_ack_seqnum = 0;
    bool fc_received       = false;
    std::function<uint32_t(uint32_t)> to_host;
    std::function<uint32_t(uint32_t)> from_host;
    std::function<void(
        const uint32_t* packet_buff, uhd::transport::vrt::if_packet_info_t&)>
        unpack;
    std::function<void(uint32_t* packet_buff, uhd::transport::vrt::if_packet_info_t&)>
        pack;
};

inline bool tx_flow_ctrl(std::shared_ptr<tx_fc_cache_t> fc_cache,
    uhd::transport::zero_copy_if::sptr /*xport*/,
    uhd::transport::managed_buffer::sptr buff)
{
    while (true) {
        // If there is space
        if (fc_cache->window_size - (fc_cache->byte_count - fc_cache->last_byte_ack)
            >= buff->size()) {
            // All is good - packet will be sent
            fc_cache->byte_count += buff->size();
            // Round up to nearest word
            if (fc_cache->byte_count % LINE_SIZE) {
                fc_cache->byte_count += LINE_SIZE - (fc_cache->byte_count % LINE_SIZE);
            }
            fc_cache->pkt_count++;

            // Just zero out the counts here to avoid actually tring to read flow
            // control packets in the benchmark
            fc_cache->byte_count    = 0;
            fc_cache->last_byte_ack = 0;
            fc_cache->pkt_count     = 0;

            return true;
        }

        // Look for a flow control message to update the space available in the
        // buffer. For the benchmark, we should never reach this point.
        UHD_THROW_INVALID_CODE_PATH();
    }
    return false;
}

inline void tx_flow_ctrl_ack(std::shared_ptr<tx_fc_cache_t> fc_cache,
    uhd::transport::zero_copy_if::sptr /*send_xport*/)
{
    if (not fc_cache->fc_received) {
        return;
    }

    // Time to send a flow control ACK packet. For the benchmark, we should
    // never reach this point.
    UHD_THROW_INVALID_CODE_PATH();
}

//
// Benchmark functions
//
void benchmark_recv_packet_handler(const size_t spp, const std::string& format)
{
    const size_t bpi        = uhd::convert::get_bytes_per_item(format);
    const size_t frame_size = bpi * spp + MAX_HEADER_LEN;

    mock_zero_copy::sptr xport(new mock_zero_copy(
        vrt::if_packet_info_t::LINK_TYPE_CHDR, frame_size, frame_size));

    // Create packet for packet handler to read
    vrt::if_packet_info_t packet_info;
    packet_info.packet_type         = vrt::if_packet_info_t::PACKET_TYPE_DATA;
    packet_info.num_payload_words32 = spp;
    packet_info.num_payload_bytes   = packet_info.num_payload_words32 * sizeof(uint32_t);
    packet_info.has_tsf             = true;
    packet_info.tsf                 = 1;

    std::vector<uint32_t> recv_data(spp, 0);
    xport->push_back_recv_packet(packet_info, recv_data);
    xport->set_reuse_recv_memory(true);

    // Configure xport flow control
    std::shared_ptr<rx_fc_cache_t> fc_cache(new rx_fc_cache_t());
    fc_cache->to_host   = uhd::ntohx<uint32_t>;
    fc_cache->from_host = uhd::htonx<uint32_t>;
    fc_cache->pack      = vrt::chdr::if_hdr_pack_be;
    fc_cache->unpack    = vrt::chdr::if_hdr_unpack_be;
    fc_cache->xport     = xport;
    fc_cache->interval  = std::numeric_limits<std::size_t>::max();

    auto zero_copy_xport = zero_copy_flow_ctrl::make(xport,
        0,
        [fc_cache](managed_buffer::sptr buff) { return rx_flow_ctrl(fc_cache, buff); });

    // Create streamer
    auto streamer = std::make_shared<sph::recv_packet_streamer>(spp);
    streamer->set_tick_rate(1.0);
    streamer->set_samp_rate(1.0);

    // Configure streamer xport
    streamer->set_vrt_unpacker(&vrt::chdr::if_hdr_unpack_be);
    streamer->set_xport_chan_get_buff(0,
        [zero_copy_xport](
            double timeout) { return zero_copy_xport->get_recv_buff(timeout); },
        false // flush
    );

    // Configure flow control ack
    streamer->set_xport_handle_flowctrl_ack(0, [fc_cache](const uint32_t* payload) {
        handle_rx_flowctrl_ack(fc_cache, payload);
    });

    // Configure converter
    uhd::convert::id_type id;
    id.output_format = format;
    id.num_inputs    = 1;
    id.input_format  = "sc16_item32_be";
    id.num_outputs   = 1;
    streamer->set_converter(id);

    // Allocate buffer
    std::vector<uint8_t> buffer(spp * bpi);
    std::vector<void*> buffers;
    buffers.push_back(buffer.data());

    // Run benchmark
    uhd::rx_metadata_t md;
    const auto start_time   = std::chrono::steady_clock::now();
    const size_t iterations = 1e7;

    for (size_t i = 0; i < iterations; i++) {
        streamer->recv(buffers, spp, md, 1.0, true);
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
    const size_t frame_size = bpi * spp + MAX_HEADER_LEN;

    mock_zero_copy::sptr xport(new mock_zero_copy(
        vrt::if_packet_info_t::LINK_TYPE_CHDR, frame_size, frame_size));

    xport->set_reuse_send_memory(true);

    // Configure flow control
    std::shared_ptr<tx_fc_cache_t> fc_cache(new tx_fc_cache_t());
    fc_cache->to_host     = uhd::ntohx<uint32_t>;
    fc_cache->from_host   = uhd::htonx<uint32_t>;
    fc_cache->pack        = vrt::chdr::if_hdr_pack_be;
    fc_cache->unpack      = vrt::chdr::if_hdr_unpack_be;
    fc_cache->window_size = UINT32_MAX;

    auto zero_copy_xport = zero_copy_flow_ctrl::make(xport,
        [fc_cache, xport](
            managed_buffer::sptr buff) { return tx_flow_ctrl(fc_cache, xport, buff); },
        0);

    // Create streamer
    auto streamer = std::make_shared<sph::send_packet_streamer>(spp);
    streamer->set_vrt_packer(&vrt::chdr::if_hdr_pack_be);

    // Configure converter
    uhd::convert::id_type id;
    id.input_format  = format;
    id.num_inputs    = 1;
    id.output_format = "sc16_item32_be";
    id.num_outputs   = 1;
    streamer->set_converter(id);
    streamer->set_enable_trailer(false);

    // Configure streamer xport
    streamer->set_xport_chan_get_buff(0, [zero_copy_xport](double timeout) {
        return zero_copy_xport->get_send_buff(timeout);
    });

    // Configure flow control ack
    streamer->set_xport_chan_post_send_cb(0,
        [fc_cache, zero_copy_xport]() { tx_flow_ctrl_ack(fc_cache, zero_copy_xport); });

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
        streamer->send(buffers, spp, md, 1.0);
    }

    const auto end_time = std::chrono::steady_clock::now();
    const std::chrono::duration<double> elapsed_time(end_time - start_time);
    const double time_per_packet = elapsed_time.count() / iterations;

    std::cout << format << ": " << time_per_packet / spp * 1e9 << " ns/sample, "
              << time_per_packet * 1e9 << " ns/packet\n";
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

    const char* formats[] = {"sc16", "fc32", "fc64"};
    constexpr size_t spp  = 1000;
    std::cout << "spp: " << spp << "\n";

    std::cout << "----------------------------------------------------------\n";
    std::cout << "Benchmark of recv with mock link                          \n";
    std::cout << "----------------------------------------------------------\n";

    for (size_t i = 0; i < std::extent<decltype(formats)>::value; i++) {
        benchmark_recv_packet_handler(spp, formats[i]);
    }

    std::cout << "\n";

    std::cout << "----------------------------------------------------------\n";
    std::cout << "Benchmark of send with mock link                          \n";
    std::cout << "----------------------------------------------------------\n";

    std::cout << "*** without timespec ***\n";
    for (size_t i = 0; i < std::extent<decltype(formats)>::value; i++) {
        benchmark_send_packet_handler(spp, formats[i], false);
    }
    std::cout << "\n";

    std::cout << "*** with timespec ***\n";
    for (size_t i = 0; i < std::extent<decltype(formats)>::value; i++) {
        benchmark_send_packet_handler(spp, formats[i], true);
    }
    std::cout << "\n";

    return EXIT_SUCCESS;
}
