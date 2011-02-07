//
// Copyright 2010-2011 Ettus Research LLC
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

#include "usrp_e100_iface.hpp"
#include <uhd/transport/zero_copy.hpp>
#include <uhd/utils/assert.hpp>
#include <linux/usrp_e.h>
#include <sys/mman.h> //mmap
#include <unistd.h> //getpagesize
#include <poll.h> //poll
#include <boost/bind.hpp>
#include <iostream>

using namespace uhd;
using namespace uhd::transport;

static const bool fp_verbose = false; //fast-path verbose
static const bool sp_verbose = false; //slow-path verbose
static const size_t poll_breakout = 10; //how many poll timeouts constitute a full timeout

/***********************************************************************
 * The zero copy interface implementation
 **********************************************************************/
class usrp_e100_mmap_zero_copy_impl : public zero_copy_if{
public:
    usrp_e100_mmap_zero_copy_impl(usrp_e100_iface::sptr iface):
        _fd(iface->get_file_descriptor()), _recv_index(0), _send_index(0)
    {
        //get system sizes
        iface->ioctl(USRP_E_GET_RB_INFO, &_rb_size);
        size_t page_size = getpagesize();
        _frame_size = page_size/2;

        //calculate the memory size
        _map_size =
            (_rb_size.num_pages_rx_flags + _rb_size.num_pages_tx_flags) * page_size +
            (_rb_size.num_rx_frames + _rb_size.num_tx_frames) * _frame_size;

        //print sizes summary
        if (sp_verbose){
            std::cout << "page_size:          " << page_size                   << std::endl;
            std::cout << "frame_size:         " << _frame_size                 << std::endl;
            std::cout << "num_pages_rx_flags: " << _rb_size.num_pages_rx_flags << std::endl;
            std::cout << "num_rx_frames:      " << _rb_size.num_rx_frames      << std::endl;
            std::cout << "num_pages_tx_flags: " << _rb_size.num_pages_tx_flags << std::endl;
            std::cout << "num_tx_frames:      " << _rb_size.num_tx_frames      << std::endl;
            std::cout << "map_size:           " << _map_size                   << std::endl;
        }

        //call mmap to get the memory
        _mapped_mem = ::mmap(
            NULL, _map_size, PROT_READ | PROT_WRITE, MAP_SHARED, _fd, 0
        );
        UHD_ASSERT_THROW(_mapped_mem != MAP_FAILED);

        //calculate the memory offsets for info and buffers
        size_t recv_info_off = 0;
        size_t recv_buff_off = recv_info_off + (_rb_size.num_pages_rx_flags * page_size);
        size_t send_info_off = recv_buff_off + (_rb_size.num_rx_frames * _frame_size);
        size_t send_buff_off = send_info_off + (_rb_size.num_pages_tx_flags * page_size);

        //print offset summary
        if (sp_verbose){
            std::cout << "recv_info_off: " << recv_info_off << std::endl;
            std::cout << "recv_buff_off: " << recv_buff_off << std::endl;
            std::cout << "send_info_off: " << send_info_off << std::endl;
            std::cout << "send_buff_off: " << send_buff_off << std::endl;
        }

        //set the internal pointers for info and buffers
        typedef ring_buffer_info (*rbi_pta)[];
        char *rb_ptr = reinterpret_cast<char *>(_mapped_mem);
        _recv_info = reinterpret_cast<rbi_pta>(rb_ptr + recv_info_off);
        _recv_buff = rb_ptr + recv_buff_off;
        _send_info = reinterpret_cast<rbi_pta>(rb_ptr + send_info_off);
        _send_buff = rb_ptr + send_buff_off;
    }

    ~usrp_e100_mmap_zero_copy_impl(void){
        if (sp_verbose) std::cout << "cleanup: munmap" << std::endl;
        ::munmap(_mapped_mem, _map_size);
    }

