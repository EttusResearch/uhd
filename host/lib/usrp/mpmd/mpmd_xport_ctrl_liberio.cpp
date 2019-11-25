//
// Copyright 2017 Ettus Research, National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "mpmd_xport_ctrl_liberio.hpp"
#include "../transport/liberio_zero_copy.hpp"
#include <uhd/transport/udp_zero_copy.hpp>
#include <uhd/utils/byteswap.hpp>
#include <uhd/rfnoc/constants.hpp>

using namespace uhd;
using namespace uhd::mpmd::xport;

namespace {

//! Max frame size of a control packet in bytes
const size_t LIBERIO_CTRL_FRAME_MAX_SIZE = 128;
//! Max frame size of an async message packet in bytes
const size_t LIBERIO_ASYNC_FRAME_MAX_SIZE = 256;
//! Max frame size of a flow control packet in bytes
const size_t LIBERIO_FC_FRAME_MAX_SIZE = 64;
//! The max MTU will be this number times the page size
const size_t LIBERIO_PAGES_PER_BUF = 2;
//! Number of descriptors that liberio allocates (receive)
const size_t LIBERIO_NUM_RECV_FRAMES = 128;
//! Number of descriptors that liberio allocates (send)
const size_t LIBERIO_NUM_SEND_FRAMES = 128;

//! Default timeout value for receiving muxed control messages
constexpr double LIBERIO_DEFAULT_RECV_TIMEOUT_CTRL = 0.5; // seconds
//! Default timeout value for receiving muxed async messages
constexpr double LIBERIO_DEFAULT_RECV_TIMEOUT_ASYNC = 0.1; // seconds
//! Default timeout value for receiving muxed data packets
constexpr double LIBERIO_DEFAULT_RECV_TIMEOUT_STRM = 0.1; // seconds

uint32_t extract_sid_from_pkt(void* pkt, size_t)
{
    return uhd::sid_t(uhd::wtohx(static_cast<const uint32_t*>(pkt)[1])).get_dst();
}

} // namespace

mpmd_xport_ctrl_liberio::mpmd_xport_ctrl_liberio(const uhd::device_addr_t& mb_args)
    : _mb_args(mb_args)
    , _recv_args(filter_args(mb_args, "recv"))
    , _send_args(filter_args(mb_args, "send"))
{
    // nop
}


