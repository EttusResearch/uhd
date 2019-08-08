//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "../common/mock_link.hpp"
#include <uhdlib/transport/tx_streamer_impl.hpp>
#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

namespace uhd { namespace transport {

/*!
 * Mock tx data xport which doesn't use I/O service, and just interacts with
 * the link directly. Transport copies packet info directly into the frame
 * buffer.
 */
class mock_tx_data_xport
{
public:
    using uptr   = std::unique_ptr<mock_tx_data_xport>;
    using buff_t = uhd::transport::frame_buff;

    struct packet_info_t
    {
        bool eob             = false;
        bool has_tsf         = false;
        uint64_t tsf         = 0;
        size_t payload_bytes = 0;
    };

    mock_tx_data_xport(mock_send_link::sptr send_link) : _send_link(send_link) {}

    buff_t::uptr get_send_buff(const int32_t timeout_ms)
    {
        return _send_link->get_send_buff(timeout_ms);
    }

    std::pair<void*, size_t> write_packet_header(
        buff_t::uptr& buff, const packet_info_t& info)
    {
        uint8_t* data                             = static_cast<uint8_t*>(buff->data());
        *(reinterpret_cast<packet_info_t*>(data)) = info;
        return std::make_pair(data + sizeof(info), sizeof(info) + info.payload_bytes);
    }

    void release_send_buff(buff_t::uptr buff)
    {
        _send_link->release_send_buff(std::move(buff));
    }

    size_t get_max_payload_size() const
    {
        return _send_link->get_send_frame_size() - sizeof(packet_info_t);
        ;
    }

private:
    mock_send_link::sptr _send_link;
};

/*!
 * Mock tx streamer for testing
 */
class mock_tx_streamer : public tx_streamer_impl<mock_tx_data_xport>
{
public:
    mock_tx_streamer(const size_t num_chans, const uhd::stream_args_t& stream_args)
        : tx_streamer_impl(num_chans, stream_args)
    {
    }

    void set_tick_rate(double rate)
    {
        tx_streamer_impl::set_tick_rate(rate);
    }

    void set_samp_rate(double rate)
    {
        tx_streamer_impl::set_samp_rate(rate);
    }

    void set_scale_factor(const size_t chan, const double scale_factor)
    {
        tx_streamer_impl::set_scale_factor(chan, scale_factor);
    }

    bool recv_async_msg(uhd::async_metadata_t& /*async_metadata*/,
        double /*timeout = 0.1*/)
    {
        return false;
    }
};

}} // namespace uhd::transport

using namespace uhd::transport;

using tx_streamer = tx_streamer_impl<mock_tx_data_xport>;

static const double TICK_RATE    = 100e6;
static const double SAMP_RATE    = 10e6;
static const size_t FRAME_SIZE   = 1000;
static const double SCALE_FACTOR = 2;

/*!
 * Helper functions
 */
static std::vector<mock_send_link::sptr> make_links(const size_t num)
{
    const mock_send_link::link_params params = {FRAME_SIZE, 1};

    std::vector<mock_send_link::sptr> links;

    for (size_t i = 0; i < num; i++) {
        links.push_back(std::make_shared<mock_send_link>(params));
    }

    return links;
}

static boost::shared_ptr<mock_tx_streamer> make_tx_streamer(
    std::vector<mock_send_link::sptr> send_links, const std::string& format)
{
    const uhd::stream_args_t stream_args(format, "sc16");
    auto streamer = boost::make_shared<mock_tx_streamer>(send_links.size(), stream_args);
    streamer->set_tick_rate(TICK_RATE);
    streamer->set_samp_rate(SAMP_RATE);

    for (size_t i = 0; i < send_links.size(); i++) {
        mock_tx_data_xport::uptr xport(
            std::make_unique<mock_tx_data_xport>(send_links[i]));

        streamer->set_scale_factor(i, SCALE_FACTOR);
        streamer->connect_channel(i, std::move(xport));
    }

    return streamer;
}

std::tuple<mock_tx_data_xport::packet_info_t, std::complex<uint16_t>*, size_t, boost::shared_array<uint8_t>>
pop_send_packet(mock_send_link::sptr send_link)
{
    auto packet = send_link->pop_send_packet();

    const size_t packet_samps =
        (packet.second - sizeof(mock_tx_data_xport::packet_info_t))
        / sizeof(std::complex<uint16_t>);

    uint8_t* buff_ptr = packet.first.get();
    auto info         = *(reinterpret_cast<mock_tx_data_xport::packet_info_t*>(buff_ptr));

    std::complex<uint16_t>* data = reinterpret_cast<std::complex<uint16_t>*>(
        buff_ptr + sizeof(mock_tx_data_xport::packet_info_t));

    return std::make_tuple(info, data, packet_samps, packet.first);
}

/*!
 * Tests
 */
