//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "common/mock_link.hpp"
#include <uhdlib/transport/offload_io_service.hpp>
#include <boost/test/unit_test.hpp>
#include <atomic>
#include <iostream>

using namespace uhd::transport;

class mock_recv_io;
constexpr size_t FRAME_SIZE = 1000;

static mock_send_link::sptr make_send_link(size_t num_frames)
{
    const mock_send_link::link_params params = {FRAME_SIZE, num_frames};
    return std::make_shared<mock_send_link>(params);
}

static mock_recv_link::sptr make_recv_link(size_t num_frames)
{
    const mock_recv_link::link_params params = {FRAME_SIZE, num_frames};
    return std::make_shared<mock_recv_link>(params);
}

class mock_recv_io : public recv_io_if
{
public:
    mock_recv_io(recv_link_if::sptr link) : _link(link) {}

    frame_buff::uptr get_recv_buff(int32_t timeout_ms) override
    {
        if (_frames_allocated > 0) {
            _frames_allocated--;
            return _link->get_recv_buff(timeout_ms);
        }
        return nullptr;
    }

    void release_recv_buff(frame_buff::uptr buff) override
    {
        _link->release_recv_buff(std::move(buff));
    }

    size_t get_num_send_frames() const
    {
        return 0;
    }

    size_t get_num_recv_frames() const
    {
        return _link->get_num_recv_frames();
    }

    void allocate_frames(const size_t num_frames)
    {
        _frames_allocated += num_frames;
    }

private:
    std::atomic<size_t> _frames_allocated{0};
    recv_link_if::sptr _link;
};

class mock_send_io : public send_io_if
{
public:
    mock_send_io(send_link_if::sptr link) : _link(link) {}

    frame_buff::uptr get_send_buff(int32_t timeout_ms) override
    {
        return _link->get_send_buff(timeout_ms);
    }

    bool wait_for_dest_ready(size_t, int32_t) override
    {
        return true;
    }

    void release_send_buff(frame_buff::uptr buff) override
    {
        _link->release_send_buff(std::move(buff));
    }

    size_t get_num_send_frames() const
    {
        return _link->get_num_send_frames();
    }

    size_t get_num_recv_frames() const
    {
        return 0;
    }

private:
    send_link_if::sptr _link;
};

class mock_io_service : public io_service
{
public:
    void attach_recv_link(recv_link_if::sptr /*link*/) override {}
    void attach_send_link(send_link_if::sptr /*link*/) override {}
    void detach_recv_link(recv_link_if::sptr /*link*/) override {}
    void detach_send_link(send_link_if::sptr /*link*/) override {}

    send_io_if::sptr make_send_client(send_link_if::sptr send_link,
        size_t /*num_send_frames*/,
        send_io_if::send_callback_t /*cb*/,
        recv_link_if::sptr /*recv_link*/,
        size_t /*num_recv_frames*/,
        recv_callback_t /*recv_cb*/,
        send_io_if::fc_callback_t /*fc_cb*/) override
    {
        return std::make_shared<mock_send_io>(send_link);
    }

    recv_io_if::sptr make_recv_client(recv_link_if::sptr recv_link,
        size_t /*num_recv_frames*/,
        recv_callback_t /*cb*/,
        send_link_if::sptr /*fc_link*/,
        size_t /*num_send_frames*/,
        recv_io_if::fc_callback_t /*fc_cb*/) override
    {
        auto io = std::make_shared<mock_recv_io>(recv_link);
        _recv_io.push_back(io);
        return io;
    }

    void allocate_recv_frames(const size_t client_idx, const size_t num_frames)
    {
        assert(client_idx < _recv_io.size());
        _recv_io[client_idx]->allocate_frames(num_frames);
    }

    void set_detach_callback(std::function<void()>) {}

private:
    std::vector<std::shared_ptr<mock_recv_io>> _recv_io;
};

constexpr auto RECV_ONLY = offload_io_service::RECV_ONLY;
constexpr auto SEND_ONLY = offload_io_service::SEND_ONLY;

constexpr auto POLL  = offload_io_service::POLL;
constexpr auto BLOCK = offload_io_service::BLOCK;
using params_t       = offload_io_service::params_t;

std::vector<offload_io_service::wait_mode_t> wait_modes({POLL, BLOCK});

