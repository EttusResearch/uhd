//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "../common/mock_link.hpp"
#include <uhdlib/transport/rx_streamer_impl.hpp>
#include <boost/test/unit_test.hpp>
#include <complex>
#include <iostream>
#include <memory>

namespace uhd { namespace transport {

/*!
 * Contents of mock packet header
 */
struct mock_header_t
{
    bool eob             = false;
    bool eov             = false;
    bool has_tsf         = false;
    uint64_t tsf         = 0;
    size_t payload_bytes = 0;
    bool ignore_seq      = true;
    size_t seq_num       = 0;
};

/*!
 * Mock rx data xport which doesn't use I/O service, and just interacts with
 * the link directly.
 */
class mock_rx_data_xport
{
public:
    using uptr   = std::unique_ptr<mock_rx_data_xport>;
    using buff_t = uhd::transport::frame_buff;

    //! Values extracted from received RX data packets
    struct packet_info_t
    {
        bool eob             = false;
        bool eov             = false;
        bool has_tsf         = false;
        uint64_t tsf         = 0;
        size_t payload_bytes = 0;
        const void* payload  = nullptr;
    };

    mock_rx_data_xport(mock_recv_link::sptr recv_link) : _recv_link(recv_link) {}

    std::tuple<frame_buff::uptr, packet_info_t, bool> get_recv_buff(
        const int32_t timeout_ms)
    {
        frame_buff::uptr buff = _recv_link->get_recv_buff(timeout_ms);
        if(buff.get() == nullptr) {
            // No samples available - simulate a timeout for the duration,
            // then return a nullptr for the buffer. This will ultimately
            // return an TIMEOUT to the upper-level receive layers.
            std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
            return std::make_tuple(
                nullptr,
                packet_info_t{},
                false);
        }
        mock_header_t header  = *(reinterpret_cast<mock_header_t*>(buff->data()));

        packet_info_t info;
        info.eob           = header.eob;
        info.eov           = header.eov;
        info.has_tsf       = header.has_tsf;
        info.tsf           = header.tsf;
        info.payload_bytes = header.payload_bytes;
        info.payload = reinterpret_cast<uint8_t*>(buff->data()) + sizeof(mock_header_t);

        const uint8_t* pkt_end =
            reinterpret_cast<uint8_t*>(buff->data()) + buff->packet_size();
        const size_t pyld_pkt_len =
            pkt_end - reinterpret_cast<const uint8_t*>(info.payload);

        if (pyld_pkt_len < info.payload_bytes) {
            _recv_link->release_recv_buff(std::move(buff));
            throw uhd::value_error("Bad header or invalid packet length.");
        }

        const bool seq_match = header.seq_num == _seq_num;
        const bool seq_error = !header.ignore_seq && !seq_match;
        _seq_num             = header.seq_num + 1;

        return std::make_tuple(std::move(buff), info, seq_error);
    }

    void release_recv_buff(frame_buff::uptr buff)
    {
        _recv_link->release_recv_buff(std::move(buff));
    }

    size_t get_mtu() const
    {
        return _recv_link->get_recv_frame_size();
    }

    size_t get_chdr_hdr_len() const
    {
        return sizeof(mock_header_t);
    }

    size_t get_max_payload_size() const
    {
        return get_mtu() - get_chdr_hdr_len();
    }

private:
    mock_recv_link::sptr _recv_link;
    size_t _seq_num = 0;
};

/*!
 * Mock rx streamer for testing
 */
class mock_rx_streamer : public rx_streamer_impl<mock_rx_data_xport>
{
public:
    mock_rx_streamer(const size_t num_chans, const uhd::stream_args_t& stream_args)
        : rx_streamer_impl(num_chans, stream_args)
    {
    }

    void issue_stream_cmd(const stream_cmd_t&) override {}

    void set_tick_rate(double rate)
    {
        rx_streamer_impl::set_tick_rate(rate);
    }

    void set_samp_rate(double rate)
    {
        rx_streamer_impl::set_samp_rate(rate);
    }

    void set_scale_factor(const size_t chan, const double scale_factor)
    {
        rx_streamer_impl::set_scale_factor(chan, scale_factor);
    }
};

}} // namespace uhd::transport

using namespace uhd::transport;

using rx_streamer = rx_streamer_impl<mock_rx_data_xport>;