BOOST_AUTO_TEST_CASE(test_send_one_channel_one_packet)
{
    const size_t NUM_PKTS_TO_TEST = 30;
    const std::string format("fc32");

    auto send_links = make_links(1);
    auto streamer   = make_tx_streamer(send_links, format);

    // Allocate metadata
    uhd::tx_metadata_t metadata;
    metadata.has_time_spec = true;
    metadata.time_spec     = uhd::time_spec_t(0.0);

    // Allocate buffer and write data
    std::vector<std::complex<float>> buff(20);
    for (size_t i = 0; i < buff.size(); i++) {
        buff[i] = std::complex<float>(i * 2, i * 2 + 1);
    }

    // Send packets and check data
    size_t num_accum_samps = 0;

    for (size_t i = 0; i < NUM_PKTS_TO_TEST; i++) {
        std::cout << "sending packet " << i << std::endl;

        // Vary num_samps for each packet
        const size_t num_samps = 10 + i % 10;
        metadata.end_of_burst  = (i == NUM_PKTS_TO_TEST - 1);
        const size_t num_sent  = streamer->send(&buff.front(), num_samps, metadata, 1.0);
        BOOST_CHECK_EQUAL(num_sent, num_samps);
        metadata.time_spec += uhd::time_spec_t(0, num_sent, SAMP_RATE);

        mock_tx_data_xport::packet_info_t info;
        std::complex<uint16_t>* data;
        size_t packet_samps;
        boost::shared_array<uint8_t> frame_buff;

        std::tie(info, data, packet_samps, frame_buff) = pop_send_packet(send_links[0]);
        BOOST_CHECK_EQUAL(num_samps, packet_samps);

        // Check data
        for (size_t j = 0; j < num_samps; j++) {
            const std::complex<uint16_t> value(
                (j * 2) * SCALE_FACTOR, (j * 2 + 1) * SCALE_FACTOR);
            BOOST_CHECK_EQUAL(value, data[j]);
        }

        BOOST_CHECK_EQUAL(num_samps, info.payload_bytes / sizeof(std::complex<uint16_t>));
        BOOST_CHECK(info.has_tsf);
        BOOST_CHECK_EQUAL(info.tsf, num_accum_samps * TICK_RATE / SAMP_RATE);
        BOOST_CHECK_EQUAL(info.eob, i == NUM_PKTS_TO_TEST - 1);
        num_accum_samps += num_samps;
    }
}

BOOST_AUTO_TEST_CASE(test_send_one_channel_multi_packet)
{
    const size_t NUM_BUFFS_TO_TEST = 5;
    const std::string format("fc64");

    auto send_links = make_links(1);
    auto streamer   = make_tx_streamer(send_links, format);

    // Allocate metadata
    uhd::tx_metadata_t metadata;
    metadata.has_time_spec = true;
    metadata.time_spec     = uhd::time_spec_t(0.0);

    // Allocate buffer and write data
    const size_t spp       = streamer->get_max_num_samps();
    const size_t num_samps = spp * 4;
    std::vector<std::complex<double>> buff(num_samps);
    for (size_t i = 0; i < buff.size(); i++) {
        buff[i] = std::complex<double>(i * 2, i * 2 + 1);
    }

    // Send packets and check data
    size_t num_accum_samps = 0;

    for (size_t i = 0; i < NUM_BUFFS_TO_TEST; i++) {
        std::cout << "sending packet " << i << std::endl;

        metadata.end_of_burst = true;
        const size_t num_sent = streamer->send(&buff.front(), num_samps, metadata, 1.0);
        BOOST_CHECK_EQUAL(num_sent, num_samps);
        metadata.time_spec += uhd::time_spec_t(0, num_sent, SAMP_RATE);

        size_t samps_checked = 0;

        while (samps_checked < num_samps) {
            mock_tx_data_xport::packet_info_t info;
            std::complex<uint16_t>* data;
            size_t packet_samps;
            boost::shared_array<uint8_t> frame_buff;

            std::tie(info, data, packet_samps, frame_buff) = pop_send_packet(send_links[0]);

            for (size_t j = 0; j < packet_samps; j++) {
                const size_t n = j + samps_checked;
                const std::complex<uint16_t> value(
                    (n * 2) * SCALE_FACTOR, (n * 2 + 1) * SCALE_FACTOR);
                BOOST_CHECK_EQUAL(value, data[j]);
            }

            BOOST_CHECK_EQUAL(
                packet_samps, info.payload_bytes / sizeof(std::complex<uint16_t>));
            BOOST_CHECK(info.has_tsf);
            BOOST_CHECK_EQUAL(
                info.tsf, (num_accum_samps + samps_checked) * TICK_RATE / SAMP_RATE);
            samps_checked += packet_samps;

            BOOST_CHECK_EQUAL(info.eob, samps_checked == num_samps);
        }

        BOOST_CHECK_EQUAL(samps_checked, num_samps);
        num_accum_samps += samps_checked;
    }
}

