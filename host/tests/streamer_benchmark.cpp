//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "../common/mock_link.hpp"
#include <uhd/rfnoc/chdr_types.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhdlib/rfnoc/chdr_rx_data_xport.hpp>
#include <uhdlib/rfnoc/chdr_tx_data_xport.hpp>
#include <uhdlib/transport/inline_io_service.hpp>
#include <uhdlib/transport/rx_streamer_impl.hpp>
#include <uhdlib/transport/tx_streamer_impl.hpp>
#include <boost/program_options.hpp>
#include <chrono>
#include <iostream>
#include <memory>
#include <vector>

namespace po = boost::program_options;
using namespace uhd;
using namespace uhd::rfnoc;
using namespace uhd::transport;

static const double TICK_RATE    = 100e6;
static const double SAMP_RATE    = 10e6;
static const double SCALE_FACTOR = 2;

/*!
 * Mock rx data xport which doesn't do anything, doesn't use I/O services or
 * links, to benchmark rx_streamer_impl with minimal other overhead.
 */
class mock_rx_data_xport
{
public:
    using uptr = std::unique_ptr<mock_rx_data_xport>;

    struct buff_t
    {
        using uptr = std::unique_ptr<buff_t>;
        std::vector<uint8_t> data;
    };

    struct packet_info_t
    {
        bool eob             = false;
        bool eov             = false;
        bool has_tsf         = false;
        uint64_t tsf         = 0;
        size_t payload_bytes = 0;
        const void* payload  = nullptr;
    };

    mock_rx_data_xport(const size_t buff_size) : _buff_size(buff_size)
    {
        _buff = std::make_unique<buff_t>();
        _buff->data.resize(buff_size);

        _packet_info.has_tsf       = true;
        _packet_info.tsf           = 1000;
        _packet_info.payload_bytes = buff_size;
        _packet_info.payload       = _buff->data.data();
    }

    std::tuple<buff_t::uptr, packet_info_t, bool> get_recv_buff(
        const int32_t /*timeout_ms*/)
    {
        const bool seq_error = false;

        return std::make_tuple(std::move(_buff), _packet_info, seq_error);
    }

    void release_recv_buff(buff_t::uptr buff)
    {
        _buff = std::move(buff);
    }

    size_t get_mtu() const
    {
        return _buff_size;
    }

    size_t get_chdr_hdr_len() const
    {
        return sizeof(packet_info_t);
    }

    size_t get_max_payload_size() const
    {
        return get_mtu() - sizeof(packet_info_t);
    }

private:
    size_t _buff_size;
    buff_t::uptr _buff;
    packet_info_t _packet_info;
};

/*!
 * Mock tx data xport which doesn't do anything, doesn't use I/O services or
 * links, to benchmark tx_streamer_impl with minimal other overhead.
 */
class mock_tx_data_xport
{
public:
    using uptr = std::unique_ptr<mock_tx_data_xport>;

    struct buff_t
    {
        using uptr = std::unique_ptr<buff_t>;
        std::vector<uint8_t> data;

        void set_packet_size(const size_t) {}
    };

    struct packet_info_t
    {
        bool eob             = false;
        bool eov             = false;
        bool has_tsf         = false;
        uint64_t tsf         = 0;
        size_t payload_bytes = 0;
    };

    mock_tx_data_xport(const size_t buff_size) : _buff_size(buff_size)
    {
        _buff = std::make_unique<buff_t>();
        _buff->data.resize(buff_size);
    }

    buff_t::uptr get_send_buff(const int32_t /*timeout_ms*/)
    {
        return std::move(_buff);
    }

    std::pair<void*, size_t> write_packet_header(
        buff_t::uptr& buff, const packet_info_t& info)
    {
        uint8_t* data                             = buff->data.data();
        *(reinterpret_cast<packet_info_t*>(data)) = info;
        return std::make_pair(data + sizeof(info), sizeof(info) + info.payload_bytes);
    }

    void release_send_buff(buff_t::uptr buff)
    {
        _buff = std::move(buff);
    }

    size_t get_mtu() const
    {
        return _buff_size;
    }

    size_t get_chdr_hdr_len() const
    {
        return sizeof(packet_info_t);
    }

