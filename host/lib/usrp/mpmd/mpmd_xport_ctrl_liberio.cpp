//
// Copyright 2017 Ettus Research, National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "mpmd_xport_ctrl_liberio.hpp"
#include "../transport/liberio_zero_copy.hpp"
#include <uhd/transport/udp_zero_copy.hpp>
#include <uhd/utils/byteswap.hpp>

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
    const size_t LIBERIO_PAGES_PER_BUF  = 2;
    //! Number of descriptors that liberio allocates (receive)
    const size_t LIBERIO_NUM_RECV_FRAMES = 128;
    //! Number of descriptors that liberio allocates (send)
    const size_t LIBERIO_NUM_SEND_FRAMES = 128;

    uint32_t extract_sid_from_pkt(void* pkt, size_t) {
        return uhd::sid_t(uhd::wtohx(static_cast<const uint32_t*>(pkt)[1]))
            .get_dst();
    }

}

mpmd_xport_ctrl_liberio::mpmd_xport_ctrl_liberio(
        const uhd::device_addr_t& mb_args
) : _mb_args(mb_args)
  , _recv_args(filter_args(mb_args, "recv"))
  , _send_args(filter_args(mb_args, "send"))
{
    // nop
}


uhd::both_xports_t
mpmd_xport_ctrl_liberio::make_transport(
        mpmd_xport_mgr::xport_info_t &xport_info,
        const usrp::device3_impl::xport_type_t xport_type,
        const uhd::device_addr_t& /*xport_args_*/
) {
    transport::zero_copy_xport_params default_buff_args;
    /* default ones for RX / TX, override below */

    default_buff_args.send_frame_size = get_mtu(uhd::TX_DIRECTION);
    default_buff_args.recv_frame_size = get_mtu(uhd::RX_DIRECTION);
    default_buff_args.num_recv_frames = LIBERIO_NUM_RECV_FRAMES;
    default_buff_args.num_send_frames = LIBERIO_NUM_SEND_FRAMES;

    if (xport_type == usrp::device3_impl::CTRL) {
        default_buff_args.send_frame_size = LIBERIO_CTRL_FRAME_MAX_SIZE;
        default_buff_args.recv_frame_size = LIBERIO_CTRL_FRAME_MAX_SIZE;
    } else if (xport_type == usrp::device3_impl::ASYNC_MSG) {
        default_buff_args.send_frame_size = LIBERIO_ASYNC_FRAME_MAX_SIZE;
        default_buff_args.recv_frame_size = LIBERIO_ASYNC_FRAME_MAX_SIZE;
    } else if (xport_type == usrp::device3_impl::RX_DATA) {
        default_buff_args.send_frame_size = LIBERIO_FC_FRAME_MAX_SIZE;
    } else {
        default_buff_args.recv_frame_size = LIBERIO_FC_FRAME_MAX_SIZE;
    }

    const std::string tx_dev = xport_info["tx_dev"];
    const std::string rx_dev = xport_info["rx_dev"];

    both_xports_t xports;
    xports.endianness = uhd::ENDIANNESS_LITTLE;
    xports.send_sid = sid_t(xport_info["send_sid"]);
    xports.recv_sid = xports.send_sid.reversed();

    // this is kinda ghetto: scale buffer for muxed xports since we share the
    // buffer...
    float divisor = 1;
    if (xport_type == usrp::device3_impl::CTRL)
        divisor = 16;
    else if (xport_type == usrp::device3_impl::ASYNC_MSG)
        divisor = 4;


    //if (xport_info["muxed"] == "True") {
    //// FIXME tbw
    //}
    if (xport_type == usrp::device3_impl::CTRL) {
        UHD_ASSERT_THROW(xport_info["muxed"] == "True");
        if (not _ctrl_dma_xport) {
            default_buff_args.send_frame_size = LIBERIO_CTRL_FRAME_MAX_SIZE;
            default_buff_args.recv_frame_size = LIBERIO_CTRL_FRAME_MAX_SIZE;
            _ctrl_dma_xport = make_muxed_liberio_xport(tx_dev, rx_dev,
                    default_buff_args, int(divisor));
        }

        UHD_LOGGER_TRACE("MPMD")
            << "Making (muxed) stream with num " << xports.recv_sid.get_dst();
        xports.recv = _ctrl_dma_xport->make_stream(xports.recv_sid.get_dst());
    } else if (xport_type == usrp::device3_impl::ASYNC_MSG) {
        UHD_ASSERT_THROW(xport_info["muxed"] == "True");
        if (not _async_msg_dma_xport) {
            default_buff_args.send_frame_size = LIBERIO_ASYNC_FRAME_MAX_SIZE;
            default_buff_args.recv_frame_size = LIBERIO_ASYNC_FRAME_MAX_SIZE;
            _async_msg_dma_xport = make_muxed_liberio_xport(
                    tx_dev, rx_dev, default_buff_args, int(divisor));
        }

        UHD_LOGGER_TRACE("MPMD")
            << "making (muxed) stream with num " << xports.recv_sid.get_dst();
        xports.recv =
            _async_msg_dma_xport->make_stream(xports.recv_sid.get_dst());
    } else {
        xports.recv = transport::liberio_zero_copy::make(
                    tx_dev, rx_dev, default_buff_args);
    }

    transport::udp_zero_copy::buff_params buff_params;
    buff_params.recv_buff_size =
        float(default_buff_args.recv_frame_size) *
        float(default_buff_args.num_recv_frames) / divisor;
    buff_params.send_buff_size =
        float(default_buff_args.send_frame_size) *
        float(default_buff_args.num_send_frames) / divisor;


    // Finish both_xports_t object and return:
    xports.recv_buff_size = buff_params.recv_buff_size;
    xports.send_buff_size = buff_params.send_buff_size;
    xports.send = xports.recv;
    return xports;
}

bool mpmd_xport_ctrl_liberio::is_valid(
    const mpmd_xport_mgr::xport_info_t& xport_info
) const {
    return xport_info.at("type") == "liberio";
}

size_t mpmd_xport_ctrl_liberio::get_mtu(
    const uhd::direction_t /* dir */
) const {
    return LIBERIO_PAGES_PER_BUF * getpagesize();
}

uhd::transport::muxed_zero_copy_if::sptr
mpmd_xport_ctrl_liberio::make_muxed_liberio_xport(
        const std::string &tx_dev,
        const std::string &rx_dev,
        const uhd::transport::zero_copy_xport_params &buff_args,
        const size_t max_muxed_ports
) {
    auto base_xport = transport::liberio_zero_copy::make(
            tx_dev, rx_dev, buff_args);

    return uhd::transport::muxed_zero_copy_if::make(
            base_xport, extract_sid_from_pkt, max_muxed_ports);
}

