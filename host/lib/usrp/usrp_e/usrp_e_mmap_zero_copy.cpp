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

#include <uhd/transport/zero_copy.hpp>
#include <uhd/utils/assert.hpp>
#include <linux/usrp_e.h>
#include <sys/mman.h> //mmap
#include <unistd.h> //getpagesize
#include <poll.h> //poll
#include <boost/cstdint.hpp>
#include "usrp_e_iface.hpp"

using namespace uhd;
using namespace uhd::transport;

static const bool debug_verbose = false;

/***********************************************************************
 * The managed receive buffer implementation
 **********************************************************************/
class usrp_e_mmap_managed_recv_buffer : public managed_recv_buffer{
public:
    usrp_e_mmap_managed_recv_buffer(
        const void *mem, size_t len, ring_buffer_info *info
    ):
        _buff(mem, len), _info(info)
    {
        /* NOP */
    }

    ~usrp_e_mmap_managed_recv_buffer(void){
        _info->flags = RB_KERNEL;
    }

private:
    const boost::asio::const_buffer &get(void) const{
        return _buff;
    }

    const boost::asio::const_buffer _buff;
    ring_buffer_info *_info;
};

/***********************************************************************
 * The managed send buffer implementation
 **********************************************************************/
class usrp_e_mmap_managed_send_buffer : public managed_send_buffer{
public:
    usrp_e_mmap_managed_send_buffer(
        void *mem, size_t len, ring_buffer_info *info, int fd
    ):
        _buff(mem, len), _info(info), _fd(fd), _commited(false)
    {
        /* NOP */
    }

    ~usrp_e_mmap_managed_send_buffer(void){
        if (not _commited) this->commit(0);
    }

    ssize_t commit(size_t num_bytes){
        _commited = true;
        _info->len = num_bytes;
        _info->flags = RB_USER;
        ssize_t ret = ::write(_fd, NULL, 0);
        return (ret < 0)? ret : num_bytes;
    }

private:
    const boost::asio::mutable_buffer &get(void) const{
        return _buff;
    }

    const boost::asio::mutable_buffer _buff;
    ring_buffer_info *_info;
    int _fd;
    bool _commited;
};

/***********************************************************************
 * The zero copy interface implementation
 **********************************************************************/
class usrp_e_mmap_zero_copy_impl : public zero_copy_if{
public:
    usrp_e_mmap_zero_copy_impl(usrp_e_iface::sptr iface):
        _fd(iface->get_file_descriptor()), _recv_index(0), _send_index(0)
    {
        //get system sizes
        iface->ioctl(USRP_E_GET_RB_INFO, &_rb_size);
        size_t page_size = getpagesize();
        _frame_size = page_size/2;

        //calculate the memory size
        size_t map_size =
            (_rb_size.num_pages_rx_flags + _rb_size.num_pages_tx_flags) * page_size +
            (_rb_size.num_rx_frames + _rb_size.num_tx_frames) * _frame_size;

        //call mmap to get the memory
        void *ring_buffer = mmap(
            NULL, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, _fd, 0
        );
        UHD_ASSERT_THROW(ring_buffer != MAP_FAILED);

        //calculate the memory offsets for info and buffers
        size_t recv_info_off = 0;
        size_t recv_buff_off = recv_info_off + (_rb_size.num_pages_rx_flags * page_size);
        size_t send_info_off = recv_buff_off + (_rb_size.num_rx_frames * _frame_size);
        size_t send_buff_off = send_info_off + (_rb_size.num_pages_tx_flags * page_size);

        //set the internal pointers for info and buffers
        typedef ring_buffer_info (*rbi_pta)[];
        boost::uint8_t *rb_ptr = reinterpret_cast<boost::uint8_t *>(ring_buffer);
        _recv_info = reinterpret_cast<rbi_pta>(rb_ptr + recv_info_off);
        _recv_buff = rb_ptr + recv_buff_off;
        _send_info = reinterpret_cast<rbi_pta>(rb_ptr + send_info_off);
        _send_buff = rb_ptr + send_buff_off;
    }

    managed_recv_buffer::sptr get_recv_buff(size_t timeout_ms){
        //grab pointers to the info and buffer
        ring_buffer_info *info = (*_recv_info) + _recv_index;
        void *mem = _recv_buff + _frame_size*_recv_index;

        //poll/wait for a ready frame
        if (not (info->flags & RB_USER)){
            pollfd pfd;
            pfd.fd = _fd;
            pfd.events = POLLIN;
            ssize_t poll_ret = poll(&pfd, 1, timeout_ms);
            if (poll_ret <= 0) return managed_recv_buffer::sptr();
        }

        //increment the index for the next call
        if (++_recv_index == size_t(_rb_size.num_rx_frames)) _recv_index = 0;

        //return the managed buffer for this frame
        return managed_recv_buffer::sptr(
            new usrp_e_mmap_managed_recv_buffer(mem, info->len, info)
        );
    }

    size_t get_num_recv_frames(void) const{
        return _rb_size.num_rx_frames;
    }

    managed_send_buffer::sptr get_send_buff(void){
        //grab pointers to the info and buffer
        ring_buffer_info *info = (*_send_info) + _send_index;
        void *mem = _send_buff + _frame_size*_send_index;

        //poll/wait for a ready frame
        if (not (info->flags & RB_KERNEL)){
            pollfd pfd;
            pfd.fd = _fd;
            pfd.events = POLLOUT;
            ssize_t poll_ret = poll(&pfd, 1, -1 /*forever*/);
            if (poll_ret <= 0) return managed_send_buffer::sptr();
        }

        //increment the index for the next call
        if (++_send_index == size_t(_rb_size.num_tx_frames)) _send_index = 0;

        //return the managed buffer for this frame
        return managed_send_buffer::sptr(
            new usrp_e_mmap_managed_send_buffer(mem, _frame_size, info, _fd)
        );
    }

    size_t get_num_send_frames(void) const{
        return _rb_size.num_tx_frames;
    }

private:
    int _fd;
    usrp_e_ring_buffer_size_t _rb_size;
    size_t _frame_size;
    ring_buffer_info (*_recv_info)[], (*_send_info)[];
    boost::uint8_t *_recv_buff, *_send_buff;
    size_t _recv_index, _send_index;
};

/***********************************************************************
 * The zero copy interface make function
 **********************************************************************/
zero_copy_if::sptr usrp_e_make_mmap_zero_copy(usrp_e_iface::sptr iface){
    return zero_copy_if::sptr(new usrp_e_mmap_zero_copy_impl(iface));
}
