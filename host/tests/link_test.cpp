//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "common/mock_link.hpp"
#include <boost/test/unit_test.hpp>

using namespace uhd::transport;

BOOST_AUTO_TEST_CASE(test_send_get_release)
{
    // Just call get_send_buff, release_send_buff, and pop_send_packet
    // from a link containing a single frame_buff.
    const size_t num_frames  = 1;
    const int32_t timeout_ms = 1;

    const mock_send_link::link_params params = {1000, num_frames};

    auto xport = std::make_shared<mock_send_link>(params);

    // Check sizes
    BOOST_CHECK_EQUAL(xport->get_num_send_frames(), params.num_frames);
    BOOST_CHECK_EQUAL(xport->get_send_frame_size(), params.frame_size);

    // Call get and release a few times and check packet contents
    for (size_t i = 0; i < 5; i++) {
        auto buff = xport->get_send_buff(timeout_ms);
        BOOST_CHECK(buff);

        auto* ptr = static_cast<uint8_t*>(buff->data());
        ptr[0]    = i;

        buff->set_packet_size(sizeof(uint8_t));
        xport->release_send_buff(std::move(buff));
        BOOST_CHECK(!buff);

        auto packet = xport->pop_send_packet();
        BOOST_CHECK_EQUAL(packet.first[0], i);
        BOOST_CHECK_EQUAL(packet.second, sizeof(uint8_t));
    }
}

BOOST_AUTO_TEST_CASE(test_send_timeout)
{
    // Test that the link returns timeouts correctly and continues to
    // operate properly after a timeout condition.
    const size_t num_frames  = 10;
    const int32_t timeout_ms = 1;

    const mock_send_link::link_params params = {1000, num_frames};

    auto xport = std::make_shared<mock_send_link>(params);

    // Repeat the following a few times to check buffers are not lost
    for (size_t i = 0; i < 3; i++) {
        // Cause a timeout by simulating an I/O delay in the underlying
        // link implementation.
        std::vector<frame_buff::uptr> buffs;

        for (size_t j = 0; j < num_frames / 2; j++) {
            buffs.push_back(xport->get_send_buff(timeout_ms));
            BOOST_CHECK(buffs.back());
        }

        // Simulate a timeout
        xport->set_simulate_io_timeout(true);
        BOOST_CHECK(!xport->get_send_buff(timeout_ms));
        xport->set_simulate_io_timeout(false);

        for (size_t j = 0; j < num_frames / 2; j++) {
            buffs.push_back(xport->get_send_buff(timeout_ms));
            BOOST_CHECK(buffs.back());
        }

        for (auto& buff : buffs) {
            buff->set_packet_size(params.frame_size);
            xport->release_send_buff(std::move(buff));
            BOOST_CHECK(!buff);
        }
    }
}

BOOST_AUTO_TEST_CASE(test_recv_get_release)
{
    // Just call push_recv_packet, get_recv_buff, and release_recv_buff
    // from a link containing a single frame_buff.
    const size_t num_frames  = 1;
    const int32_t timeout_ms = 1;

    const mock_recv_link::link_params params = {1000, num_frames};

    auto xport = std::make_shared<mock_recv_link>(params);

    // Check sizes
    BOOST_CHECK_EQUAL(xport->get_num_recv_frames(), params.num_frames);
    BOOST_CHECK_EQUAL(xport->get_recv_frame_size(), params.frame_size);

    // Call get and release a few times and check packet contents
    for (size_t i = 0; i < 5; i++) {
        size_t packet_size = sizeof(uint8_t);
        auto packet_data   = boost::shared_array<uint8_t>(new uint8_t[packet_size]);
        packet_data[0]     = static_cast<uint8_t>(i);
        xport->push_back_recv_packet(packet_data, packet_size);

        auto buff = xport->get_recv_buff(timeout_ms);
        BOOST_CHECK(buff);
        BOOST_CHECK(buff->data());

        auto* ptr = static_cast<uint8_t*>(buff->data());
        BOOST_CHECK_EQUAL(ptr[0], static_cast<uint8_t>(i));

        xport->release_recv_buff(std::move(buff));
        BOOST_CHECK(!buff);
    }
}

BOOST_AUTO_TEST_CASE(test_recv_timeout)
{
    // Test that the link returns timeouts correctly and continues to
    // operate properly after a timeout condition.
    const size_t num_frames  = 10;
    const int32_t timeout_ms = 1;

    const mock_recv_link::link_params params = {1000, num_frames};

    auto xport = std::make_shared<mock_recv_link>(params);

    // Repeat the following a few times to check buffers are not lost
    for (size_t i = 0; i < 3; i++) {
        // Cause a timeout by simulating an I/O delay in the underlying
        // link implementation.
        std::vector<frame_buff::uptr> buffs;

        for (size_t i = 0; i < num_frames / 2; i++) {
            size_t packet_size = sizeof(uint8_t);
            auto packet_data   = boost::shared_array<uint8_t>(new uint8_t[packet_size]);
            xport->push_back_recv_packet(packet_data, packet_size);

            buffs.push_back(xport->get_recv_buff(timeout_ms));
            BOOST_CHECK(buffs.back());
        }

        // Simulate a timeout by getting a buffer without queueing a data array
        BOOST_CHECK(!xport->get_recv_buff(timeout_ms));

        for (size_t i = 0; i < num_frames / 2; i++) {
            size_t packet_size = sizeof(uint8_t);
            auto packet_data   = boost::shared_array<uint8_t>(new uint8_t[packet_size]);
            xport->push_back_recv_packet(packet_data, packet_size);

            buffs.push_back(xport->get_recv_buff(timeout_ms));
            BOOST_CHECK(buffs.back());
        }

        for (auto& buff : buffs) {
            xport->release_recv_buff(std::move(buff));
            BOOST_CHECK(!buff);
        }
    }
}
