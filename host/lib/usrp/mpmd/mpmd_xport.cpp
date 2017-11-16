//
// Copyright 2017 Ettus Research, National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0
//

// make_transport logic for mpmd_impl.


#include "mpmd_impl.hpp"
#include "../transport/liberio_zero_copy.hpp"
#include <uhd/transport/udp_zero_copy.hpp>
#include <uhd/utils/byteswap.hpp>

using namespace uhd;


// TODO this does not consider the liberio use case!
uhd::device_addr_t mpmd_impl::get_rx_hints(size_t /* mb_index */)
{
    //device_addr_t rx_hints = _mb[mb_index].recv_args;
    device_addr_t rx_hints; // TODO don't ignore what the user tells us
    // (default to a large recv buff)
    if (not rx_hints.has_key("recv_buff_size"))
    {
        //For the ethernet transport, the buffer has to be set before creating
        //the transport because it is independent of the frame size and # frames
        //For nirio, the buffer size is not configurable by the user
        #if defined(UHD_PLATFORM_MACOS) || defined(UHD_PLATFORM_BSD)
            //limit buffer resize on macos or it will error
            rx_hints["recv_buff_size"] = boost::lexical_cast<std::string>(MPMD_RX_SW_BUFF_SIZE_ETH_MACOS);
        #elif defined(UHD_PLATFORM_LINUX) || defined(UHD_PLATFORM_WIN32)
            //set to half-a-second of buffering at max rate
            rx_hints["recv_buff_size"] = boost::lexical_cast<std::string>(MPMD_RX_SW_BUFF_SIZE_ETH);
        #endif
    }
    return rx_hints;
}


/******************************************************************************
 * General make_transport() + helpers
 *****************************************************************************/
size_t mpmd_impl::identify_mboard_by_sid(const size_t remote_addr)
{
    for (size_t mb_index = 0; mb_index < _mb.size(); mb_index++) {
        for (size_t xbar_index = 0;
                xbar_index < _mb[mb_index]->num_xbars;
                xbar_index++) {
            if (_mb[mb_index]->get_xbar_local_addr(xbar_index) == remote_addr) {
                return mb_index;
            }
        }
    }
    throw uhd::lookup_error(str(
        boost::format("Cannot identify mboard for remote address %d")
        % remote_addr
    ));
}

both_xports_t mpmd_impl::make_transport(
        const sid_t& dst_address,
        usrp::device3_impl::xport_type_t xport_type,
        const uhd::device_addr_t& args
) {
    const size_t mb_index = identify_mboard_by_sid(dst_address.get_dst_addr());
    // Could also be a map...
    const std::string xport_type_str = [xport_type](){
        switch (xport_type) {
        case CTRL:
            return "CTRL";
        case ASYNC_MSG:
            return "ASYNC_MSG";
        case RX_DATA:
            return "RX_DATA";
        case TX_DATA:
            return "TX_DATA";
        default:
            UHD_THROW_INVALID_CODE_PATH();
        };
    }();

    UHD_LOGGER_TRACE("MPMD")
        << "Creating new transport of type: " << xport_type_str
        << " To mboard: " << mb_index
        << " Destination address: " << dst_address.to_pp_string_hex().substr(6)
        << " User-defined xport args: " << args.to_string()
    ;

    const auto xport_info_list =
        _mb[mb_index]->rpc->request_with_token<xport_info_list_t>(
            "request_xport",
            dst_address.get_dst(),
            _sid_framer++, // FIXME make sure we only increment if actually valid
            xport_type_str
    );
    UHD_LOGGER_TRACE("MPMD")
        << "request_xport() gave us " << xport_info_list.size()
        << " option(s)."
    ;

    // This is not very elegant, and needs some kind of factory model or
    // whatnot.
    auto xport_info = xport_info_list.at(0); // In the future, actually pick one
                                           // sensibly. This is what would
                                           // enable dual Eth support. FIXME
    both_xports_t xports;
    if (xport_info["type"] == "UDP") {
        xports = make_transport_udp(mb_index, xport_info, xport_type, args);
#ifdef HAVE_LIBERIO
    } else if (xport_info["type"] == "liberio") {
        xports = make_transport_liberio(mb_index, xport_info, xport_type, args);
#endif
    } else {
        UHD_THROW_INVALID_CODE_PATH();
    }

    UHD_LOGGER_TRACE("MPMD")
        << "xport info: send_sid==" << xports.send_sid.to_pp_string_hex()
        << " recv_sid==" << xports.recv_sid.to_pp_string_hex()
        << " endianness=="
            << (xports.endianness == uhd::ENDIANNESS_BIG ? "BE" : "LE")
        << " recv_buff_size==" << xports.recv_buff_size
        << " send_buff_size==" << xports.send_buff_size
    ;

    return xports;
}


/******************************************************************************
 * UDP Transport
 *****************************************************************************/
