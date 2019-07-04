//
// Copyright 2017 Ettus Research, National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "mpmd_link_if_ctrl_liberio.hpp"
#include <uhd/rfnoc/constants.hpp>
#include <uhd/utils/byteswap.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/transport/inline_io_service.hpp>
#include <uhdlib/transport/liberio_link.hpp>

using namespace uhd;
using namespace uhd::transport;
using namespace uhd::mpmd::xport;

const uhd::rfnoc::chdr::chdr_packet_factory mpmd_link_if_ctrl_liberio::_pkt_factory(
    uhd::rfnoc::CHDR_W_64, ENDIANNESS_LITTLE);

namespace {

//! The default MTU will be this number times the page size
const size_t LIBERIO_PAGES_PER_BUF = 2;
//! The default MTU
const size_t LIBERIO_DEFAULT_MTU = LIBERIO_PAGES_PER_BUF * getpagesize();
//! The default link_rate (8 Bytes * 200 MHz)
const double LIBERIO_DEFAULT_LINK_RATE = 200e6 * 8;
//! Number of descriptors that liberio allocates (receive)
const size_t LIBERIO_NUM_RECV_FRAMES = 128;
//! Number of descriptors that liberio allocates (send)
const size_t LIBERIO_NUM_SEND_FRAMES = 128;
//! MTU for largest non-data packet accepted (arbitrarily-determined...)
// Note: Management frames must fit here, and it determines the padded RX size
const size_t LIBERIO_MAX_NONDATA_PACKET_SIZE = 128;

std::vector<mpmd_link_if_ctrl_liberio::liberio_link_info_t>
get_liberio_info_from_xport_info(
    const mpmd_link_if_mgr::xport_info_list_t& link_info_list)
{
    std::vector<mpmd_link_if_ctrl_liberio::liberio_link_info_t> result;
    for (const auto& link_info : link_info_list) {
        if (!link_info.count("tx_dev")) {
            UHD_LOG_ERROR("MPMD::XPORT::LIBERIO",
                "Invalid response from get_chdr_link_options()! No `tx_dev' key!");
            throw uhd::runtime_error(
                "Invalid response from get_chdr_link_options()! No `tx_dev' key!");
        }
        if (!link_info.count("rx_dev")) {
            UHD_LOG_ERROR("MPMD::XPORT::LIBERIO",
                "Invalid response from get_chdr_link_options()! No `rx_dev' key!");
            throw uhd::runtime_error(
                "Invalid response from get_chdr_link_options()! No `rx_dev' key!");
        }
        const std::string tx_dev = link_info.at("tx_dev");
        const std::string rx_dev = link_info.at("rx_dev");
        result.emplace_back(
            mpmd_link_if_ctrl_liberio::liberio_link_info_t{tx_dev, rx_dev});
    }

    return result;
}

} // namespace


/******************************************************************************
 * Structors
 *****************************************************************************/
mpmd_link_if_ctrl_liberio::mpmd_link_if_ctrl_liberio(const uhd::device_addr_t& mb_args,
    const mpmd_link_if_mgr::xport_info_list_t& xport_info)
    : _mb_args(mb_args)
    , _recv_args(filter_args(mb_args, "recv"))
    , _send_args(filter_args(mb_args, "send"))
    , _dma_channels(get_liberio_info_from_xport_info(xport_info))
    , _link_rate(LIBERIO_DEFAULT_LINK_RATE) // FIXME
{
    // nop
}

/******************************************************************************
 * API
 *****************************************************************************/
uhd::transport::both_links_t mpmd_link_if_ctrl_liberio::get_link(const size_t link_idx,
    const uhd::transport::link_type_t link_type,
    const uhd::device_addr_t& link_args)
{
    UHD_ASSERT_THROW(link_idx == 0);
    if (_next_channel >= _dma_channels.size()) {
        UHD_LOG_ERROR(
            "MPMD::XPORT::LIBERIO", "Cannot create liberio link: DMA channels exhausted");
        throw uhd::runtime_error("Cannot create liberio link: DMA channels exhausted");
    }
    auto link_info = _dma_channels.at(_next_channel++);

    /* FIXME: Should have common infrastructure for creating I/O services */
    auto io_srv = uhd::transport::inline_io_service::make();
    link_params_t link_params;
    if (link_type == link_type_t::RX_DATA) {
        link_params.recv_frame_size = get_mtu(uhd::RX_DIRECTION); // FIXME
        link_params.send_frame_size = LIBERIO_MAX_NONDATA_PACKET_SIZE; // FIXME
    } else if (link_type == link_type_t::TX_DATA) {
        link_params.recv_frame_size = LIBERIO_MAX_NONDATA_PACKET_SIZE; // FIXME
        link_params.send_frame_size = get_mtu(uhd::TX_DIRECTION); // FIXME
    } else {
        link_params.recv_frame_size = LIBERIO_MAX_NONDATA_PACKET_SIZE; // FIXME
        link_params.send_frame_size = LIBERIO_MAX_NONDATA_PACKET_SIZE; // FIXME
    }
    link_params.num_recv_frames = LIBERIO_NUM_RECV_FRAMES; // FIXME
    link_params.num_send_frames = LIBERIO_NUM_SEND_FRAMES; // FIXME

    // Liberio doesn't need in-band flow control, so pretend have very large buffers
    link_params.recv_buff_size = std::numeric_limits<size_t>::max();
    link_params.send_buff_size = std::numeric_limits<size_t>::max();
    auto link                  = uhd::transport::liberio_link::make(
        link_info.first, link_info.second, link_params);
    io_srv->attach_send_link(link);
    io_srv->attach_recv_link(link);
    return std::tuple<io_service::sptr,
        send_link_if::sptr,
        size_t,
        recv_link_if::sptr,
        size_t,
        bool>(io_srv,
        link,
        link_params.send_buff_size,
        link,
        link_params.recv_buff_size,
        false);
}

size_t mpmd_link_if_ctrl_liberio::get_mtu(const uhd::direction_t /*dir*/) const
{
    return LIBERIO_DEFAULT_MTU; // FIXME
}