static const double TICK_RATE    = 100e6;
static const double SAMP_RATE    = 10e6;
static const size_t FRAME_SIZE   = 1000;
static const double SCALE_FACTOR = 2;

/*!
 * Helper functions
 */
static std::vector<mock_recv_link::sptr> make_links(const size_t num)
{
    const mock_recv_link::link_params params = {FRAME_SIZE, 1};

    std::vector<mock_recv_link::sptr> links;

    for (size_t i = 0; i < num; i++) {
        links.push_back(std::make_shared<mock_recv_link>(params));
    }

    return links;
}

static std::shared_ptr<mock_rx_streamer> make_rx_streamer(
    std::vector<mock_recv_link::sptr> recv_links,
    const std::string& host_format,
    const std::string& otw_format = "sc16")
{
    const uhd::stream_args_t stream_args(host_format, otw_format);
    auto streamer = std::make_shared<mock_rx_streamer>(recv_links.size(), stream_args);
    streamer->set_tick_rate(TICK_RATE);
    streamer->set_samp_rate(SAMP_RATE);

    for (size_t i = 0; i < recv_links.size(); i++) {
        mock_rx_data_xport::uptr xport(
            std::make_unique<mock_rx_data_xport>(recv_links[i]));

        streamer->set_scale_factor(i, SCALE_FACTOR);
        streamer->connect_channel(i, std::move(xport));
    }

    return streamer;
}

static void push_back_recv_packet(mock_recv_link::sptr recv_link,
    mock_header_t header,
    size_t num_samps,
    uint16_t start_data = 0)
{
    // Allocate buffer
    const size_t pyld_bytes = num_samps * sizeof(std::complex<uint16_t>);
    const size_t buff_len   = sizeof(header) + pyld_bytes;
    boost::shared_array<uint8_t> data(new uint8_t[buff_len]);

    // Write header to buffer
    header.payload_bytes                            = pyld_bytes;
    *(reinterpret_cast<mock_header_t*>(data.get())) = header;

    // Write data to buffer
    auto data_ptr =
        reinterpret_cast<std::complex<uint16_t>*>(data.get() + sizeof(header));

    for (size_t i = 0; i < num_samps; i++) {
        uint16_t val = (start_data + i) * 2;
        data_ptr[i]  = std::complex<uint16_t>(val, val + 1);
    }

    // Push back buffer for link to recv
    recv_link->push_back_recv_packet(data, buff_len);
}

/*!
 * Tests
 */
BOOST_AUTO_TEST_CASE(test_recv_one_channel_one_packet)
{
    const size_t NUM_PKTS_TO_TEST = 5;
    const std::string format("fc32");

    auto recv_links = make_links(1);
    auto streamer   = make_rx_streamer(recv_links, format);

    const size_t num_samps = 20;
    std::vector<std::complex<float>> buff(num_samps);
    uhd::rx_metadata_t metadata;

    for (size_t i = 0; i < NUM_PKTS_TO_TEST; i++) {
        const bool even_iteration = (i % 2 == 0);
        const bool odd_iteration  = (i % 2 != 0);
        mock_header_t header;
        header.eob     = even_iteration;
        header.has_tsf = odd_iteration;
        header.tsf     = i;
        push_back_recv_packet(recv_links[0], header, num_samps);

        std::cout << "receiving packet " << i << std::endl;

        size_t num_samps_ret =
            streamer->recv(buff.data(), buff.size(), metadata, 1.0, false);

        BOOST_CHECK_EQUAL(num_samps_ret, num_samps);
        BOOST_CHECK_EQUAL(metadata.end_of_burst, even_iteration);
        BOOST_CHECK_EQUAL(metadata.has_time_spec, odd_iteration);
        BOOST_CHECK_EQUAL(metadata.time_spec.to_ticks(TICK_RATE), i);

        for (size_t j = 0; j < num_samps; j++) {
            const auto value =
                std::complex<float>((j * 2) * SCALE_FACTOR, (j * 2 + 1) * SCALE_FACTOR);
            BOOST_CHECK_EQUAL(value, buff[j]);
        }
    }
}