    size_t get_max_payload_size() const
    {
        return get_mtu() - sizeof(packet_info_t);
    }


private:
    size_t _buff_size;
    buff_t::uptr _buff;
};

/*!
 * Mock rx streamer, configured to ignore sequence errors
 */
template <typename xport>
class mock_rx_streamer : public rx_streamer_impl<xport, true>
{
public:
    mock_rx_streamer(const size_t num_chans, const uhd::stream_args_t& stream_args)
        : rx_streamer_impl<xport, true>(num_chans, stream_args)
    {
    }

    void set_tick_rate(double rate)
    {
        rx_streamer_impl<xport, true>::set_tick_rate(rate);
    }

    void set_samp_rate(double rate)
    {
        rx_streamer_impl<xport, true>::set_samp_rate(rate);
    }

    void set_scale_factor(const size_t chan, const double scale_factor)
    {
        rx_streamer_impl<xport, true>::set_scale_factor(chan, scale_factor);
    }

    void issue_stream_cmd(const stream_cmd_t& /*stream_cmd*/) override {}
};

/*!
 * Mock tx streamer
 */
template <typename xport>
class mock_tx_streamer : public tx_streamer_impl<xport>
{
public:
    mock_tx_streamer(const size_t num_chans, const uhd::stream_args_t& stream_args)
        : tx_streamer_impl<xport>(num_chans, stream_args)
    {
    }

    void set_tick_rate(double rate)
    {
        tx_streamer_impl<xport>::set_tick_rate(rate);
    }

    void set_samp_rate(double rate)
    {
        tx_streamer_impl<xport>::set_samp_rate(rate);
    }

    void set_scale_factor(const size_t chan, const double scale_factor)
    {
        tx_streamer_impl<xport>::set_scale_factor(chan, scale_factor);
    }

    bool recv_async_msg(
        uhd::async_metadata_t& /*async_metadata*/, double /*timeout = 0.1*/) override
    {
        return false;
    }
};

using rx_streamer_mock_xport = mock_rx_streamer<mock_rx_data_xport>;
using tx_streamer_mock_xport = mock_tx_streamer<mock_tx_data_xport>;
using rx_streamer_mock_link  = mock_rx_streamer<chdr_rx_data_xport>;
using tx_streamer_mock_link  = mock_tx_streamer<chdr_tx_data_xport>;

/*!
 * Helper functions
 */
static std::shared_ptr<rx_streamer_mock_xport> make_rx_streamer_mock_xport(
    const size_t spp, const std::string& format)
{
    const uhd::stream_args_t stream_args(format, "sc16");
    auto streamer = std::make_shared<rx_streamer_mock_xport>(1, stream_args);
    streamer->set_tick_rate(TICK_RATE);
    streamer->set_samp_rate(SAMP_RATE);
    streamer->set_scale_factor(0, SCALE_FACTOR);

    const size_t bpi        = convert::get_bytes_per_item(format);
    const size_t frame_size = bpi * spp;

    auto xport = std::make_unique<mock_rx_data_xport>(frame_size);

    streamer->connect_channel(0, std::move(xport));

    return streamer;
}

static std::shared_ptr<tx_streamer_mock_xport> make_tx_streamer_mock_xport(
    const size_t spp, const std::string& format)
{
    const uhd::stream_args_t stream_args(format, "sc16");
    auto streamer = std::make_shared<tx_streamer_mock_xport>(1, stream_args);
    streamer->set_tick_rate(TICK_RATE);
    streamer->set_samp_rate(SAMP_RATE);
    streamer->set_scale_factor(0, SCALE_FACTOR);

    const size_t bpi        = convert::get_bytes_per_item(format);
    const size_t frame_size = bpi * spp + sizeof(mock_tx_data_xport::packet_info_t);

    auto xport = std::make_unique<mock_tx_data_xport>(frame_size);

    streamer->connect_channel(0, std::move(xport));

    return streamer;
}

