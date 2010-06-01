//
// Copyright 2010 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "../../transport/vrt_packet_handler.hpp"
#include "usrp2_impl.hpp"
#include "usrp2_regs.hpp"
#include <uhd/transport/convert_types.hpp>
#include <boost/format.hpp>
#include <boost/asio.hpp> //htonl and ntohl
#include <boost/bind.hpp>
#include <iostream>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;
namespace asio = boost::asio;

/***********************************************************************
 * Helper Functions
 **********************************************************************/
void usrp2_impl::io_init(void){
    //setup rx otw type
    _rx_otw_type.width = 16;
    _rx_otw_type.shift = 0;
    _rx_otw_type.byteorder = otw_type_t::BO_BIG_ENDIAN;

    //setup tx otw type
    _tx_otw_type.width = 16;
    _tx_otw_type.shift = 0;
    _tx_otw_type.byteorder = otw_type_t::BO_BIG_ENDIAN;

    //send a small data packet so the usrp2 knows the udp source port
    managed_send_buffer::sptr send_buff = _data_transport->get_send_buff();
    boost::uint32_t data = htonl(USRP2_INVALID_VRT_HEADER);
    memcpy(send_buff->cast<void*>(), &data, sizeof(data));
    send_buff->commit(sizeof(data));

    //setup RX DSP regs
    std::cout << "RX samples per packet: " << get_max_recv_samps_per_packet() << std::endl;
    _iface->poke32(FR_RX_CTRL_NSAMPS_PER_PKT, get_max_recv_samps_per_packet());
    _iface->poke32(FR_RX_CTRL_NCHANNELS, 1);
    _iface->poke32(FR_RX_CTRL_CLEAR_OVERRUN, 1); //reset
    _iface->poke32(FR_RX_CTRL_VRT_HEADER, 0
        | (0x1 << 28) //if data with stream id
        | (0x1 << 26) //has trailer
        | (0x3 << 22) //integer time other
        | (0x1 << 20) //fractional time sample count
    );
    _iface->poke32(FR_RX_CTRL_VRT_STREAM_ID, 0);
    _iface->poke32(FR_RX_CTRL_VRT_TRAILER, 0);
}

/***********************************************************************
 * Send Data
 **********************************************************************/
static inline managed_send_buffer::sptr get_send_buff(zero_copy_if::sptr zc_if){
    return zc_if->get_send_buff();
}

size_t usrp2_impl::send(
    const asio::const_buffer &buff,
    const tx_metadata_t &metadata,
    const io_type_t &io_type,
    send_mode_t send_mode
){
    return vrt_packet_handler::send(
        _packet_handler_send_state, //last state of the send handler
        buff, metadata, send_mode,  //buffer to empty and samples metadata
        io_type, _tx_otw_type,      //input and output types to convert
        get_master_clock_freq(),    //master clock tick rate
        boost::bind(get_send_buff, _data_transport),
        get_max_send_samps_per_packet()
    );
}

/***********************************************************************
 * Receive Data
 **********************************************************************/
static inline managed_recv_buffer::sptr get_recv_buff(zero_copy_if::sptr zc_if){
    return zc_if->get_recv_buff();
}

size_t usrp2_impl::recv(
    const asio::mutable_buffer &buff,
    rx_metadata_t &metadata,
    const io_type_t &io_type,
    recv_mode_t recv_mode
){
    return vrt_packet_handler::recv(
        _packet_handler_recv_state, //last state of the recv handler
        buff, metadata, recv_mode,  //buffer to fill and samples metadata
        io_type, _rx_otw_type,      //input and output types to convert
        get_master_clock_freq(),    //master clock tick rate
        boost::bind(get_recv_buff, _data_transport)
    );
}
