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

#ifndef INCLUDED_UHD_DEVICE_IPP
#define INCLUDED_UHD_DEVICE_IPP

namespace uhd{

    UHD_INLINE size_t device::send(
        const void *buff,
        size_t nsamps_per_buff,
        const tx_metadata_t &metadata,
        const io_type_t &io_type,
        send_mode_t send_mode
    ){
        return this->send(
            std::vector<const void *>(1, buff),
            nsamps_per_buff, metadata,
            io_type, send_mode
        );
    }

    UHD_DEPRECATED UHD_INLINE size_t device::send(
        const boost::asio::const_buffer &buff,
        const tx_metadata_t &metadata,
        const io_type_t &io_type,
        send_mode_t send_mode
    ){
        return this->send(
            boost::asio::buffer_cast<const void *>(buff),
            boost::asio::buffer_size(buff)/io_type.size,
            metadata, io_type, send_mode
        );
    }

    UHD_INLINE size_t device::recv(
        void *buff,
        size_t nsamps_per_buff,
        rx_metadata_t &metadata,
        const io_type_t &io_type,
        recv_mode_t recv_mode,
        size_t timeout_ms
    ){
        return this->recv(
            std::vector<void *>(1, buff),
            nsamps_per_buff, metadata,
            io_type, recv_mode, timeout_ms
        );
    }

    UHD_DEPRECATED UHD_INLINE size_t device::recv(
        const boost::asio::mutable_buffer &buff,
        rx_metadata_t &metadata,
        const io_type_t &io_type,
        recv_mode_t recv_mode
    ){
        return this->recv(
            boost::asio::buffer_cast<void *>(buff),
            boost::asio::buffer_size(buff)/io_type.size,
            metadata, io_type, recv_mode
        );
    }

} //namespace uhd

#endif /* INCLUDED_UHD_DEVICE_IPP */