static std::shared_ptr<rx_streamer_mock_link> make_rx_streamer_mock_link(
    const size_t spp, const std::string& format)
{
    const uhd::stream_args_t stream_args(format, "sc16");
    auto streamer = std::make_shared<rx_streamer_mock_link>(1, stream_args);
    streamer->set_tick_rate(TICK_RATE);
    streamer->set_samp_rate(SAMP_RATE);
    streamer->set_scale_factor(0, SCALE_FACTOR);

    const chdr::chdr_packet_factory pkt_factory(CHDR_W_64, ENDIANNESS_BIG);
    const sep_id_pair_t epids                = {0, 1};
    const stream_buff_params_t buff_capacity = {UINT64_MAX, UINT32_MAX};
    const stream_buff_params_t fc_freq       = {UINT64_MAX, UINT32_MAX};
    const chdr_rx_data_xport::fc_params_t fc_params{buff_capacity, fc_freq};

    const size_t bpi        = convert::get_bytes_per_item(format);
    const size_t frame_size = bpi * spp + 16;

    const mock_recv_link::link_params recv_params = {frame_size, 1};
    const mock_send_link::link_params send_params = {frame_size, 1};

    auto recv_link = std::make_shared<mock_recv_link>(recv_params, true);
    auto send_link = std::make_shared<mock_send_link>(send_params, true);

    boost::shared_array<uint8_t> recv_frame(new uint8_t[frame_size]);
    auto pkt = pkt_factory.make_generic();
    chdr::chdr_header header;
    header.set_pkt_type(chdr::PKT_TYPE_DATA_WITH_TS);
    header.set_length(frame_size);
    header.set_dst_epid(epids.second);
    pkt->refresh(recv_frame.get(), header, 1000 /*tsf*/);

    recv_link->push_back_recv_packet(recv_frame, frame_size);

    auto io_srv = inline_io_service::make();
    io_srv->attach_recv_link(recv_link);
    io_srv->attach_send_link(send_link);

    auto xport = std::make_unique<chdr_rx_data_xport>(io_srv,
        recv_link,
        send_link,
        pkt_factory,
        epids,
        send_link->get_num_send_frames(),
        fc_params,
        [io_srv = io_srv, recv_link, send_link]() {
            io_srv->detach_recv_link(recv_link);
            io_srv->detach_send_link(send_link);
        });

    streamer->connect_channel(0, std::move(xport));
    return streamer;
}

static std::shared_ptr<tx_streamer_mock_link> make_tx_streamer_mock_link(
    const size_t spp, const std::string& format)
{
    const uhd::stream_args_t stream_args(format, "sc16");
    auto streamer = std::make_shared<tx_streamer_mock_link>(1, stream_args);
    streamer->set_tick_rate(TICK_RATE);
    streamer->set_samp_rate(SAMP_RATE);
    streamer->set_scale_factor(0, SCALE_FACTOR);

    const chdr::chdr_packet_factory pkt_factory(CHDR_W_64, ENDIANNESS_BIG);
    const sep_id_pair_t epids                = {0, 1};
    const stream_buff_params_t buff_capacity = {UINT64_MAX, UINT32_MAX};
    const chdr_tx_data_xport::fc_params_t fc_params{buff_capacity};

    const size_t bpi        = convert::get_bytes_per_item(format);
    const size_t frame_size = bpi * spp + 16;

    const mock_recv_link::link_params recv_params = {frame_size, 1};
    const mock_send_link::link_params send_params = {frame_size, 1};

    auto recv_link = std::make_shared<mock_recv_link>(recv_params, true);
    auto send_link = std::make_shared<mock_send_link>(send_params, true);

    auto io_srv = inline_io_service::make();
    io_srv->attach_recv_link(recv_link);
    io_srv->attach_send_link(send_link);

    auto xport = std::make_unique<chdr_tx_data_xport>(io_srv,
        recv_link,
        send_link,
        pkt_factory,
        epids,
        send_link->get_num_send_frames(),
        fc_params,
        [io_srv = io_srv, recv_link, send_link]() {
            io_srv->detach_recv_link(recv_link);
            io_srv->detach_send_link(send_link);
        });

    streamer->connect_channel(0, std::move(xport));
    return streamer;
}

/*!
 * Benchmark of rx streamer
 */
