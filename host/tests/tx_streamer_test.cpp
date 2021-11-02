//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "../common/mock_link.hpp"
#include <uhdlib/transport/tx_streamer_impl.hpp>
#include <boost/test/unit_test.hpp>
#include <complex>
#include <iostream>
#include <memory>

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
        bool eov             = false;
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

    size_t get_mtu() const
    {
        return _send_link->get_send_frame_size();
    }

    size_t get_chdr_hdr_len() const
    {
        return sizeof(packet_info_t);
    }

    size_t get_max_payload_size() const
    {
        return get_mtu() - get_chdr_hdr_len();
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

    bool recv_async_msg(
        uhd::async_metadata_t& /*async_metadata*/, double /*timeout = 0.1*/) override
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

static std::shared_ptr<mock_tx_streamer> make_tx_streamer(
    std::vector<mock_send_link::sptr> send_links, const std::string& format)
{
    const uhd::stream_args_t stream_args(format, "sc16");
    auto streamer = std::make_shared<mock_tx_streamer>(send_links.size(), stream_args);
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

std::tuple<mock_tx_data_xport::packet_info_t,
    std::complex<uint16_t>*,
    size_t,
    boost::shared_array<uint8_t>>
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

//! Generates a non-biased random number in the range (low, high).
static size_t generate_rand(size_t low, size_t high)
{
    return (std::rand() % (high - low + 1)) + low;
}

/*!
 * Generates a vector of legal random EOV positions.
 * `eovs` is a vector already sized to the desired number of EOV positions.
 * The range [1, num_samps) will be broken into N=`eov.size()`
 * non-overlapping adjacent ranges, and a random value within each range will
 * be generated and stored in the vector.
 */
static void generate_random_eov_positions(
    std::vector<size_t>& eovs, const size_t num_samps)
{
    UHD_ASSERT_THROW(!eovs.empty());

    const size_t num_eovs   = eovs.size();
    const size_t range_size = (num_samps - 1) / num_eovs;
    size_t low              = 1;
    for (size_t i = 0; i < num_eovs; i++) {
        const size_t high = low + range_size - 1;
        eovs[i]           = generate_rand(low, high);
        low               = high + 1;
    }
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

BOOST_AUTO_TEST_CASE(test_send_one_channel_eov_lte_spp)
{
    const size_t NUM_PKTS_TO_TEST = 30;
    const std::string format("fc32");

    auto send_links = make_links(1);
    auto streamer   = make_tx_streamer(send_links, format);

    // Allocate metadata
    uhd::tx_metadata_t metadata;
    metadata.has_time_spec = true;
    metadata.time_spec     = uhd::time_spec_t(0.0);

    // Allocate buffer
    const size_t num_samps = streamer->get_max_num_samps();
    std::vector<std::complex<float>> buff(num_samps);

    // Send buffer and check resultant packets
    size_t num_accum_samps = 0;

    for (size_t i = 0; i < NUM_PKTS_TO_TEST; i++) {
        std::cout << "sending packet " << i << std::endl;

        // Vary number of EOVs for each send
        const size_t num_eovs = (i % 10) + 1;
        std::vector<size_t> eovs(num_eovs);
        generate_random_eov_positions(eovs, num_samps);

        metadata.eov_positions      = eovs.data();
        metadata.eov_positions_size = eovs.size();

        const size_t num_sent = streamer->send(&buff.front(), num_samps, metadata, 1.0);
        BOOST_CHECK_EQUAL(num_sent, num_samps);
        metadata.time_spec += uhd::time_spec_t(0, num_sent, SAMP_RATE);

        mock_tx_data_xport::packet_info_t info;
        std::complex<uint16_t>* data;
        size_t packet_samps;
        boost::shared_array<uint8_t> frame_buff;

        // Check number of packets written (should be # of EOVs plus one)
        size_t num_packets_written = send_links[0]->get_num_packets();
        BOOST_CHECK_EQUAL(num_packets_written, eovs.size() + 1);

        // Pop each packets and check size of each relative to EOV positions
        size_t total_samps_popped = 0;
        size_t eov_index          = 0;
        size_t last_eov_position  = 0;
        while (total_samps_popped < num_samps) {
            std::tie(info, data, packet_samps, frame_buff) =
                pop_send_packet(send_links[0]);

            // All but the last packet should have an EOV indication
            if (eov_index < eovs.size()) {
                BOOST_CHECK_EQUAL(eovs[eov_index] - last_eov_position, packet_samps);
                BOOST_CHECK(info.eov);
                last_eov_position = eovs[eov_index];
            } else {
                BOOST_CHECK_EQUAL(num_samps - last_eov_position, packet_samps);
                BOOST_CHECK(not info.eov);
            }

            // Verify correctness of TSF data
            BOOST_CHECK(info.has_tsf);
            BOOST_CHECK_EQUAL(info.tsf, num_accum_samps * TICK_RATE / SAMP_RATE);

            eov_index++;
            num_accum_samps += packet_samps;
            total_samps_popped += packet_samps;
        }
    }
}

BOOST_AUTO_TEST_CASE(test_send_one_channel_eov_gt_spp)
{
    const size_t NUM_PKTS_TO_TEST = 30;
    const std::string format("fc32");

    auto send_links = make_links(1);
    auto streamer   = make_tx_streamer(send_links, format);

    // Allocate metadata
    uhd::tx_metadata_t metadata;
    metadata.has_time_spec = true;
    metadata.time_spec     = uhd::time_spec_t(0.0);

    // Allocate buffer
    const size_t spp       = streamer->get_max_num_samps();
    const size_t num_samps = spp * 50;
    std::vector<std::complex<float>> buff(num_samps);

    // Send buffer and check resultant packets
    size_t num_accum_samps = 0;

    for (size_t i = 0; i < NUM_PKTS_TO_TEST; i++) {
        std::cout << "sending packet " << i << std::endl;

        // Vary number of EOVs for each send
        const size_t num_eovs = (i % 5) + 1;
        std::vector<size_t> eovs(num_eovs);
        generate_random_eov_positions(eovs, num_samps);

        metadata.eov_positions      = eovs.data();
        metadata.eov_positions_size = eovs.size();

        const size_t num_sent = streamer->send(&buff.front(), num_samps, metadata, 1.0);
        BOOST_CHECK_EQUAL(num_sent, num_samps);
        metadata.time_spec += uhd::time_spec_t(0, num_sent, SAMP_RATE);

        mock_tx_data_xport::packet_info_t info;
        std::complex<uint16_t>* data;
        size_t packet_samps;
        boost::shared_array<uint8_t> frame_buff;

        size_t total_samps_popped   = 0;
        size_t eov_index            = 0;
        size_t last_eov_position    = 0;
        size_t distance_to_next_eov = eovs[eov_index] - last_eov_position;
        size_t distance_to_end      = num_samps;
        while (total_samps_popped < num_samps) {
            std::tie(info, data, packet_samps, frame_buff) =
                pop_send_packet(send_links[0]);

            if (distance_to_next_eov <= spp) {
                // Next EOV is within a single SPP: ensure packet is EOV
                BOOST_CHECK_EQUAL(distance_to_next_eov, packet_samps);
                BOOST_CHECK(info.eov);
                last_eov_position = eovs[eov_index];
                eov_index++;
                if (eov_index < eovs.size()) {
                    // NOTE: Addition of `packet_samps` is to compensate for
                    // its subtraction below
                    distance_to_next_eov =
                        eovs[eov_index] - last_eov_position + packet_samps;
                } else {
                    // No more EOVs
                    distance_to_next_eov = std::numeric_limits<size_t>::max();
                }
            } else if (distance_to_end <= spp) {
                // End of data within a single SPP
                BOOST_CHECK_EQUAL(distance_to_end, packet_samps);
                BOOST_CHECK(not info.eov);
            } else {
                BOOST_CHECK(not info.eov);
            }

            // Verify correctness of TSF data
            BOOST_CHECK(info.has_tsf);
            BOOST_CHECK_EQUAL(info.tsf, num_accum_samps * TICK_RATE / SAMP_RATE);

            distance_to_end -= packet_samps;
            distance_to_next_eov -= packet_samps;
            total_samps_popped += packet_samps;
            num_accum_samps += packet_samps;
        }
    }
}

BOOST_AUTO_TEST_CASE(test_send_one_channel_eov_corner_case)
{
    const std::string format("fc32");

    auto send_links = make_links(1);
    auto streamer   = make_tx_streamer(send_links, format);

    // Allocate metadata
    uhd::tx_metadata_t metadata;
    metadata.has_time_spec = true;
    metadata.time_spec     = uhd::time_spec_t(0.0);

    // Allocate buffer
    const size_t spp       = streamer->get_max_num_samps();
    const size_t num_samps = spp * 2;
    std::vector<std::complex<float>> buff(num_samps);

    // Mark every sample as an EOV :D
    std::vector<size_t> eovs(num_samps);
    for (size_t i = 0; i < num_samps; i++) {
        eovs[i] = i + 1;
    }

    metadata.eov_positions      = eovs.data();
    metadata.eov_positions_size = eovs.size();

    const size_t num_sent = streamer->send(&buff.front(), num_samps, metadata, 1.0);
    BOOST_CHECK_EQUAL(num_sent, num_samps);

    mock_tx_data_xport::packet_info_t info;
    std::complex<uint16_t>* data;
    size_t packet_samps;
    boost::shared_array<uint8_t> frame_buff;

    // Check all packets for EOV
    BOOST_CHECK_EQUAL(send_links[0]->get_num_packets(), num_samps);

    for (size_t i = 0; i < num_samps; i++) {
        std::tie(info, data, packet_samps, frame_buff) = pop_send_packet(send_links[0]);
        BOOST_CHECK_EQUAL(packet_samps, 1);
        BOOST_CHECK(info.eov);
    }
}

BOOST_AUTO_TEST_CASE(test_send_one_channel_eov_error_cases)
{
    const std::string format("fc32");

    auto send_links = make_links(1);
    auto streamer   = make_tx_streamer(send_links, format);

    uhd::tx_metadata_t metadata;
    metadata.has_time_spec = true;
    metadata.time_spec     = uhd::time_spec_t(0.0);

    const size_t spp       = streamer->get_max_num_samps();
    const size_t num_samps = spp;
    std::vector<std::complex<float>> buff(num_samps);

    // Error case: EOV of 0
    size_t eov                  = 0;
    metadata.eov_positions      = &eov;
    metadata.eov_positions_size = 1;

    BOOST_CHECK_THROW(
        streamer->send(&buff.front(), num_samps, metadata, 1.0), uhd::value_error);

    // Error case: Adjacent EOV values that are the same
    std::vector<size_t> eovs{100, 100};
    metadata.eov_positions      = eovs.data();
    metadata.eov_positions_size = eovs.size();

    BOOST_CHECK_THROW(
        streamer->send(&buff.front(), num_samps, metadata, 1.0), uhd::value_error);

    // Error case: EOV values not monotonically increasing
    eovs = {50, 25};

    BOOST_CHECK_THROW(
        streamer->send(&buff.front(), num_samps, metadata, 1.0), uhd::value_error);

    // Error case: EOV values greater than nsamps_per_buff
    BOOST_CHECK_THROW(streamer->send(&buff.front(), 1, metadata, 1.0), uhd::value_error);
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

            std::tie(info, data, packet_samps, frame_buff) =
                pop_send_packet(send_links[0]);

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

            std::tie(info, data, packet_samps, frame_buff) =
                pop_send_packet(send_links[ch]);
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

    std::tie(info, std::ignore, packet_samps, frame_buff) =
        pop_send_packet(send_links[0]);
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
        auto streamer =
            std::make_shared<mock_tx_streamer>(send_links.size(), stream_args);
        mock_tx_data_xport::uptr xport(
            std::make_unique<mock_tx_data_xport>(send_links[0]));
        streamer->connect_channel(0, std::move(xport));
        BOOST_CHECK_EQUAL(streamer->get_max_num_samps(), 10);
    }

    // Test the spp calculation when it is limited by the frame size
    {
        auto send_links = make_links(1);
        uhd::stream_args_t stream_args("fc64", "sc16");
        stream_args.args["spp"] = std::to_string(10000);
        auto streamer =
            std::make_shared<mock_tx_streamer>(send_links.size(), stream_args);
        mock_tx_data_xport::uptr xport(
            std::make_unique<mock_tx_data_xport>(send_links[0]));
        const size_t max_pyld = xport->get_max_payload_size();
        streamer->connect_channel(0, std::move(xport));
        BOOST_CHECK_EQUAL(
            streamer->get_max_num_samps(), max_pyld / sizeof(std::complex<uint16_t>));
    }
}