    managed_recv_buffer::sptr get_recv_buff(double timeout){
        if (fp_verbose) std::cout << "get_recv_buff: " << _recv_index << std::endl;

        //grab pointers to the info and buffer
        ring_buffer_info *info = (*_recv_info) + _recv_index;
        void *mem = _recv_buff + _frame_size*_recv_index;

        //poll/wait for a ready frame
        if (not (info->flags & RB_USER)){
            for (size_t i = 0; i < poll_breakout; i++){
                pollfd pfd;
                pfd.fd = _fd;
                pfd.events = POLLIN;
                ssize_t poll_ret = ::poll(&pfd, 1, size_t(timeout*1e3/poll_breakout));
                if (fp_verbose) std::cout << "  POLLIN: " << poll_ret << std::endl;
                if (poll_ret > 0) goto found_user_frame; //good poll, continue on
            }
            return managed_recv_buffer::sptr(); //timed-out for real
        } found_user_frame:

        //the process has claimed the frame
        info->flags = RB_USER_PROCESS;

        //increment the index for the next call
        if (++_recv_index == size_t(_rb_size.num_rx_frames)) _recv_index = 0;

        //return the managed buffer for this frame
        if (fp_verbose) std::cout << "  make_recv_buff: " << info->len << std::endl;
        return managed_recv_buffer::make_safe(
            mem, info->len,
            boost::bind(&usrp_e100_mmap_zero_copy_impl::release, this, info)
        );
    }

    size_t get_num_recv_frames(void) const{
        return _rb_size.num_rx_frames;
    }

    size_t get_recv_frame_size(void) const{
        return _frame_size;
    }

    managed_send_buffer::sptr get_send_buff(double timeout){
        if (fp_verbose) std::cout << "get_send_buff: " << _send_index << std::endl;

        //grab pointers to the info and buffer
        ring_buffer_info *info = (*_send_info) + _send_index;
        void *mem = _send_buff + _frame_size*_send_index;

        //poll/wait for a ready frame
        if (not (info->flags & RB_KERNEL)){
            pollfd pfd;
            pfd.fd = _fd;
            pfd.events = POLLOUT;
            ssize_t poll_ret = ::poll(&pfd, 1, size_t(timeout*1e3));
            if (fp_verbose) std::cout << "  POLLOUT: " << poll_ret << std::endl;
            if (poll_ret <= 0) return managed_send_buffer::sptr();
        }

        //increment the index for the next call
        if (++_send_index == size_t(_rb_size.num_tx_frames)) _send_index = 0;

        //return the managed buffer for this frame
        if (fp_verbose) std::cout << "  make_send_buff: " << _frame_size << std::endl;
        return managed_send_buffer::make_safe(
            mem, _frame_size,
            boost::bind(&usrp_e100_mmap_zero_copy_impl::commit, this, info, _1)
        );
    }

    size_t get_num_send_frames(void) const{
        return _rb_size.num_tx_frames;
    }

    size_t get_send_frame_size(void) const{
        return _frame_size;
    }

private:

    void release(ring_buffer_info *info){
        if (fp_verbose) std::cout << "recv buff: release" << std::endl;
        info->flags = RB_KERNEL;
    }

    void commit(ring_buffer_info *info, size_t len){
        if (fp_verbose) std::cout << "send buff: commit " << len << std::endl;
        info->len = len;
        info->flags = RB_USER;
        if (::write(_fd, NULL, 0) < 0){
            std::cerr << UHD_THROW_SITE_INFO("write error") << std::endl;
        }
    }

    int _fd;

    //the mapped memory itself
    void *_mapped_mem;

    //mapped memory sizes
    usrp_e_ring_buffer_size_t _rb_size;
    size_t _frame_size, _map_size;

    //pointers to sections in the mapped memory
    ring_buffer_info (*_recv_info)[], (*_send_info)[];
    char *_recv_buff, *_send_buff;

    //indexes into sub-sections of mapped memory
    size_t _recv_index, _send_index;
};

/***********************************************************************
 * The zero copy interface make function
 **********************************************************************/
zero_copy_if::sptr usrp_e100_make_mmap_zero_copy(usrp_e100_iface::sptr iface){
    return zero_copy_if::sptr(new usrp_e100_mmap_zero_copy_impl(iface));
}