BOOST_AUTO_TEST_CASE(test_recv_one_channel_multi_packet)
{
    const size_t NUM_BUFFS_TO_TEST = 5;
    const std::string format("fc64");

    auto recv_links = make_links(1);
    auto streamer   = make_rx_streamer(recv_links, format);

    const size_t spp       = streamer->get_max_num_samps();
    const size_t num_samps = spp * 4;
    std::vector<std::complex<double>> buff(num_samps);
    uhd::rx_metadata_t metadata;

    for (size_t i = 0; i < NUM_BUFFS_TO_TEST; i++) {
        mock_header_t header;
        header.eob     = false;
        header.has_tsf = true;
        header.tsf     = i;

        size_t samps_written = 0;
        while (samps_written < num_samps) {
            size_t samps_to_write = std::min(num_samps - samps_written, spp);
            push_back_recv_packet(recv_links[0], header, samps_to_write, samps_written);
            samps_written += samps_to_write;
        }

        std::cout << "receiving packet " << i << std::endl;

        size_t num_samps_ret =
            streamer->recv(buff.data(), buff.size(), metadata, 1.0, false);

        BOOST_CHECK_EQUAL(num_samps_ret, num_samps);
        BOOST_CHECK_EQUAL(metadata.end_of_burst, false);
        BOOST_CHECK_EQUAL(metadata.has_time_spec, true);
        BOOST_CHECK_EQUAL(metadata.time_spec.to_ticks(TICK_RATE), i);

        for (size_t j = 0; j < num_samps; j++) {
            const auto value =
                std::complex<double>((j * 2) * SCALE_FACTOR, (j * 2 + 1) * SCALE_FACTOR);
            BOOST_CHECK_EQUAL(value, buff[j]);
        }
    }
}

BOOST_AUTO_TEST_CASE(test_recv_one_channel_multi_packet_with_eob)
{
    // EOB should terminate a multi-packet recv, test that it does
    const std::string format("sc16");

    auto recv_links = make_links(1);
    auto streamer   = make_rx_streamer(recv_links, format);

    const size_t num_packets = 4;
    const size_t spp         = streamer->get_max_num_samps();
    const size_t num_samps   = spp * num_packets;
    std::vector<std::complex<double>> buff(num_samps);
    uhd::rx_metadata_t metadata;

    // Queue 4 packets, with eob set in every other packet
    for (size_t i = 0; i < num_packets; i++) {
        mock_header_t header;
        header.has_tsf = false;
        header.eob     = (i % 2) != 0;
        push_back_recv_packet(recv_links[0], header, spp);
    }

    // Now call recv and check that eob terminates a recv call
    for (size_t i = 0; i < num_packets / 2; i++) {
        std::cout << "receiving packet " << i << std::endl;

        size_t num_samps_ret =
            streamer->recv(buff.data(), buff.size(), metadata, 1.0, false);

        BOOST_CHECK_EQUAL(num_samps_ret, spp * 2);
        BOOST_CHECK_EQUAL(metadata.end_of_burst, true);
        BOOST_CHECK_EQUAL(metadata.has_time_spec, false);
    }
}

BOOST_AUTO_TEST_CASE(test_recv_two_channel_one_packet)
{
    const size_t NUM_PKTS_TO_TEST = 5;
    const std::string format("sc16");

    const size_t num_chans = 2;

    auto recv_links = make_links(num_chans);
    auto streamer   = make_rx_streamer(recv_links, format);

    const size_t num_samps = 20;

    std::vector<std::vector<std::complex<uint16_t>>> buffer(num_chans);
    std::vector<void*> buffers;
    for (size_t i = 0; i < num_chans; i++) {
        buffer[i].resize(num_samps);
        buffers.push_back(&buffer[i].front());
    }

    uhd::rx_metadata_t metadata;

    for (size_t i = 0; i < NUM_PKTS_TO_TEST; i++) {
        const bool even_iteration = (i % 2 == 0);
        const bool odd_iteration  = (i % 2 != 0);
        mock_header_t header;
        header.eob     = even_iteration;
        header.has_tsf = odd_iteration;
        header.tsf     = i;

        size_t samps_pushed = 0;
        for (size_t ch = 0; ch < num_chans; ch++) {
            push_back_recv_packet(recv_links[ch], header, num_samps, samps_pushed);
            samps_pushed += num_samps;
        }

        std::cout << "receiving packet " << i << std::endl;

        size_t num_samps_ret = streamer->recv(buffers, num_samps, metadata, 1.0, false);

        BOOST_CHECK_EQUAL(num_samps_ret, num_samps);
        BOOST_CHECK_EQUAL(metadata.end_of_burst, even_iteration);
        BOOST_CHECK_EQUAL(metadata.has_time_spec, odd_iteration);
        BOOST_CHECK_EQUAL(metadata.time_spec.to_ticks(TICK_RATE), i);

        size_t samps_checked = 0;
        for (size_t ch = 0; ch < num_chans; ch++) {
            for (size_t samp = 0; samp < num_samps; samp++) {
                const size_t n   = samps_checked + samp;
                const auto value = std::complex<uint16_t>((n * 2), (n * 2 + 1));
                BOOST_CHECK_EQUAL(value, buffer[ch][samp]);
            }
            samps_checked += num_samps;
        }
    }
}