BOOST_AUTO_TEST_CASE(test_send_two_channel_one_packet)
{
    const size_t NUM_PKTS_TO_TEST = 30;
    const std::string format("sc16");

    auto send_links = make_links(2);
    auto streamer   = make_tx_streamer(send_links, format);

    // Allocate metadata
    uhd::tx_metadata_t metadata;
    metadata.has_time_spec = true;
    metadata.time_spec     = uhd::time_spec_t(0.0);

    // Allocate buffer and write data
    std::vector<std::complex<uint16_t>> buff(20);
    for (size_t i = 0; i < buff.size(); i++) {
        buff[i] = std::complex<uint16_t>(i * 2, i * 2 + 1);
    }
    std::vector<void*> buffs;
    for (size_t ch = 0; ch < 2; ch++) {
        buffs.push_back(buff.data()); // same buffer for each channel
    }

    // Send packets and check data
    size_t num_accum_samps = 0;

    for (size_t i = 0; i < NUM_PKTS_TO_TEST; i++) {
        std::cout << "sending packet " << i << std::endl;

        // Vary num_samps for each packet
        const size_t num_samps = 10 + i % 10;
        metadata.end_of_burst  = (i == NUM_PKTS_TO_TEST - 1);
        const size_t num_sent  = streamer->send(buffs, num_samps, metadata, 1.0);
        BOOST_CHECK_EQUAL(num_sent, num_samps);
        metadata.time_spec += uhd::time_spec_t(0, num_sent, SAMP_RATE);

        for (size_t ch = 0; ch < 2; ch++) {
            mock_tx_data_xport::packet_info_t info;
            std::complex<uint16_t>* data;
            size_t packet_samps;
            boost::shared_array<uint8_t> frame_buff;

            std::tie(info, data, packet_samps, frame_buff) = pop_send_packet(send_links[ch]);
            BOOST_CHECK_EQUAL(num_samps, packet_samps);

            // Check data
            for (size_t j = 0; j < num_samps; j++) {
                const std::complex<uint16_t> value((j * 2), (j * 2 + 1));
                BOOST_CHECK_EQUAL(value, data[j]);
            }

            BOOST_CHECK_EQUAL(
                num_samps, info.payload_bytes / sizeof(std::complex<uint16_t>));
            BOOST_CHECK(info.has_tsf);
            BOOST_CHECK_EQUAL(info.tsf, num_accum_samps * TICK_RATE / SAMP_RATE);
            BOOST_CHECK_EQUAL(info.eob, i == NUM_PKTS_TO_TEST - 1);
        }
        num_accum_samps += num_samps;
    }
}

BOOST_AUTO_TEST_CASE(test_meta_data_cache)
{
    auto send_links = make_links(1);
    auto streamer   = make_tx_streamer(send_links, "fc32");

    // Allocate metadata
    uhd::tx_metadata_t metadata;
    metadata.start_of_burst = true;
    metadata.end_of_burst   = true;
    metadata.has_time_spec  = true;
    metadata.time_spec      = uhd::time_spec_t(0.0);

    // Allocate buffer and write data
    std::vector<std::complex<float>> buff(20);

    size_t num_sent = streamer->send(buff.data(), 0, metadata, 1.0);
    BOOST_CHECK_EQUAL(send_links[0]->get_num_packets(), 0);
    BOOST_CHECK_EQUAL(num_sent, 0);
    uhd::tx_metadata_t metadata2;
    num_sent = streamer->send(buff.data(), 10, metadata2, 1.0);

    mock_tx_data_xport::packet_info_t info;
    size_t packet_samps;
    boost::shared_array<uint8_t> frame_buff;

    std::tie(info, std::ignore, packet_samps, frame_buff) = pop_send_packet(send_links[0]);
    BOOST_CHECK_EQUAL(packet_samps, num_sent);
    BOOST_CHECK(info.has_tsf);
    BOOST_CHECK(info.eob);
}

BOOST_AUTO_TEST_CASE(test_spp)
{
    // Test the spp calculation when it is limited by the stream args
    {
        auto send_links = make_links(1);
        uhd::stream_args_t stream_args("fc64", "sc16");
        stream_args.args["spp"] = std::to_string(10);
        auto streamer = boost::make_shared<mock_tx_streamer>(send_links.size(), stream_args);
        mock_tx_data_xport::uptr xport(std::make_unique<mock_tx_data_xport>(send_links[0]));
        streamer->connect_channel(0, std::move(xport));
        BOOST_CHECK_EQUAL(streamer->get_max_num_samps(), 10);
    }

    // Test the spp calculation when it is limited by the frame size
    {
        auto send_links = make_links(1);
        uhd::stream_args_t stream_args("fc64", "sc16");
        stream_args.args["spp"] = std::to_string(10000);
        auto streamer = boost::make_shared<mock_tx_streamer>(send_links.size(), stream_args);
        mock_tx_data_xport::uptr xport(std::make_unique<mock_tx_data_xport>(send_links[0]));
        const size_t max_pyld = xport->get_max_payload_size();
        streamer->connect_channel(0, std::move(xport));
        BOOST_CHECK_EQUAL(streamer->get_max_num_samps(), max_pyld / sizeof(std::complex<uint16_t>));
    }
}
