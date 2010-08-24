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
#include <uhd/usrp/dsp_utils.hpp>
#include "../../transport/vrt_packet_handler.hpp"
#include <boost/bind.hpp>
#include <fcntl.h> //read, write
#include <poll.h>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <iostream>

using namespace uhd;
using namespace uhd::usrp;

/***********************************************************************
 * Constants
 **********************************************************************/
static const size_t MAX_BUFF_SIZE = 2048;
static const bool usrp_e_io_impl_verbose = false;

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

    size_t recv_timeout_ms;

private:
    int _fd;
    ssize_t send(const boost::asio::const_buffer &buff){
        return write(_fd,
            boost::asio::buffer_cast<const void *>(buff),
            boost::asio::buffer_size(buff)
        );
    }
    ssize_t recv(const boost::asio::mutable_buffer &buff){
        //std::cout << boost::format(
        //    "calling read on fd %d, buff size is %d"
        //) % _fd % boost::asio::buffer_size(buff) << std::endl;

        //setup and call poll on the file descriptor
        //return 0 and do not read when poll times out
        pollfd pfd;
        pfd.fd = _fd;
        pfd.events = POLLIN;
        ssize_t poll_ret = poll(&pfd, 1, recv_timeout_ms);
        if (poll_ret <= 0){
            if (usrp_e_io_impl_verbose) std::cerr << boost::format(
                "usrp-e io impl recv(): poll() returned non-positive value: %d\n"
                "    -> return 0 for timeout"
            ) % poll_ret << std::endl;
            return 0; //timeout
        }

        //perform the blocking read(...)
        ssize_t read_ret = read(_fd,
            boost::asio::buffer_cast<void *>(buff),
            boost::asio::buffer_size(buff)
        );
        if (read_ret < 0){
            if (usrp_e_io_impl_verbose) std::cerr << boost::format(
                "usrp-e io impl recv(): read() returned small value: %d\n"
                "    -> return -1 for error"
            ) % read_ret << std::endl;
            return -1;
        }

        //std::cout << "len " << int(read_ret) << std::endl;
        //for (size_t i = 0; i < 9; i++){
        //    std::cout << boost::format("    0x%08x") % boost::asio::buffer_cast<boost::uint32_t *>(buff)[i] << std::endl;
        //}

        return read_ret;
    }
};

/***********************************************************************
 * IO Implementation Details
 **********************************************************************/
struct usrp_e_impl::io_impl{
    //state management for the vrt packet handler code
    vrt_packet_handler::recv_state packet_handler_recv_state;
    vrt_packet_handler::send_state packet_handler_send_state;
    data_transport transport;
    bool continuous_streaming;
    io_impl(int fd): packet_handler_recv_state(1), transport(fd){}
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

void usrp_e_impl::issue_stream_cmd(const stream_cmd_t &stream_cmd){
    _io_impl->continuous_streaming = (stream_cmd.stream_mode == stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
    _iface->poke32(UE_REG_CTRL_RX_STREAM_CMD, dsp_type1::calc_stream_cmd_word(
        stream_cmd, get_max_recv_samps_per_packet()
    ));
    _iface->poke32(UE_REG_CTRL_RX_TIME_SECS,  boost::uint32_t(stream_cmd.time_spec.get_full_secs()));
    _iface->poke32(UE_REG_CTRL_RX_TIME_TICKS, stream_cmd.time_spec.get_tick_count(MASTER_CLOCK_RATE));
}

void usrp_e_impl::handle_overrun(size_t){
    std::cerr << "O"; //the famous OOOOOOOOOOO
    _iface->poke32(UE_REG_CTRL_RX_CLEAR_OVERRUN, 0);
    if (_io_impl->continuous_streaming){
        this->issue_stream_cmd(stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
    }
}

/***********************************************************************
 * Data Send
 **********************************************************************/
bool get_send_buffs(
    data_transport *trans,
    vrt_packet_handler::managed_send_buffs_t &buffs
){
    UHD_ASSERT_THROW(buffs.size() == 1);
    buffs[0] = trans->get_send_buff();
    return buffs[0].get();
}

size_t usrp_e_impl::send(
    const std::vector<const void *> &buffs,
    size_t num_samps,
    const tx_metadata_t &metadata,
    const io_type_t &io_type,
    send_mode_t send_mode
){
    otw_type_t send_otw_type;
    send_otw_type.width = 16;
    send_otw_type.shift = 0;
    send_otw_type.byteorder = otw_type_t::BO_LITTLE_ENDIAN;

    return vrt_packet_handler::send(
        _io_impl->packet_handler_send_state,       //last state of the send handler
        buffs, num_samps,                          //buffer to fill
        metadata, send_mode,                       //samples metadata
        io_type, send_otw_type,                    //input and output types to convert
        MASTER_CLOCK_RATE,                         //master clock tick rate
        uhd::transport::vrt::if_hdr_pack_le,
        boost::bind(&get_send_buffs, &_io_impl->transport, _1),
        get_max_send_samps_per_packet()
    );
}

/***********************************************************************
 * Data Recv
 **********************************************************************/
bool get_recv_buffs(
    data_transport *trans,
    vrt_packet_handler::managed_recv_buffs_t &buffs
){
    UHD_ASSERT_THROW(buffs.size() == 1);
    buffs[0] = trans->get_recv_buff();
    return buffs[0].get();
}

size_t usrp_e_impl::recv(
    const std::vector<void *> &buffs,
    size_t num_samps,
    rx_metadata_t &metadata,
    const io_type_t &io_type,
    recv_mode_t recv_mode,
    size_t timeout_ms
){
    otw_type_t recv_otw_type;
    recv_otw_type.width = 16;
    recv_otw_type.shift = 0;
    recv_otw_type.byteorder = otw_type_t::BO_LITTLE_ENDIAN;

    //hand-off the timeout to the transport
    _io_impl->transport.recv_timeout_ms = timeout_ms;

    return vrt_packet_handler::recv(
        _io_impl->packet_handler_recv_state,       //last state of the recv handler
        buffs, num_samps,                          //buffer to fill
        metadata, recv_mode,                       //samples metadata
        io_type, recv_otw_type,                    //input and output types to convert
        MASTER_CLOCK_RATE,                         //master clock tick rate
        uhd::transport::vrt::if_hdr_unpack_le,
        boost::bind(&get_recv_buffs, &_io_impl->transport, _1),
        boost::bind(&usrp_e_impl::handle_overrun, this, _1)
    );
}

/***********************************************************************
 * Dummy Async Recv
 **********************************************************************/
bool usrp_e_impl::recv_async_msg(async_metadata_t &, size_t timeout_ms){
    boost::this_thread::sleep(boost::posix_time::milliseconds(timeout_ms));
    return false;
}