BOOST_AUTO_TEST_CASE(test_recv_one_channel_packet_fragment)
{
    const size_t NUM_PKTS_TO_TEST = 5;
    const std::string format("fc32");

    auto recv_links = make_links(1);
    auto streamer   = make_rx_streamer(recv_links, format);

    // Push back five packets, then read them 1/4 of a packet at a time
    const size_t spp              = streamer->get_max_num_samps();
    const size_t reads_per_packet = 4;
    const size_t num_samps        = spp / reads_per_packet;
    for (size_t i = 0; i < NUM_PKTS_TO_TEST; i++) {
        mock_header_t header;
        header.eob     = true;
        header.has_tsf = true;
        header.tsf     = 0;
        push_back_recv_packet(recv_links[0], header, num_samps * reads_per_packet);
    }

    std::vector<std::complex<float>> buff(num_samps);
    uhd::rx_metadata_t metadata;

    for (size_t i = 0; i < NUM_PKTS_TO_TEST; i++) {
        std::cout << "receiving packet " << i << std::endl;

        size_t total_samps_read = 0;
        for (size_t j = 0; j < reads_per_packet; j++) {
            size_t num_samps_ret =
                streamer->recv(buff.data(), buff.size(), metadata, 1.0, false);

            BOOST_CHECK_EQUAL(num_samps_ret, num_samps);
            BOOST_CHECK_EQUAL(metadata.has_time_spec, true);
            BOOST_CHECK_EQUAL(metadata.end_of_burst, true);
            BOOST_CHECK_EQUAL(metadata.more_fragments, j != reads_per_packet - 1);
            BOOST_CHECK_EQUAL(metadata.fragment_offset, total_samps_read);

            const size_t ticks_per_sample = static_cast<size_t>(TICK_RATE / SAMP_RATE);
            const size_t expected_ticks   = ticks_per_sample * total_samps_read;
            BOOST_CHECK_EQUAL(metadata.time_spec.to_ticks(TICK_RATE), expected_ticks);

            for (size_t samp = 0; samp < num_samps; samp++) {
                const size_t pkt_idx = samp + total_samps_read;
                const auto value     = std::complex<float>(
                    (pkt_idx * 2) * SCALE_FACTOR, (pkt_idx * 2 + 1) * SCALE_FACTOR);
                BOOST_CHECK_EQUAL(value, buff[samp]);
            }

            total_samps_read += num_samps_ret;
        }
    }
}