void benchmark_rx_streamer(
    rx_streamer::sptr streamer, const size_t spp, const std::string& format)
{
    // Allocate buffer
    const size_t bpi = convert::get_bytes_per_item(format);
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

/*!
 * Benchmark of tx streamer
 */
void benchmark_tx_streamer(tx_streamer::sptr streamer,
    const size_t spp,
    const std::string& format,
    bool use_time_spec)
{
    // Allocate buffer
    const size_t bpi = convert::get_bytes_per_item(format);
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
        std::cout << boost::format("UHD Streamer Benchmark %s") % desc << std::endl;
        std::cout << "    Benchmark of send and receive streamer functions\n"
                     "    All benchmarks use mock transport objects. No\n"
                     "    parameters are needed to run this benchmark.\n"
                  << std::endl;
        return EXIT_FAILURE;
    }

    const char* formats[] = {"sc16", "fc32", "fc64"};
    constexpr size_t spp  = 1000;
    std::cout << "spp: " << spp << "\n";

    std::cout << "----------------------------------------------------------\n";
    std::cout << "Benchmark of recv with mock transport                     \n";
    std::cout << "                                                          \n";
    std::cout << "   Measures time spent in the rx streamer only.           \n";
    std::cout << "----------------------------------------------------------\n";

    std::cout << "*** with timespec ***\n";
    for (size_t i = 0; i < std::extent<decltype(formats)>::value; i++) {
        auto streamer = make_rx_streamer_mock_xport(spp, formats[i]);
        benchmark_rx_streamer(streamer, spp, formats[i]);
    }

    std::cout << "\n";
    std::cout << "----------------------------------------------------------\n";
    std::cout << "Benchmark of send with mock transport                     \n";
    std::cout << "                                                          \n";
    std::cout << "   Measures time time spent in the tx streamer only.      \n";
    std::cout << "----------------------------------------------------------\n";

    std::cout << "*** without timespec ***\n";
    for (size_t i = 0; i < std::extent<decltype(formats)>::value; i++) {
        auto streamer = make_tx_streamer_mock_xport(spp, formats[i]);
        benchmark_tx_streamer(streamer, spp, formats[i], false);
    }
    std::cout << "\n";

    std::cout << "*** with timespec ***\n";
    for (size_t i = 0; i < std::extent<decltype(formats)>::value; i++) {
        auto streamer = make_tx_streamer_mock_xport(spp, formats[i]);
        benchmark_tx_streamer(streamer, spp, formats[i], true);
    }
    std::cout << "\n";

    std::cout << "----------------------------------------------------------\n";
    std::cout << "Benchmark of recv with mock link                          \n";
    std::cout << "                                                          \n";
    std::cout << "   Measures time time spent in the rx streamer, I/O       \n";
    std::cout << "   service, and chdr data xport.                          \n";
    std::cout << "----------------------------------------------------------\n";

    std::cout << "*** with timespec ***\n";
    for (size_t i = 0; i < std::extent<decltype(formats)>::value; i++) {
        auto streamer = make_rx_streamer_mock_link(spp, formats[i]);
        benchmark_rx_streamer(streamer, spp, formats[i]);
    }
    std::cout << "\n";

    std::cout << "----------------------------------------------------------\n";
    std::cout << "Benchmark of send with mock link                          \n";
    std::cout << "                                                          \n";
    std::cout << "   Measures time time spent in the tx streamer, I/O       \n";
    std::cout << "   service, and chdr data xport.                          \n";
    std::cout << "----------------------------------------------------------\n";

    std::cout << "*** without timespec ***\n";
    for (size_t i = 0; i < std::extent<decltype(formats)>::value; i++) {
        auto streamer = make_tx_streamer_mock_link(spp, formats[i]);
        benchmark_tx_streamer(streamer, spp, formats[i], false);
    }
    std::cout << "\n";

    std::cout << "*** with timespec ***\n";
    for (size_t i = 0; i < std::extent<decltype(formats)>::value; i++) {
        auto streamer = make_tx_streamer_mock_link(spp, formats[i]);
        benchmark_tx_streamer(streamer, spp, formats[i], true);
    }
    std::cout << "\n";

    return EXIT_SUCCESS;
}
