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

#include "usrp_e_impl.hpp"
#include "usrp_e_regs.hpp"
#include "../../transport/vrt_packet_handler.hpp"
#include <boost/bind.hpp>
#include <fcntl.h> //read, write
#include <linux/usrp_e.h> //transfer frame struct
#include <stddef.h> //offsetof
#include <boost/format.hpp>
#include <iostream>

using namespace uhd;

static const size_t MAX_BUFF_SIZE = 2048;

static const size_t vrt_header_offset_words32 = offsetof(usrp_transfer_frame, buf)/sizeof(boost::uint32_t);

/***********************************************************************
 * Data Transport (phony zero-copy with read/write)
 **********************************************************************/
class data_transport:
    public transport::phony_zero_copy_recv_if,
    public transport::phony_zero_copy_send_if
{
public:
    data_transport(int fd):
        transport::phony_zero_copy_recv_if(MAX_BUFF_SIZE),
        transport::phony_zero_copy_send_if(MAX_BUFF_SIZE),
        _fd(fd)
    {
        /* NOP */
    }

    size_t get_num_recv_frames(void) const{
        return 10; //FIXME no idea!
    }

    size_t get_num_send_frames(void) const{
        return 10; //FIXME no idea!
    }

private:
    int _fd;
    size_t send(const boost::asio::const_buffer &buff){
        //Set the frame length in the frame header.
        //This is technically bad to write to a const buffer,
        //but this will go away when the ring gets implemented,
        //and the send buffer commit method will set the length.
        const_cast<usrp_transfer_frame *>(
            boost::asio::buffer_cast<const usrp_transfer_frame *>(buff)
        )->len = boost::asio::buffer_size(buff);
        return write(
            _fd,
            boost::asio::buffer_cast<const void *>(buff),
            boost::asio::buffer_size(buff)
        );
    }
    size_t recv(const boost::asio::mutable_buffer &buff){
        //std::cout << boost::format(
        //    "calling read on fd %d, buff size is %d"
        //) % _fd % boost::asio::buffer_size(buff) << std::endl;
        ssize_t ret = read(
            _fd,
            boost::asio::buffer_cast<void *>(buff),
            boost::asio::buffer_size(buff)
        );
        if (ret < ssize_t(sizeof(usrp_transfer_frame))) return 0;
        //overwrite the vrt header length with the transfer frame length
        size_t frame_size = boost::asio::buffer_cast<usrp_transfer_frame *>(buff)->len;
        boost::uint32_t *vrt_header = boost::asio::buffer_cast<boost::uint32_t *>(buff) + vrt_header_offset_words32;
        vrt_header[0] = (vrt_header[0] & ~0xffff) | ((frame_size/sizeof(boost::uint32_t)) & 0xffff);

        //std::cout << "len " << int(boost::asio::buffer_cast<usrp_transfer_frame *>(buff)->len) << std::endl;
        //for (size_t i = 0; i < 7; i++){
        //    std::cout << boost::format("    0x%08x") % boost::asio::buffer_cast<boost::uint32_t *>(buff)[i] << std::endl;
        //}

        return frame_size;
    }
};

/***********************************************************************
 * IO Implementation Details
 **********************************************************************/
struct usrp_e_impl::io_impl{
    vrt_packet_handler::recv_state recv_state;
    vrt_packet_handler::send_state send_state;
    data_transport transport;
    io_impl(int fd): transport(fd){}
};

void usrp_e_impl::io_init(void){
    //setup rx data path
    _iface->poke32(UE_REG_CTRL_RX_NSAMPS_PER_PKT, get_max_recv_samps_per_packet());
    _iface->poke32(UE_REG_CTRL_RX_NCHANNELS, 1);
    _iface->poke32(UE_REG_CTRL_RX_CLEAR_OVERRUN, 1); //reset
    _iface->poke32(UE_REG_CTRL_RX_VRT_HEADER, 0
        | (0x1 << 28) //if data with stream id
        | (0x1 << 26) //has trailer
        | (0x3 << 22) //integer time other
        | (0x1 << 20) //fractional time sample count
    );
    _iface->poke32(UE_REG_CTRL_RX_VRT_STREAM_ID, 0);
    _iface->poke32(UE_REG_CTRL_RX_VRT_TRAILER, 0);

    _io_impl = UHD_PIMPL_MAKE(io_impl, (_iface->get_file_descriptor()));
}

static boost::uint32_t make_stream_cmd(bool now, bool chain, boost::uint32_t nsamps){
    return (((now)? 1 : 0) << 31) | (((chain)? 1 : 0) << 30) | nsamps;
}

void usrp_e_impl::issue_stream_cmd(const stream_cmd_t &stream_cmd){
    boost::uint32_t cmd = 0;
    switch(stream_cmd.stream_mode){
    case stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE:
        cmd = make_stream_cmd(stream_cmd.stream_now, false, stream_cmd.num_samps);
        break;

    case stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_MORE:
        cmd = make_stream_cmd(stream_cmd.stream_now, true, stream_cmd.num_samps);
        break;

    default: throw std::runtime_error("stream mode not implemented");
    }
    _iface->poke32(UE_REG_CTRL_RX_STREAM_CMD, cmd);
    _iface->poke32(UE_REG_CTRL_RX_TIME_SECS,  stream_cmd.time_spec.secs);
    _iface->poke32(UE_REG_CTRL_RX_TIME_TICKS, stream_cmd.time_spec.get_ticks(MASTER_CLOCK_RATE));
}

/***********************************************************************
 * Data Send
 **********************************************************************/
size_t usrp_e_impl::send(
    const boost::asio::const_buffer &buff,
    const uhd::tx_metadata_t &metadata,
    const io_type_t & io_type,
    send_mode_t send_mode
){
    otw_type_t send_otw_type;
    send_otw_type.width = 16;
    send_otw_type.shift = 0;
    send_otw_type.byteorder = otw_type_t::BO_LITTLE_ENDIAN;

    return vrt_packet_handler::send(
        _io_impl->send_state,
        buff,
        metadata,
        send_mode,
        io_type,
        send_otw_type, //TODO
        MASTER_CLOCK_RATE,
        uhd::transport::vrt::pack_le,
        boost::bind(&data_transport::get_send_buff, &_io_impl->transport),
        get_max_send_samps_per_packet(),
        vrt_header_offset_words32
    );
}

/***********************************************************************
 * Data Recv
 **********************************************************************/
size_t usrp_e_impl::recv(
    const boost::asio::mutable_buffer &buff,
    uhd::rx_metadata_t &metadata,
    const io_type_t &io_type,
    recv_mode_t recv_mode
){
    otw_type_t recv_otw_type;
    recv_otw_type.width = 16;
    recv_otw_type.shift = 0;
    recv_otw_type.byteorder = otw_type_t::BO_LITTLE_ENDIAN;

    return vrt_packet_handler::recv(
        _io_impl->recv_state,
        buff,
        metadata,
        recv_mode,
        io_type,
        recv_otw_type, //TODO
        MASTER_CLOCK_RATE,
        uhd::transport::vrt::unpack_le,
        boost::bind(&data_transport::get_recv_buff, &_io_impl->transport),
        vrt_header_offset_words32
    );
}