BOOST_AUTO_TEST_CASE(test_recv_seq_error)
{
    // Test that when we get a sequence error the error is returned in the
    // metadata with a time spec that corresponds to the time spec of the
    // last sample in the previous packet plus one sample clock. Test that
    // the packet that causes the sequence error is not discarded.
    const size_t NUM_PKTS_TO_TEST = 2;
    const std::string format("fc32");

    auto recv_links = make_links(1);
    auto streamer   = make_rx_streamer(recv_links, format);

    const size_t num_samps = 20;
    std::vector<std::complex<float>> buff(num_samps);
    uhd::rx_metadata_t metadata;
    size_t seq_num = 0;
    size_t tsf     = 0;

    for (size_t i = 0; i < NUM_PKTS_TO_TEST; i++) {
        mock_header_t header;
        header.eob        = false;
        header.has_tsf    = true;
        header.ignore_seq = false;

        // Push back three packets but skip a seq_num after the second
        header.seq_num = seq_num++;
        header.tsf     = tsf;
        push_back_recv_packet(recv_links[0], header, num_samps);

        tsf += num_samps;
        header.seq_num = seq_num++;
        header.tsf     = tsf;
        push_back_recv_packet(recv_links[0], header, num_samps);

        seq_num++; // dropped packet
        tsf += num_samps;

        header.seq_num = seq_num++;
        header.tsf     = tsf;
        push_back_recv_packet(recv_links[0], header, num_samps);

        // First two reads should succeed
        size_t num_samps_ret =
            streamer->recv(buff.data(), buff.size(), metadata, 1.0, false);
        BOOST_CHECK_EQUAL(num_samps_ret, num_samps);

        num_samps_ret = streamer->recv(buff.data(), buff.size(), metadata, 1.0, false);
        BOOST_CHECK_EQUAL(num_samps_ret, num_samps);
        size_t prev_tsf     = metadata.time_spec.to_ticks(TICK_RATE);
        size_t expected_tsf = prev_tsf + num_samps * (TICK_RATE / SAMP_RATE);

        // Third read should be a sequence error
        num_samps_ret = streamer->recv(buff.data(), buff.size(), metadata, 1.0, false);
        BOOST_CHECK_EQUAL(num_samps_ret, 0);
        BOOST_CHECK_EQUAL(metadata.error_code, uhd::rx_metadata_t::ERROR_CODE_OVERFLOW);
        BOOST_CHECK_EQUAL(metadata.out_of_sequence, true);
        size_t metadata_tsf = metadata.time_spec.to_ticks(TICK_RATE);
        BOOST_CHECK_EQUAL(metadata_tsf, expected_tsf);

        // Next read should succeed
        num_samps_ret = streamer->recv(buff.data(), buff.size(), metadata, 1.0, false);
        BOOST_CHECK_EQUAL(num_samps_ret, num_samps);
        BOOST_CHECK_EQUAL(metadata.error_code, uhd::rx_metadata_t::ERROR_CODE_NONE);
        BOOST_CHECK_EQUAL(metadata.out_of_sequence, false);
    }
}

BOOST_AUTO_TEST_CASE(test_recv_bad_packet)
{
    // Test that when we receive a packet with invalid chdr header or length
    // the streamer returns the correct error in meatadata.
    auto push_back_bad_packet = [](mock_recv_link::sptr recv_link) {
        mock_header_t header;
        header.payload_bytes = 1000;

        // Allocate a buffer that is too small for the payload
        const size_t buff_len = 100;
        boost::shared_array<uint8_t> data(new uint8_t[buff_len]);

        // Write header to buffer
        *(reinterpret_cast<mock_header_t*>(data.get())) = header;

        // Push back buffer for link to recv
        recv_link->push_back_recv_packet(data, buff_len);
    };

    const std::string format("fc32");

    auto recv_links = make_links(1);
    auto streamer   = make_rx_streamer(recv_links, format);

    const size_t num_samps = 20;
    std::vector<std::complex<float>> buff(num_samps);
    uhd::rx_metadata_t metadata;

    mock_header_t header;

    // Push back a regular packet
    push_back_recv_packet(recv_links[0], header, num_samps);

    // Push back a bad packet
    push_back_bad_packet(recv_links[0]);

    // Push back another regular packet
    push_back_recv_packet(recv_links[0], header, num_samps);

    // First read should succeed
    size_t num_samps_ret = streamer->recv(buff.data(), buff.size(), metadata, 1.0, false);
    BOOST_CHECK_EQUAL(num_samps_ret, num_samps);

    // Second read should be an error
    num_samps_ret = streamer->recv(buff.data(), buff.size(), metadata, 1.0, false);
    BOOST_CHECK_EQUAL(num_samps_ret, 0);
    BOOST_CHECK_EQUAL(metadata.error_code, uhd::rx_metadata_t::ERROR_CODE_BAD_PACKET);

    // Third read should succeed
    num_samps_ret = streamer->recv(buff.data(), buff.size(), metadata, 1.0, false);
    BOOST_CHECK_EQUAL(num_samps_ret, num_samps);
    BOOST_CHECK_EQUAL(metadata.error_code, uhd::rx_metadata_t::ERROR_CODE_NONE);
}