BOOST_AUTO_TEST_CASE(test_construction)
{
    for (const auto wait_mode : wait_modes) {
        params_t params{{}, SEND_ONLY, wait_mode};
        auto mock_io_srv = std::make_shared<mock_io_service>();
        auto io_srv      = offload_io_service::make(mock_io_srv, params_t());
        auto send_link   = make_send_link(5);
        io_srv->attach_send_link(send_link);
        auto send_client =
            io_srv->make_send_client(send_link, 5, nullptr, nullptr, 0, nullptr, nullptr);
    }
    for (const auto wait_mode : wait_modes) {
        params_t params{{}, RECV_ONLY, wait_mode};
        auto mock_io_srv = std::make_shared<mock_io_service>();
        auto io_srv      = offload_io_service::make(mock_io_srv, params_t());
        auto recv_link   = make_recv_link(5);
        io_srv->attach_recv_link(recv_link);
        auto recv_client =
            io_srv->make_recv_client(recv_link, 5, nullptr, nullptr, 0, nullptr);
    }
}

BOOST_AUTO_TEST_CASE(test_construction_with_options)
{
    offload_io_service::params_t params;
    params.cpu_affinity_list = {0};

    auto mock_io_srv = std::make_shared<mock_io_service>();
    auto io_srv      = offload_io_service::make(mock_io_srv, params);
    auto send_link   = make_send_link(5);
    io_srv->attach_send_link(send_link);
    auto recv_link = make_recv_link(5);
    io_srv->attach_recv_link(recv_link);
    auto send_client =
        io_srv->make_send_client(send_link, 5, nullptr, nullptr, 0, nullptr, nullptr);
    auto recv_client =
        io_srv->make_recv_client(recv_link, 5, nullptr, nullptr, 0, nullptr);
}

BOOST_AUTO_TEST_CASE(test_send)
{
    for (const auto wait_mode : wait_modes) {
        params_t params  = {{}, SEND_ONLY, wait_mode};
        auto mock_io_srv = std::make_shared<mock_io_service>();
        auto io_srv      = offload_io_service::make(mock_io_srv, params);
        auto send_link   = make_send_link(5);
        io_srv->attach_send_link(send_link);
        auto send_client =
            io_srv->make_send_client(send_link, 1, nullptr, nullptr, 0, nullptr, nullptr);

        for (size_t i = 0; i < 10; i++) {
            auto buff = send_client->get_send_buff(100);
            BOOST_CHECK(buff != nullptr);
            send_client->release_send_buff(std::move(buff));
        }
        send_client.reset();
    }
}

BOOST_AUTO_TEST_CASE(test_recv)
{
    for (const auto wait_mode : wait_modes) {
        params_t params  = {{}, RECV_ONLY, wait_mode};
        auto mock_io_srv = std::make_shared<mock_io_service>();
        auto io_srv      = offload_io_service::make(mock_io_srv, params);
        auto recv_link   = make_recv_link(5);
        io_srv->attach_recv_link(recv_link);

        auto recv_client =
            io_srv->make_recv_client(recv_link, 1, nullptr, nullptr, 0, nullptr);

        for (size_t i = 0; i < 10; i++) {
            recv_link->push_back_recv_packet(
                boost::shared_array<uint8_t>(new uint8_t[FRAME_SIZE]), FRAME_SIZE);
        }
        BOOST_CHECK(recv_client);
        mock_io_srv->allocate_recv_frames(0, 10);

        for (size_t i = 0; i < 10; i++) {
            auto buff = recv_client->get_recv_buff(100);
            BOOST_CHECK(buff != nullptr);
            recv_client->release_recv_buff(std::move(buff));
        }
        recv_client.reset();
    }
}

