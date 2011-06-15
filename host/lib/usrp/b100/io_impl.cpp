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

#include "../../transport/super_recv_packet_handler.hpp"
#include "../../transport/super_send_packet_handler.hpp"
#include "usrp_commands.h"
#include "b100_impl.hpp"
#include "b100_regs.hpp"
#include <uhd/usrp/dsp_utils.hpp>
#include <uhd/usrp/dsp_props.hpp>
#include <uhd/utils/thread_priority.hpp>
#include <uhd/transport/bounded_buffer.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/utils/log.hpp>
#include <iostream>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;
namespace asio = boost::asio;

/***********************************************************************
 * IO Implementation Details
 **********************************************************************/
struct b100_impl::io_impl{
    io_impl(zero_copy_if::sptr data_transport):
        data_transport(data_transport)
    {
        /* NOP */
    }

    ~io_impl(void){
        //drain the rx buffs
        //while(data_transport->get_recv_buff().get() != NULL){
                /* NOP */
        //}
    }

    zero_copy_if::sptr &data_transport;

    sph::recv_packet_handler recv_handler;
    sph::send_packet_handler send_handler;
    bool continuous_streaming;
};

/***********************************************************************
 * Initialize internals within this file
 **********************************************************************/
void b100_impl::io_init(void){
    _recv_otw_type.width = 16;
    _recv_otw_type.shift = 0;
    _recv_otw_type.byteorder = otw_type_t::BO_LITTLE_ENDIAN;

    _send_otw_type.width = 16;
    _send_otw_type.shift = 0;
    _send_otw_type.byteorder = otw_type_t::BO_LITTLE_ENDIAN;
    
    _iface->reset_gpif(6);

    //reset state machines
    _iface->poke32(B100_REG_CTRL_TX_CLEAR_UNDERRUN, 0);
    _iface->poke32(B100_REG_CTRL_RX_CLEAR_OVERRUN, 0);

    _io_impl = UHD_PIMPL_MAKE(io_impl, (_data_transport));
    
    //setup rx data path
    _iface->poke32(B100_REG_CTRL_RX_NSAMPS_PER_PKT, get_max_recv_samps_per_packet());
    UHD_LOGV(always) << "IO: Using " << get_max_recv_samps_per_packet() << " samples per packet" << std::endl;
    _iface->poke32(B100_REG_CTRL_RX_NCHANNELS, 1);
    _iface->poke32(B100_REG_CTRL_RX_VRT_HEADER, 0
        | (0x1 << 28) //if data with stream id
        | (0x1 << 26) //has trailer
        | (0x3 << 22) //integer time other
        | (0x1 << 20) //fractional time sample count
    );
    _iface->poke32(B100_REG_CTRL_RX_VRT_TRAILER, 0);

    //set the streamid to reset the seq num
    _iface->poke32(B100_REG_CTRL_TX_REPORT_SID, 0);
    //setup the tx policy
    _iface->poke32(B100_REG_CTRL_TX_POLICY, B100_FLAG_CTRL_TX_POLICY_NEXT_PACKET);
    
    //set the expected packet size in USB frames
    _iface->poke32(B100_REG_MISC_RX_LEN, 4);

    update_transport_channel_mapping();
}