BOOST_AUTO_TEST_CASE(test_recv_multi_channel_no_tsf)
{
    // Test that we can receive packets without tsf. Start by pushing
    // a packet with a tsf followed by a few packets without.
    const size_t NUM_PKTS_TO_TEST = 6;
    const std::string format("fc64");

    const size_t num_chans = 10;

    auto recv_links = make_links(num_chans);
    auto streamer   = make_rx_streamer(recv_links, format);

    const size_t num_samps = 21;

    std::vector<std::vector<std::complex<double>>> buffer(num_chans);
    std::vector<void*> buffers;
    for (size_t i = 0; i < num_chans; i++) {
        buffer[i].resize(num_samps);
        buffers.push_back(&buffer[i].front());
    }

    uhd::rx_metadata_t metadata;

    for (size_t i = 0; i < NUM_PKTS_TO_TEST; i++) {
        mock_header_t header;
        header.eob     = (i == NUM_PKTS_TO_TEST - 1);
        header.has_tsf = (i == 0);
        header.tsf     = 500;

        for (size_t ch = 0; ch < num_chans; ch++) {
            push_back_recv_packet(recv_links[ch], header, num_samps);
        }

        size_t num_samps_ret = streamer->recv(buffers, num_samps, metadata, 1.0, false);

        BOOST_CHECK_EQUAL(num_samps_ret, num_samps);
        BOOST_CHECK_EQUAL(metadata.end_of_burst, i == NUM_PKTS_TO_TEST - 1);
        BOOST_CHECK_EQUAL(metadata.has_time_spec, i == 0);
    }
}

BOOST_AUTO_TEST_CASE(test_recv_multi_channel_seq_error)
{
    // Test that the streamer handles dropped packets correctly by injecting
    // a sequence error in one channel. The streamer should discard
    // corresponding packets from all other channels.
    const std::string format("fc64");

    const size_t num_chans = 100;

    auto recv_links = make_links(num_chans);
    auto streamer   = make_rx_streamer(recv_links, format);

    const size_t num_samps = 99;

    std::vector<std::vector<std::complex<double>>> buffer(num_chans);
    std::vector<void*> buffers;
    for (size_t i = 0; i < num_chans; i++) {
        buffer[i].resize(num_samps);
        buffers.push_back(&buffer[i].front());
    }

    for (size_t ch = 0; ch < num_chans; ch++) {
        mock_header_t header;
        header.eob        = false;
        header.has_tsf    = true;
        header.tsf        = 0;
        header.ignore_seq = false;
        header.seq_num    = 0;

        // Drop a packet from an arbitrary channel right at the start
        if (ch != num_chans / 2) {
            push_back_recv_packet(recv_links[ch], header, num_samps);
        }

        // Add a regular packet to check the streamer drops the first
        header.seq_num++;
        header.tsf++;
        push_back_recv_packet(recv_links[ch], header, num_samps);

        // Drop a packet from the first channel
        header.seq_num++;
        header.tsf++;
        if (ch != 0) {
            push_back_recv_packet(recv_links[ch], header, num_samps);
        }

        // Add a regular packet
        header.seq_num++;
        header.tsf++;
        push_back_recv_packet(recv_links[ch], header, num_samps);

        // Drop a few packets from the last channel
        for (size_t j = 0; j < 10; j++) {
            header.seq_num++;
            header.tsf++;
            if (ch != num_chans - 1) {
                push_back_recv_packet(recv_links[ch], header, num_samps);
            }
        }

        // Add a regular packet
        header.seq_num++;
        header.tsf++;
        push_back_recv_packet(recv_links[ch], header, num_samps);
    }

    uhd::rx_metadata_t metadata;

    // First recv should result in error
    size_t num_samps_ret = streamer->recv(buffers, num_samps, metadata, 1.0, false);
    BOOST_CHECK_EQUAL(num_samps_ret, 0);
    BOOST_CHECK_EQUAL(metadata.error_code, uhd::rx_metadata_t::ERROR_CODE_OVERFLOW);
    BOOST_CHECK_EQUAL(metadata.out_of_sequence, true);

    // Packet with tsf == 1 should be returned next
    num_samps_ret = streamer->recv(buffers, num_samps, metadata, 1.0, false);
    BOOST_CHECK_EQUAL(num_samps_ret, num_samps);
    BOOST_CHECK_EQUAL(metadata.time_spec.to_ticks(TICK_RATE), 1);

    // Next recv should result in error
    num_samps_ret = streamer->recv(buffers, num_samps, metadata, 1.0, false);
    BOOST_CHECK_EQUAL(num_samps_ret, 0);
    BOOST_CHECK_EQUAL(metadata.error_code, uhd::rx_metadata_t::ERROR_CODE_OVERFLOW);
    BOOST_CHECK_EQUAL(metadata.out_of_sequence, true);

    // Packet with tsf == 3 should be returned next
    num_samps_ret = streamer->recv(buffers, num_samps, metadata, 1.0, false);
    BOOST_CHECK_EQUAL(num_samps_ret, num_samps);
    BOOST_CHECK_EQUAL(metadata.time_spec.to_ticks(TICK_RATE), 3);

    // Next recv should result in error
    num_samps_ret = streamer->recv(buffers, num_samps, metadata, 1.0, false);
    BOOST_CHECK_EQUAL(num_samps_ret, 0);
    BOOST_CHECK_EQUAL(metadata.error_code, uhd::rx_metadata_t::ERROR_CODE_OVERFLOW);
    BOOST_CHECK_EQUAL(metadata.out_of_sequence, true);

    // Packet with tsf == 14 should be returned next
    num_samps_ret = streamer->recv(buffers, num_samps, metadata, 1.0, false);
    BOOST_CHECK_EQUAL(num_samps_ret, num_samps);
    BOOST_CHECK_EQUAL(metadata.time_spec.to_ticks(TICK_RATE), 14);
}