both_xports_t mpmd_impl::make_transport_udp(
        const size_t mb_index,
        xport_info_t &xport_info,
        const xport_type_t /*xport_type*/,
        const uhd::device_addr_t& xport_args
) {
    auto &rpc = _mb[mb_index]->rpc;

    transport::zero_copy_xport_params default_buff_args;
    // Create actual UDP transport
    // TODO don't hardcode these
    default_buff_args.send_frame_size = 8000;
    default_buff_args.recv_frame_size = 8000;
    default_buff_args.num_recv_frames = 32;
    default_buff_args.num_send_frames = 32;

    transport::udp_zero_copy::buff_params buff_params;
    auto recv = transport::udp_zero_copy::make(
        xport_info["ipv4"],
        xport_info["port"],
        default_buff_args,
        buff_params,
        xport_args
    );
    const uint16_t port = recv->get_local_port();
    const std::string src_ip_addr = recv->get_local_addr();
    xport_info["src_port"] = std::to_string(port);
    xport_info["src_ipv4"] = src_ip_addr;

    // Communicate it all back to MPM
    if (not rpc->request_with_token<bool>("commit_xport", xport_info)) {
        UHD_LOG_ERROR("MPMD", "Failed to create UDP transport!");
        throw uhd::runtime_error("commit_xport() failed!");
    }

    // Create both_xports_t object and finish:
    both_xports_t xports;
    xports.endianness = uhd::ENDIANNESS_BIG;
    xports.send_sid = sid_t(xport_info["send_sid"]);
    xports.recv_sid = xports.send_sid.reversed();
    xports.recv_buff_size = buff_params.recv_buff_size;
    xports.send_buff_size = buff_params.send_buff_size;
    xports.recv = recv; // Note: This is a type cast!
    xports.send = recv; // This too
    return xports;
}

/******************************************************************************
 * Liberio Transport
 *****************************************************************************/
#ifdef HAVE_LIBERIO
static uint32_t extract_sid_from_pkt(void* pkt, size_t) {
    return uhd::sid_t(uhd::wtohx(static_cast<const uint32_t*>(pkt)[1]))
        .get_dst();
}

uhd::transport::muxed_zero_copy_if::sptr mpmd_impl::make_muxed_liberio_xport(
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

both_xports_t mpmd_impl::make_transport_liberio(
        const size_t mb_index,
        xport_info_t &xport_info,
        const xport_type_t xport_type,
        const uhd::device_addr_t& xport_args
) {
    auto &rpc = _mb[mb_index]->rpc;

    transport::zero_copy_xport_params default_buff_args;
    /* default ones for RX / TX, override below */
    default_buff_args.send_frame_size = 4 * getpagesize();
    default_buff_args.recv_frame_size = 4 * getpagesize();
    default_buff_args.num_recv_frames = 128;
    default_buff_args.num_send_frames = 128;

    if (xport_type == CTRL) {
      default_buff_args.send_frame_size = 128;
      default_buff_args.recv_frame_size = 128;
    } else if (xport_type == ASYNC_MSG) {
      default_buff_args.send_frame_size = 256;
      default_buff_args.recv_frame_size = 256;
    } else if (xport_type == RX_DATA) {
      default_buff_args.send_frame_size = 64;
    } else {
      default_buff_args.recv_frame_size = 64;
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
    if (xport_type == CTRL)
      divisor = 16;
    else if (xport_type == ASYNC_MSG)
      divisor = 4;


    //if (xport_info["muxed"] == "True") {
    //// FIXME tbw
    //}
    if (xport_type == CTRL) {
        UHD_ASSERT_THROW(xport_info["muxed"] == "True");
        if (not _ctrl_dma_xport) {
            default_buff_args.send_frame_size = 128;
            default_buff_args.recv_frame_size = 128;
            _ctrl_dma_xport = make_muxed_liberio_xport(tx_dev, rx_dev,
                    default_buff_args, int(divisor));
        }

        UHD_LOGGER_TRACE("MPMD")
            << "Making (muxed) stream with num " << xports.recv_sid.get_dst();
        xports.recv = _ctrl_dma_xport->make_stream(xports.recv_sid.get_dst());
    } else if (xport_type == ASYNC_MSG) {
        UHD_ASSERT_THROW(xport_info["muxed"] == "True");
        if (not _async_msg_dma_xport) {
            default_buff_args.send_frame_size = 256;
            default_buff_args.recv_frame_size = 256;
            _async_msg_dma_xport = make_muxed_liberio_xport(
                    tx_dev, rx_dev, default_buff_args, int(divisor));
        }

        UHD_LOGGER_TRACE("MPMD")
            << "making (muxed) stream with num " << xports.recv_sid.get_dst();
        xports.recv =
            _async_msg_dma_xport->make_stream(xports.recv_sid.get_dst());
    } else {
        xports.recv =
            transport::liberio_zero_copy::make(
                    tx_dev, rx_dev, default_buff_args);
    }

    transport::udp_zero_copy::buff_params buff_params;
    buff_params.recv_buff_size =
        float(default_buff_args.recv_frame_size) *
        float(default_buff_args.num_recv_frames) / divisor;
    buff_params.send_buff_size =
        float(default_buff_args.send_frame_size) *
        float(default_buff_args.num_send_frames) / divisor;


    // Communicate it all back to MPM
    if (not rpc->request_with_token<bool>("commit_xport", xport_info)) {
        UHD_LOG_ERROR("MPMD", "Failed to create UDP transport!");
        throw uhd::runtime_error("commit_xport() failed!");
    }

    // Finish both_xports_t object and return:
    xports.recv_buff_size = buff_params.recv_buff_size;
    xports.send_buff_size = buff_params.send_buff_size;
    xports.send = xports.recv;
    return xports;
}

#endif /* HAVE_LIBERIO */
