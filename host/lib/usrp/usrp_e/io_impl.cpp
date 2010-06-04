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
#include "../../transport/vrt_packet_handler.hpp"
#include <boost/bind.hpp>
#include <fcntl.h> //read, write
#include <linux/usrp_e.h> //transfer frame struct
#include <stddef.h> //offsetof

using namespace uhd;

static const size_t MAX_BUFF_SIZE = 2048;

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
        return read(
            _fd,
            boost::asio::buffer_cast<void *>(buff),
            boost::asio::buffer_size(buff)
        );
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
    _io_impl = UHD_PIMPL_MAKE(io_impl, (_iface->get_file_descriptor()));
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
    send_otw_type.byteorder = otw_type_t::BO_NATIVE;

    return vrt_packet_handler::send(
        _io_impl->send_state,
        buff,
        metadata,
        send_mode,
        io_type,
        send_otw_type, //TODO
        64e6, //TODO
        boost::bind(&data_transport::get_send_buff, &_io_impl->transport),
        (MAX_BUFF_SIZE - sizeof(usrp_transfer_frame))/send_otw_type.get_sample_size(),
        offsetof(usrp_transfer_frame, buf)
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
    recv_otw_type.byteorder = otw_type_t::BO_NATIVE;

    return vrt_packet_handler::recv(
        _io_impl->recv_state,
        buff,
        metadata,
        recv_mode,
        io_type,
        recv_otw_type, //TODO
        64e6, //TODO
        boost::bind(&data_transport::get_recv_buff, &_io_impl->transport),
        offsetof(usrp_transfer_frame, buf)
    );
}