BOOST_AUTO_TEST_CASE(test_send_recv)
{
    auto mock_io_srv = std::make_shared<mock_io_service>();
    auto io_srv      = offload_io_service::make(mock_io_srv, params_t());
    auto send_link   = make_send_link(5);
    io_srv->attach_send_link(send_link);
    auto recv_link = make_recv_link(5);
    io_srv->attach_recv_link(recv_link);

    auto send_client =
        io_srv->make_send_client(send_link, 1, nullptr, nullptr, 0, nullptr, nullptr);
    auto recv_client =
        io_srv->make_recv_client(recv_link, 1, nullptr, nullptr, 0, nullptr);

    for (size_t i = 0; i < 20; i++) {
        recv_link->push_back_recv_packet(
            boost::shared_array<uint8_t>(new uint8_t[FRAME_SIZE]), FRAME_SIZE);
    }

    for (size_t i = 0; i < 10; i++) {
        send_client->release_send_buff(send_client->get_send_buff(100));
        mock_io_srv->allocate_recv_frames(0, 1);
        recv_client->release_recv_buff(recv_client->get_recv_buff(100));
    }

    auto recv_client2 =
        io_srv->make_recv_client(recv_link, 1, nullptr, nullptr, 0, nullptr);
    auto send_client2 =
        io_srv->make_send_client(send_link, 1, nullptr, nullptr, 0, nullptr, nullptr);
    for (size_t i = 0; i < 5; i++) {
        mock_io_srv->allocate_recv_frames(1, 1);
        recv_client2->release_recv_buff(recv_client2->get_recv_buff(100));
        send_client2->release_send_buff(send_client2->get_send_buff(100));
    }
    send_client2.reset();
    recv_client2.reset();

    for (size_t i = 0; i < 5; i++) {
        mock_io_srv->allocate_recv_frames(0, 1);
        recv_client->release_recv_buff(recv_client->get_recv_buff(100));
    }

    send_client.reset();
    recv_client.reset();
}

BOOST_AUTO_TEST_CASE(test_attach_detach)
{
    auto mock_io_srv = std::make_shared<mock_io_service>();
    auto io_srv      = offload_io_service::make(mock_io_srv, params_t());
    auto recv_link0  = make_recv_link(5);
    auto send_link0  = make_send_link(5);
    auto recv_link1  = make_recv_link(5);
    auto send_link1  = make_send_link(5);

    io_srv->attach_recv_link(recv_link0);
    io_srv->attach_send_link(send_link0);
    io_srv->attach_recv_link(recv_link1);
    io_srv->attach_send_link(send_link1);

    auto recv_client0 =
        io_srv->make_recv_client(recv_link0, 1, nullptr, nullptr, 0, nullptr);
    auto send_client0 =
        io_srv->make_send_client(send_link0, 1, nullptr, nullptr, 0, nullptr, nullptr);
    auto recv_client1 =
        io_srv->make_recv_client(recv_link1, 1, nullptr, nullptr, 0, nullptr);
    auto send_client1 =
        io_srv->make_send_client(send_link1, 1, nullptr, nullptr, 0, nullptr, nullptr);

    recv_link0->push_back_recv_packet(
        boost::shared_array<uint8_t>(new uint8_t[FRAME_SIZE]), FRAME_SIZE);

    send_client0->release_send_buff(send_client0->get_send_buff(100));
    mock_io_srv->allocate_recv_frames(0, 1);
    recv_client0->release_recv_buff(recv_client0->get_recv_buff(100));

    recv_client0.reset();
    send_client0.reset();

    io_srv->detach_recv_link(recv_link0);
    io_srv->detach_send_link(send_link0);

    // Check other clients continue to work after detaching a pair of links
    recv_link1->push_back_recv_packet(
        boost::shared_array<uint8_t>(new uint8_t[FRAME_SIZE]), FRAME_SIZE);

    mock_io_srv->allocate_recv_frames(1, 1);
    recv_client1->release_recv_buff(recv_client1->get_recv_buff(100));
    send_client1->release_send_buff(send_client1->get_send_buff(100));

    send_client1.reset();
    recv_client1.reset();

    io_srv->detach_recv_link(recv_link1);
    io_srv->detach_send_link(send_link1);

    // Check that we can re-attach a link
    io_srv->attach_recv_link(recv_link0);
    io_srv->attach_send_link(send_link0);

    auto recv_client2 =
        io_srv->make_recv_client(recv_link0, 1, nullptr, nullptr, 0, nullptr);
    auto send_client2 =
        io_srv->make_send_client(send_link0, 1, nullptr, nullptr, 0, nullptr, nullptr);

    recv_link0->push_back_recv_packet(
        boost::shared_array<uint8_t>(new uint8_t[FRAME_SIZE]), FRAME_SIZE);

    send_client2->release_send_buff(send_client2->get_send_buff(100));
    mock_io_srv->allocate_recv_frames(2, 1);
    recv_client2->release_recv_buff(recv_client2->get_recv_buff(100));
}
