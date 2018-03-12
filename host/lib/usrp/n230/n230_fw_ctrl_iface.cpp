//
// Copyright 2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "n230_fw_ctrl_iface.hpp"

#include <uhd/utils/byteswap.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/exception.hpp>
#include <boost/format.hpp>
#include <boost/asio.hpp> //used for htonl and ntohl
#include "n230_fw_comm_protocol.h"
#include <cstring>

namespace uhd { namespace usrp { namespace n230 {

//----------------------------------------------------------
// Factory method
//----------------------------------------------------------
uhd::wb_iface::sptr n230_fw_ctrl_iface::make(
    uhd::transport::udp_simple::sptr udp_xport,
    const uint16_t product_id,
    const bool verbose)
{
    return wb_iface::sptr(new n230_fw_ctrl_iface(udp_xport, product_id, verbose));
}

//----------------------------------------------------------
// udp_fw_ctrl_iface
//----------------------------------------------------------

n230_fw_ctrl_iface::n230_fw_ctrl_iface(
    uhd::transport::udp_simple::sptr udp_xport,
    const uint16_t product_id,
    const bool verbose) :
    _product_id(product_id), _verbose(verbose), _udp_xport(udp_xport),
    _seq_num(0)
{
    flush();
    peek32(0);
}

n230_fw_ctrl_iface::~n230_fw_ctrl_iface()
{
    flush();
}

void n230_fw_ctrl_iface::flush()
{
    boost::mutex::scoped_lock lock(_mutex);
    _flush();
}

void n230_fw_ctrl_iface::poke32(const wb_addr_type addr, const uint32_t data)
{
    boost::mutex::scoped_lock lock(_mutex);

    for (size_t i = 1; i <= NUM_RETRIES; i++) {
        try {
            _poke32(addr, data);
            return;
        } catch(const std::exception &ex) {
            const std::string error_msg = str(boost::format(
                "udp fw poke32 failure #%u\n%s") % i % ex.what());
            if (_verbose) UHD_LOGGER_WARNING("N230") << error_msg ;
            if (i == NUM_RETRIES) throw uhd::io_error(error_msg);
        }
    }
}

uint32_t n230_fw_ctrl_iface::peek32(const wb_addr_type addr)
{
    boost::mutex::scoped_lock lock(_mutex);

    for (size_t i = 1; i <= NUM_RETRIES; i++) {
        try {
            return _peek32(addr);
        } catch(const std::exception &ex) {
            const std::string error_msg = str(boost::format(
                "udp fw peek32 failure #%u\n%s") % i % ex.what());
            if (_verbose) UHD_LOGGER_WARNING("N230") << error_msg ;
            if (i == NUM_RETRIES) throw uhd::io_error(error_msg);
        }
    }
    return 0;
}

void n230_fw_ctrl_iface::_poke32(const wb_addr_type addr, const uint32_t data)
{
    //Load request struct
    fw_comm_pkt_t request;
    request.id = uhd::htonx<uint32_t>(FW_COMM_GENERATE_ID(_product_id));
    request.flags = uhd::htonx<uint32_t>(FW_COMM_FLAGS_ACK | FW_COMM_CMD_POKE32);
    request.sequence = uhd::htonx<uint32_t>(_seq_num++);
    request.addr = uhd::htonx(addr);
    request.data_words = 1;
    request.data[0] = uhd::htonx(data);

    //Send request
    _flush();
    _udp_xport->send(boost::asio::buffer(&request, sizeof(request)));

    //Recv reply
    fw_comm_pkt_t reply;
    const size_t nbytes = _udp_xport->recv(boost::asio::buffer(&reply, sizeof(reply)), 1.0);
    if (nbytes == 0) throw uhd::io_error("udp fw poke32 - reply timed out");

    //Sanity checks
    const size_t flags = uhd::ntohx<uint32_t>(reply.flags);
    UHD_ASSERT_THROW(nbytes == sizeof(reply));
    UHD_ASSERT_THROW(not (flags & FW_COMM_FLAGS_ERROR_MASK));
    UHD_ASSERT_THROW(flags & FW_COMM_CMD_POKE32);
    UHD_ASSERT_THROW(flags & FW_COMM_FLAGS_ACK);
    UHD_ASSERT_THROW(reply.sequence == request.sequence);
    UHD_ASSERT_THROW(reply.addr == request.addr);
    UHD_ASSERT_THROW(reply.data[0] == request.data[0]);
}

uint32_t n230_fw_ctrl_iface::_peek32(const wb_addr_type addr)
{
    //Load request struct
    fw_comm_pkt_t request;
    request.id = uhd::htonx<uint32_t>(FW_COMM_GENERATE_ID(_product_id));
    request.flags = uhd::htonx<uint32_t>(FW_COMM_FLAGS_ACK | FW_COMM_CMD_PEEK32);
    request.sequence = uhd::htonx<uint32_t>(_seq_num++);
    request.addr = uhd::htonx(addr);
    request.data_words = 1;
    request.data[0] = 0;

    //Send request
    _flush();
    _udp_xport->send(boost::asio::buffer(&request, sizeof(request)));

    //Recv reply
    fw_comm_pkt_t reply;
    const size_t nbytes = _udp_xport->recv(boost::asio::buffer(&reply, sizeof(reply)), 1.0);
    if (nbytes == 0) throw uhd::io_error("udp fw peek32 - reply timed out");

    //Sanity checks
    const size_t flags = uhd::ntohx<uint32_t>(reply.flags);
    UHD_ASSERT_THROW(nbytes == sizeof(reply));
    UHD_ASSERT_THROW(not (flags & FW_COMM_FLAGS_ERROR_MASK));
    UHD_ASSERT_THROW(flags & FW_COMM_CMD_PEEK32);
    UHD_ASSERT_THROW(flags & FW_COMM_FLAGS_ACK);
    UHD_ASSERT_THROW(reply.sequence == request.sequence);
    UHD_ASSERT_THROW(reply.addr == request.addr);

    //return result!
    return uhd::ntohx<uint32_t>(reply.data[0]);
}

void n230_fw_ctrl_iface::_flush(void)
{
    char buff[FW_COMM_PROTOCOL_MTU] = {};
    while (_udp_xport->recv(boost::asio::buffer(buff), 0.0)) {
        /*NOP*/
    }
}

std::vector<std::string> n230_fw_ctrl_iface::discover_devices(
    const std::string& addr_hint, const std::string& port,
    uint16_t product_id)
{
    std::vector<std::string> addrs;

    //Create a UDP transport to communicate:
    //Some devices will cause a throw when opened for a broadcast address.
    //We print and recover so the caller can loop through all bcast addrs.
    uhd::transport::udp_simple::sptr udp_bcast_xport;
    try {
        udp_bcast_xport = uhd::transport::udp_simple::make_broadcast(addr_hint, port);
    } catch(const std::exception &e) {
        UHD_LOGGER_ERROR("N230") << boost::format("Cannot open UDP transport on %s for discovery\n%s")
        % addr_hint % e.what() ;
        return addrs;
    }

    //Send dummy request
    fw_comm_pkt_t request;
    std::memset(&request, 0, sizeof(request));
    request.id = uhd::htonx<uint32_t>(FW_COMM_GENERATE_ID(product_id));
    request.flags = uhd::htonx<uint32_t>(FW_COMM_FLAGS_ACK|FW_COMM_CMD_ECHO);
    request.sequence = uhd::htonx<uint32_t>(std::rand());
    udp_bcast_xport->send(boost::asio::buffer(&request, sizeof(request)));

    //loop for replies until timeout
    while (true) {
        char buff[FW_COMM_PROTOCOL_MTU] = {};
        const size_t nbytes = udp_bcast_xport->recv(boost::asio::buffer(buff), 0.050);
        if (nbytes != sizeof(fw_comm_pkt_t)) break; //No more responses or responses are invalid

        const fw_comm_pkt_t *reply = (const fw_comm_pkt_t *)buff;
        if (request.id       == reply->id &&
            request.flags    == reply->flags &&
            request.sequence == reply->sequence)
        {
            addrs.push_back(udp_bcast_xport->get_recv_addr());
        }
    }

    return addrs;
}

uint32_t n230_fw_ctrl_iface::get_iface_id(
    const std::string& addr, const std::string& port,
    uint16_t product_id)
{
    uhd::transport::udp_simple::sptr udp_xport =
        uhd::transport::udp_simple::make_connected(addr, port);

    //Send dummy request
    fw_comm_pkt_t request;
    request.id = uhd::htonx<uint32_t>(FW_COMM_GENERATE_ID(product_id));
    request.flags = uhd::htonx<uint32_t>(FW_COMM_FLAGS_ACK|FW_COMM_CMD_ECHO);
    request.sequence = uhd::htonx<uint32_t>(std::rand());
    udp_xport->send(boost::asio::buffer(&request, sizeof(request)));

    //loop for replies until timeout
    char buff[FW_COMM_PROTOCOL_MTU] = {};
    const size_t nbytes = udp_xport->recv(boost::asio::buffer(buff), 1.0);

    const fw_comm_pkt_t *reply = (const fw_comm_pkt_t *)buff;
    if (nbytes            >  0 &&
        request.id        == reply->id &&
        request.flags     == reply->flags &&
        request.sequence  == reply->sequence)
    {
        return uhd::ntohx<uint32_t>(reply->data[0]);
    } else {
        throw uhd::io_error("udp get_iface_id - bad response");
    }
}

}}} //namespace