BOOST_AUTO_TEST_CASE(test_recv_alignment_error)
{
    // Test that the alignment procedure returns an alignment error if it can't
    // time align packets.
    const std::string format("fc64");

    const size_t num_chans = 4;

    auto recv_links = make_links(num_chans);
    auto streamer   = make_rx_streamer(recv_links, format);

    const size_t num_samps = 2;

    std::vector<std::vector<std::complex<double>>> buffer(num_chans);
    std::vector<void*> buffers;
    for (size_t i = 0; i < num_chans; i++) {
        buffer[i].resize(num_samps);
        buffers.push_back(&buffer[i].front());
    }

    uhd::rx_metadata_t metadata;

    mock_header_t header;
    header.eob     = true;
    header.has_tsf = true;
    header.tsf     = 500;

    for (size_t ch = 0; ch < num_chans; ch++) {
        push_back_recv_packet(recv_links[ch], header, num_samps);
    }

    size_t num_samps_ret = streamer->recv(buffers, num_samps, metadata, 1.0, false);

    BOOST_CHECK_EQUAL(num_samps_ret, num_samps);
    BOOST_CHECK_EQUAL(metadata.end_of_burst, true);
    BOOST_CHECK_EQUAL(metadata.has_time_spec, true);

    for (size_t pkt = 0; pkt < uhd::transport::ALIGNMENT_FAILURE_THRESHOLD; pkt++) {
        header.tsf = header.tsf + num_samps;
        for (size_t ch = 0; ch < num_chans; ch++) {
            if (ch == num_chans - 1) {
                // Misalign this time stamp
                header.tsf += 1;
            }
            push_back_recv_packet(recv_links[ch], header, num_samps);
        }
    }

    num_samps_ret = streamer->recv(buffers, num_samps, metadata, 1.0, false);
    BOOST_CHECK_EQUAL(num_samps_ret, 0);
    BOOST_CHECK_EQUAL(metadata.error_code, uhd::rx_metadata_t::ERROR_CODE_ALIGNMENT);
}

BOOST_AUTO_TEST_CASE(test_recv_one_channel_one_eov)
{
    const size_t NUM_PACKETS = 5;
    const std::string format("fc64");

    auto recv_links = make_links(1);
    auto streamer   = make_rx_streamer(recv_links, format);

    const size_t spp       = streamer->get_max_num_samps();
    const size_t num_samps = spp * NUM_PACKETS;
    std::vector<std::complex<double>> buff(num_samps);

    for (size_t i = 0; i < NUM_PACKETS; i++) {
        mock_header_t header;
        header.eob     = false;
        header.has_tsf = true;
        header.tsf     = i;

        for (size_t j = 0; j < NUM_PACKETS; j++) {
            header.eov = (i == j);
            push_back_recv_packet(recv_links[0], header, spp);
        }

        uhd::rx_metadata_t metadata;
        // Create a vector with storage for two EOVs even though we expect
        // only one, since filling the EOV vector results in an early
        // termination of `recv()` (which we don't want here).
        std::vector<size_t> eov_positions(2);
        metadata.eov_positions      = eov_positions.data();
        metadata.eov_positions_size = eov_positions.size();

        std::cout << "receiving packet " << i << std::endl;

        size_t num_samps_ret =
            streamer->recv(buff.data(), buff.size(), metadata, 1.0, false);

        BOOST_CHECK_EQUAL(num_samps_ret, num_samps);
        BOOST_CHECK_EQUAL(metadata.eov_positions, eov_positions.data());
        BOOST_CHECK_EQUAL(metadata.eov_positions_size, eov_positions.size());
        BOOST_CHECK_EQUAL(metadata.eov_positions_count, 1);
        BOOST_CHECK_EQUAL(eov_positions[0], (i + 1) * spp);
    }
}