uhd::both_xports_t mpmd_xport_ctrl_liberio::make_transport(
    mpmd_xport_mgr::xport_info_t& xport_info,
    const usrp::device3_impl::xport_type_t xport_type,
    const uhd::device_addr_t& xport_args_)
{
    auto xport_args = (xport_type == usrp::device3_impl::CTRL) ?
                        uhd::device_addr_t() : xport_args_;

    // Constrain by this transport's MTU and the MTU passed in
    const size_t send_mtu = std::min(get_mtu(uhd::TX_DIRECTION),
        xport_args.cast<size_t>("mtu", get_mtu(uhd::TX_DIRECTION)));
    const size_t recv_mtu = std::min(get_mtu(uhd::RX_DIRECTION),
        xport_args.cast<size_t>("mtu", get_mtu(uhd::RX_DIRECTION)));
    size_t send_frame_size = xport_args.cast<size_t>("send_frame_size", send_mtu);
    size_t recv_frame_size = xport_args.cast<size_t>("recv_frame_size", recv_mtu);

    // Check any user supplied frame sizes and constrain to MTU
    if (xport_args.has_key("send_frame_size") and
        xport_type == usrp::device3_impl::TX_DATA)
    {
        if (send_frame_size > send_mtu) {
            UHD_LOGGER_WARNING("MPMD")
                << boost::format("Requested send_frame_size of %d exceeds the "
                                 "maximum supported by the hardware.  Using %d.")
                       % send_frame_size % send_mtu;
            send_frame_size = send_mtu;
        }
    }
    if (xport_args.has_key("recv_frame_size") and
        xport_type == usrp::device3_impl::RX_DATA)
    {
        size_t recv_frame_size = xport_args.cast<size_t>("recv_frame_size", recv_mtu);
        if (recv_frame_size > recv_mtu) {
            UHD_LOGGER_WARNING("MPMD")
                << boost::format("Requested recv_frame_size of %d exceeds the "
                                 "maximum supported by the hardware.  Using %d.")
                       % recv_frame_size % recv_mtu;
            recv_frame_size = recv_mtu;
        }
    }

    transport::zero_copy_xport_params default_buff_args;
    /* default ones for RX / TX, override below */

    default_buff_args.send_frame_size = send_mtu;
    default_buff_args.recv_frame_size = recv_mtu;
    default_buff_args.num_recv_frames = LIBERIO_NUM_RECV_FRAMES;
    default_buff_args.num_send_frames = LIBERIO_NUM_SEND_FRAMES;

    if (xport_type == usrp::device3_impl::CTRL) {
        default_buff_args.send_frame_size = LIBERIO_CTRL_FRAME_MAX_SIZE;
        default_buff_args.recv_frame_size = LIBERIO_CTRL_FRAME_MAX_SIZE;
        default_buff_args.num_recv_frames = uhd::rfnoc::CMD_FIFO_SIZE /
                                            uhd::rfnoc::MAX_CMD_PKT_SIZE;
        default_buff_args.num_send_frames = uhd::rfnoc::CMD_FIFO_SIZE /
                                            uhd::rfnoc::MAX_CMD_PKT_SIZE;
    } else if (xport_type == usrp::device3_impl::ASYNC_MSG) {
        default_buff_args.send_frame_size = LIBERIO_ASYNC_FRAME_MAX_SIZE;
        default_buff_args.recv_frame_size = LIBERIO_ASYNC_FRAME_MAX_SIZE;
    } else if (xport_type == usrp::device3_impl::RX_DATA) {
        default_buff_args.recv_frame_size = recv_frame_size;
        default_buff_args.send_frame_size = LIBERIO_FC_FRAME_MAX_SIZE;
    } else {
        default_buff_args.recv_frame_size = LIBERIO_FC_FRAME_MAX_SIZE;
        default_buff_args.send_frame_size = send_frame_size;
    }

    const std::string tx_dev = xport_info["tx_dev"];
    const std::string rx_dev = xport_info["rx_dev"];

    both_xports_t xports;
    xports.lossless = true;
    xports.endianness = uhd::ENDIANNESS_LITTLE;
    xports.send_sid   = sid_t(xport_info["send_sid"]);
    xports.recv_sid   = xports.send_sid.reversed();

    if (xport_type == usrp::device3_impl::CTRL) {
        UHD_ASSERT_THROW(xport_info["muxed"] == "True");
        if (not _ctrl_dma_xport) {
            const double recv_timeout = LIBERIO_DEFAULT_RECV_TIMEOUT_CTRL;
            // We could also try and get the timeout value from xport_args
            _ctrl_dma_xport = make_muxed_liberio_xport(tx_dev,
                rx_dev,
                default_buff_args,
                uhd::rfnoc::MAX_NUM_BLOCKS * uhd::rfnoc::MAX_NUM_PORTS,
                recv_timeout);
        }

        UHD_LOGGER_TRACE("MPMD")
            << "Making (muxed) stream with num " << xports.recv_sid.get_dst();
        xports.recv = _ctrl_dma_xport->make_stream(xports.recv_sid.get_dst());
    } else if (xport_type == usrp::device3_impl::ASYNC_MSG) {
        UHD_ASSERT_THROW(xport_info["muxed"] == "True");
        if (not _async_msg_dma_xport) {
            const double recv_timeout = LIBERIO_DEFAULT_RECV_TIMEOUT_ASYNC;
            // We could also try and get the timeout value from xport_args
            _async_msg_dma_xport = make_muxed_liberio_xport(tx_dev,
                rx_dev,
                default_buff_args,
                uhd::rfnoc::MAX_NUM_BLOCKS * uhd::rfnoc::MAX_NUM_PORTS,
                recv_timeout);
        }

        UHD_LOGGER_TRACE("MPMD")
            << "making (muxed) stream with num " << xports.recv_sid.get_dst();
        xports.recv = _async_msg_dma_xport->make_stream(xports.recv_sid.get_dst());
    } else {
        // Create muxed transport in case of less DMA channels
        if (xport_info["muxed"] == "True") {
            const size_t dma_chan = std::stoul(xport_info["dma_chan"]);
            if (not _data_dma_xport.count(dma_chan)) {
                const double recv_timeout = LIBERIO_DEFAULT_RECV_TIMEOUT_STRM;
                // We could also try and get the timeout value from xport_args
                _data_dma_xport.insert({dma_chan,
                    make_muxed_liberio_xport(tx_dev,
                        rx_dev,
                        default_buff_args,
                        uhd::rfnoc::MAX_NUM_BLOCKS * uhd::rfnoc::MAX_NUM_PORTS,
                        recv_timeout)});
            }

            UHD_LOGGER_TRACE("MPMD")
                << "Making (muxed) stream with num " << xports.recv_sid.get_dst();
            xports.recv =
                _data_dma_xport.at(dma_chan)->make_stream(xports.recv_sid.get_dst());
        }
        else {
            xports.recv =
                transport::liberio_zero_copy::make(tx_dev, rx_dev, default_buff_args);
        }
    }

    // Finish both_xports_t object and return:
    xports.recv_buff_size = default_buff_args.recv_frame_size *
                                default_buff_args.num_recv_frames;
    xports.send_buff_size = default_buff_args.send_frame_size *
                                default_buff_args.num_send_frames;
    xports.send           = xports.recv;
    return xports;
}

bool mpmd_xport_ctrl_liberio::is_valid(
    const mpmd_xport_mgr::xport_info_t& xport_info) const
{
    return xport_info.at("type") == "liberio";
}

size_t mpmd_xport_ctrl_liberio::get_mtu(const uhd::direction_t /*dir*/) const
{
    return LIBERIO_PAGES_PER_BUF * getpagesize();
}

uhd::transport::muxed_zero_copy_if::sptr
mpmd_xport_ctrl_liberio::make_muxed_liberio_xport(const std::string& tx_dev,
    const std::string& rx_dev,
    const uhd::transport::zero_copy_xport_params& buff_args,
    const size_t max_muxed_ports,
    const double recv_timeout_s)
{
    auto base_xport = transport::liberio_zero_copy::make(tx_dev, rx_dev, buff_args);

    return uhd::transport::muxed_zero_copy_if::make(
        base_xport, extract_sid_from_pkt, max_muxed_ports, recv_timeout_s);
}
