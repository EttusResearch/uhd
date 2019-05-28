//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "common/mock_link.hpp"
#include "common/mock_transport.hpp"
#include <uhdlib/transport/inline_io_service.hpp>
#include <boost/test/unit_test.hpp>

using namespace uhd::transport;

static mock_send_link::sptr make_send_link(size_t num_frames)
{
    const mock_send_link::link_params params = {1000, num_frames};
    return std::make_shared<mock_send_link>(params);
}

static mock_recv_link::sptr make_recv_link(size_t num_frames)
{
    const mock_recv_link::link_params params = {1000, num_frames};
    return std::make_shared<mock_recv_link>(params);
}

static mock_send_transport::sptr make_send_xport(io_service::sptr io_srv,
    send_link_if::sptr send_link,
    recv_link_if::sptr recv_link,
    uint16_t dst_addr,
    uint16_t src_addr,
    uint32_t credits)
{
    return std::make_shared<mock_send_transport>(
        io_srv, send_link, recv_link, dst_addr, src_addr, credits);
}

static mock_recv_transport::sptr make_recv_xport(io_service::sptr io_srv,
    recv_link_if::sptr recv_link,
    send_link_if::sptr send_link,
    uint16_t dst_addr,
    uint16_t src_addr,
    uint32_t credits)
{
    return std::make_shared<mock_recv_transport>(
        io_srv, recv_link, send_link, dst_addr, src_addr, credits);
}

BOOST_AUTO_TEST_CASE(test_construction)
{
    auto io_srv    = inline_io_service::make();
    auto send_link = make_send_link(40);
    io_srv->attach_send_link(send_link);
    auto recv_link = make_recv_link(40);
    io_srv->attach_recv_link(recv_link);
    auto send_xport = make_send_xport(io_srv, send_link, recv_link, 1, 2, 32);

    auto send_buff = send_xport->get_data_buff(0);
    send_buff->set_packet_size(0);
    send_xport->release_data_buff(send_buff, 0);
    send_xport.reset();

    auto recv_xport = make_recv_xport(io_srv, recv_link, send_link, 1, 2, 32);
    uint32_t msg;
    UHD_ASSERT_THROW(recv_xport->get_msg(msg) == false);
}

BOOST_AUTO_TEST_CASE(test_io)
{
    auto io_srv     = inline_io_service::make();
    auto send_link0 = make_send_link(40);
    io_srv->attach_send_link(send_link0);
    auto recv_link0 = make_recv_link(40);
    io_srv->attach_recv_link(recv_link0);
    auto send_xport = make_send_xport(io_srv, send_link0, recv_link0, 1, 2, 32);

    auto send_link1 = make_send_link(40);
    io_srv->attach_send_link(send_link1);
    auto recv_link1 = make_recv_link(40);
    io_srv->attach_recv_link(recv_link1);
    auto recv_xport = make_recv_xport(io_srv, recv_link1, send_link1, 1, 2, 32);

    /* FIXME: Testing async messages requires the dummy read -- To not have it, needs recv
     * mux + separate recv queue */
    send_xport->put_msg(0xa5d3b33f, 0);
    auto packet = send_link0->pop_send_packet();
    recv_link1->push_back_recv_packet(packet.first, packet.second);
    auto recv_buff = recv_xport->get_data_buff(0);
    if (recv_buff) {
        recv_xport->release_data_buff(std::move(recv_buff));
    }
    uint32_t msg;
    UHD_ASSERT_THROW(recv_xport->get_msg(msg));
    UHD_ASSERT_THROW(msg == 0xa5d3b33f);

    auto send_buff = send_xport->get_data_buff(0);
    UHD_ASSERT_THROW(send_buff);
    auto buff_data = send_xport->buff_to_data(send_buff.get());
    UHD_ASSERT_THROW(buff_data.second >= 16);
    uint32_t* data = buff_data.first;
    for (size_t i = 0; i < 16; i++) {
        data[i] = (uint32_t)i;
    }

    send_xport->release_data_buff(send_buff, 16);
    packet = send_link0->pop_send_packet();
    recv_link1->push_back_recv_packet(packet.first, packet.second);

    recv_buff = recv_xport->get_data_buff(0);
    UHD_ASSERT_THROW(recv_buff);
    auto recv_data = recv_xport->buff_to_data(recv_buff.get());
    UHD_ASSERT_THROW(recv_data.second == 16);
    data = recv_data.first;
    for (size_t i = 0; i < 16; i++) {
        UHD_ASSERT_THROW(data[i] == (uint32_t)i);
    }
    recv_xport->release_data_buff(std::move(recv_buff));
}

BOOST_AUTO_TEST_CASE(test_muxed_io)
{
    auto io_srv    = inline_io_service::make();
    auto send_link = make_send_link(80);
    io_srv->attach_send_link(send_link);
    auto recv_link = make_recv_link(80);
    io_srv->attach_recv_link(recv_link);
    auto send_xport = make_send_xport(io_srv, send_link, recv_link, 1, 2, 32);
    auto recv_xport = make_recv_xport(io_srv, recv_link, send_link, 1, 2, 32);

    /* Send a sideband message */
    send_xport->put_msg(0xa5d3b33f, 0);

    /* Send some normal data */
    auto send_buff = send_xport->get_data_buff(0);
    UHD_ASSERT_THROW(send_buff);
    auto buff_data = send_xport->buff_to_data(send_buff.get());
    UHD_ASSERT_THROW(buff_data.second >= 16);
    uint32_t* data = buff_data.first;
    for (size_t i = 0; i < 16; i++) {
        data[i] = (uint32_t)i;
    }
    send_xport->release_data_buff(send_buff, 16);

    /* Move the two packets over */
    auto packet = send_link->pop_send_packet();
    recv_link->push_back_recv_packet(packet.first, packet.second);
    packet = send_link->pop_send_packet();
    recv_link->push_back_recv_packet(packet.first, packet.second);

    /* Try to receive the data
     * (message won't arrive unless we try to get the data first)
     * However, the message should be processed and enqueued here
     */
    auto recv_buff = recv_xport->get_data_buff(0);
    UHD_ASSERT_THROW(recv_buff);
    auto recv_data = recv_xport->buff_to_data(recv_buff.get());
    UHD_ASSERT_THROW(recv_data.second == 16);
    data = recv_data.first;
    for (size_t i = 0; i < 16; i++) {
        UHD_ASSERT_THROW(data[i] == (uint32_t)i);
    }
    recv_xport->release_data_buff(std::move(recv_buff));

    /* Now can get the message */
    uint32_t msg;
    UHD_ASSERT_THROW(recv_xport->get_msg(msg));
    UHD_ASSERT_THROW(msg == 0xa5d3b33f);
}

/*
BOOST_AUTO_TEST_CASE(test_oversubscribed)
{
    auto io_srv = inline_io_service::make();
    auto send_link = make_send_link(32);
    io_srv->attach_send_link(send_link);
    auto recv_link = make_recv_link(32);
    io_srv->attach_recv_link(recv_link);
    auto send_xport = make_send_xport(io_srv, send_link, recv_link, 1, 2, 32);

    auto send_buff = send_xport->get_data_buff(0);
    send_buff->set_packet_size(0);
    send_xport->release_data_buff(send_buff, 0);
    send_xport.reset();

    auto recv_xport = make_recv_xport(io_srv, recv_link, send_link, 1, 2, 32);
    uint32_t msg;
    UHD_ASSERT_THROW(recv_xport->get_msg(msg) == false);
}
*/