BOOST_AUTO_TEST_CASE(test_recv_two_channel_aggregate_eov)
{
    const size_t NUM_PACKETS = 20;
    const std::string format("fc64");

    // This vector defines which packets in each channel's mock link will
    // signal EOV in their packet headers.
    //
    // For example, for a vector with 3 values, [3, 5, 8]:
    //   Link 0 packets with EOV: 3rd, 6th, 9th, 12th, 15th, ...
    //   Link 1 packets with EOV: 5th, 10th, 15th, 20th, ...
    //   Link 2 packets with EOV: 8th, 16th, 24th, 32nd, ...
    const std::vector<size_t> eov_every_nth_packet{3, 5};

    const size_t num_chans = eov_every_nth_packet.size();
    auto recv_links        = make_links(num_chans);
    auto streamer          = make_rx_streamer(recv_links, format);

    const size_t spp       = streamer->get_max_num_samps();
    const size_t num_samps = spp * NUM_PACKETS;

    std::vector<std::vector<std::complex<double>>> buffer(num_chans);
    std::vector<void*> buffers;
    for (size_t i = 0; i < num_chans; i++) {
        buffer[i].resize(num_samps);
        buffers.push_back(&buffer[i].front());
    }

    mock_header_t header;
    std::vector<size_t> expected_eov_offsets;
    for (size_t i = 0; i < NUM_PACKETS; i++) {
        bool eov = false;
        for (size_t ch = 0; ch < num_chans; ch++) {
            header.eob     = false;
            header.has_tsf = false;
            header.eov     = ((i + 1) % eov_every_nth_packet[ch]) == 0;

            push_back_recv_packet(recv_links[ch], header, spp);

            eov |= header.eov;
        }
        if (eov) {
            expected_eov_offsets.push_back(spp * (i + 1));
        }
    }

    uhd::rx_metadata_t metadata;

    std::vector<size_t> eov_positions(expected_eov_offsets.size() + 1);
    metadata.eov_positions      = eov_positions.data();
    metadata.eov_positions_size = eov_positions.size();

    size_t num_samps_ret = streamer->recv(buffers, num_samps, metadata, 1.0, false);

    BOOST_CHECK_EQUAL(num_samps_ret, num_samps);
    BOOST_CHECK_EQUAL(metadata.eov_positions_count, expected_eov_offsets.size());
    for (size_t i = 0; i < metadata.eov_positions_count; i++) {
        BOOST_CHECK_EQUAL(expected_eov_offsets[i], metadata.eov_positions[i]);
    }
}

// A call to `recv()` of zero samples should return immediately, regardless of
// the timeout parameter, and not return a timeout error despite the potential
// absence of a packet on the wire.
BOOST_AUTO_TEST_CASE(test_recv_zero_samples)
{
    const std::string format("fc64");

    auto recv_links = make_links(1);
    auto streamer   = make_rx_streamer(recv_links, format);

    std::vector<std::complex<double>> buff(1);
    uhd::rx_metadata_t metadata;

    const auto start_time = std::chrono::steady_clock::now();

    const size_t num_samps_ret =
        streamer->recv(buff.data(), 0, metadata, 10.0, false);

    const auto end_time = std::chrono::steady_clock::now();
    const std::chrono::duration<double> elapsed_time(end_time - start_time);

    BOOST_CHECK_EQUAL(num_samps_ret, 0);
    BOOST_CHECK_EQUAL(metadata.error_code, uhd::rx_metadata_t::ERROR_CODE_NONE);

    // Ensure that the `recv()` of zero samples didn't wait the requested
    // timeout period of 10 seconds.
    BOOST_CHECK_LE(elapsed_time.count(), 0.5);
}