void b100_impl::update_transport_channel_mapping(void){
    if (_io_impl.get() == NULL) return; //not inited yet

    //set all of the relevant properties on the handler
    boost::mutex::scoped_lock recv_lock = _io_impl->recv_handler.get_scoped_lock();
    _io_impl->recv_handler.resize(_rx_subdev_spec.size());
    _io_impl->recv_handler.set_vrt_unpacker(&vrt::if_hdr_unpack_le);
    _io_impl->recv_handler.set_tick_rate(_clock_ctrl->get_fpga_clock_rate());
    _io_impl->recv_handler.set_samp_rate(_rx_ddc_proxy->get_link()[DSP_PROP_HOST_RATE].as<double>());
    for (size_t chan = 0; chan < _io_impl->recv_handler.size(); chan++){
        _io_impl->recv_handler.set_xport_chan_get_buff(chan, boost::bind(
            &uhd::transport::zero_copy_if::get_recv_buff, _io_impl->data_transport, _1
        ));
        _io_impl->recv_handler.set_overflow_handler(chan, boost::bind(
            &b100_impl::handle_overrun, this, chan
        ));
    }
    _io_impl->recv_handler.set_converter(_recv_otw_type);

    //set all of the relevant properties on the handler
    boost::mutex::scoped_lock send_lock = _io_impl->send_handler.get_scoped_lock();
    _io_impl->send_handler.resize(_tx_subdev_spec.size());
    _io_impl->send_handler.set_vrt_packer(&vrt::if_hdr_pack_le);
    _io_impl->send_handler.set_tick_rate(_clock_ctrl->get_fpga_clock_rate());
    _io_impl->send_handler.set_samp_rate(_tx_duc_proxy->get_link()[DSP_PROP_HOST_RATE].as<double>());
    for (size_t chan = 0; chan < _io_impl->send_handler.size(); chan++){
        _io_impl->send_handler.set_xport_chan_get_buff(chan, boost::bind(
            &uhd::transport::zero_copy_if::get_send_buff, _io_impl->data_transport, _1
        ));
    }
    _io_impl->send_handler.set_converter(_send_otw_type);
    _io_impl->send_handler.set_max_samples_per_packet(get_max_send_samps_per_packet());
}

/***********************************************************************
 * Data send + helper functions
 **********************************************************************/
size_t b100_impl::get_max_send_samps_per_packet(void) const {
    static const size_t hdr_size = 0
        + vrt::max_if_hdr_words32*sizeof(boost::uint32_t)
        - sizeof(vrt::if_packet_info_t().cid) //no class id ever used
    ;
    static const size_t bpp = 2048 - hdr_size;
    return bpp / _send_otw_type.get_sample_size();
}

size_t b100_impl::send(
    const send_buffs_type &buffs, size_t nsamps_per_buff,
    const tx_metadata_t &metadata, const io_type_t &io_type,
    send_mode_t send_mode, double timeout
){    
    return _io_impl->send_handler.send(
        buffs, nsamps_per_buff,
        metadata, io_type,
        send_mode, timeout
    );
}

/***********************************************************************
 * Data recv + helper functions
 **********************************************************************/

size_t b100_impl::get_max_recv_samps_per_packet(void) const {
    static const size_t hdr_size = 0
        + vrt::max_if_hdr_words32*sizeof(boost::uint32_t)
        + sizeof(vrt::if_packet_info_t().tlr) //forced to have trailer
        - sizeof(vrt::if_packet_info_t().cid) //no class id ever used
    ;
    size_t bpp = 2048 - hdr_size; //limited by FPGA pkt buffer size
    return bpp/_recv_otw_type.get_sample_size();
}

size_t b100_impl::recv(
    const recv_buffs_type &buffs, size_t nsamps_per_buff,
    rx_metadata_t &metadata, const io_type_t &io_type,
    recv_mode_t recv_mode, double timeout
){
    return _io_impl->recv_handler.recv(
        buffs, nsamps_per_buff,
        metadata, io_type,
        recv_mode, timeout
    );
}

void b100_impl::issue_stream_cmd(const stream_cmd_t &stream_cmd)
{
    _io_impl->continuous_streaming = (stream_cmd.stream_mode == stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
    _iface->poke32(B100_REG_CTRL_RX_STREAM_CMD, dsp_type1::calc_stream_cmd_word(stream_cmd));
    _iface->poke32(B100_REG_CTRL_RX_TIME_SECS,  boost::uint32_t(stream_cmd.time_spec.get_full_secs()));
    _iface->poke32(B100_REG_CTRL_RX_TIME_TICKS, stream_cmd.time_spec.get_tick_count(_clock_ctrl->get_fpga_clock_rate()));
    
    if (stream_cmd.stream_mode == stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS) {
        while(_io_impl->data_transport->get_recv_buff().get() != NULL){
            /* NOP */
        }
    }
}

void b100_impl::handle_overrun(size_t){
    if (_io_impl->continuous_streaming){
        this->issue_stream_cmd(stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
    }
}
