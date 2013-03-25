//
// Copyright 2010-2012 Ettus Research LLC
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

#include "e100_ctrl.hpp"
#include <uhd/transport/zero_copy.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/exception.hpp>
#include <boost/make_shared.hpp>
#include <linux/usrp_e.h>
#include <sys/mman.h> //mmap
#include <unistd.h> //getpagesize
#include <poll.h> //poll
#include <vector>

using namespace uhd;
using namespace uhd::transport;

#define fp_verbose false //fast-path verbose
static const size_t poll_breakout = 10; //how many poll timeouts constitute a full timeout

/***********************************************************************
 * Reusable managed receiver buffer:
 *  - The buffer knows how to claim and release a frame.
 **********************************************************************/
class e100_mmap_zero_copy_mrb : public managed_recv_buffer{
public:
    e100_mmap_zero_copy_mrb(void *mem, ring_buffer_info *info):
        _mem(mem), _info(info) { /* NOP */ }

    void release(void){
        if (fp_verbose) UHD_LOGV(always) << "recv buff: release" << std::endl;
        _info->flags = RB_KERNEL; //release the frame
    }

    UHD_INLINE bool ready(void){return _info->flags & RB_USER;}

    UHD_INLINE sptr get_new(void){
        if (fp_verbose) UHD_LOGV(always) << "  make_recv_buff: " << _info->len << std::endl;
        _info->flags = RB_USER_PROCESS; //claim the frame
        return make(this, _mem, _info->len);
    }

private:
    void *_mem;
    ring_buffer_info *_info;
};

/***********************************************************************
 * Reusable managed send buffer:
 *  - The buffer knows how to claim and release a frame.
 **********************************************************************/
class e100_mmap_zero_copy_msb : public managed_send_buffer{
public:
    e100_mmap_zero_copy_msb(void *mem, ring_buffer_info *info, size_t len, int fd):
        _mem(mem), _info(info), _len(len), _fd(fd) { /* NOP */ }

    void release(void){
        if (fp_verbose) UHD_LOGV(always) << "send buff: commit " << size() << std::endl;
        _info->len = _len;//size();
        _info->flags = RB_USER; //release the frame
        if (::write(_fd, NULL, 0) < 0){ //notifies the kernel
            UHD_LOGV(rarely) << UHD_THROW_SITE_INFO("write error") << std::endl;
        }
    }

    UHD_INLINE bool ready(void){return _info->flags & RB_KERNEL;}

    UHD_INLINE sptr get_new(void){
        if (fp_verbose) UHD_LOGV(always) << "  make_send_buff: " << _len << std::endl;
        _info->flags = RB_USER_PROCESS; //claim the frame
        return make(this, _mem, _len);
    }

private:
    void *_mem;
    ring_buffer_info *_info;
    size_t _len;
    int _fd;
};

/***********************************************************************
 * The zero copy interface implementation
 **********************************************************************/
class e100_mmap_zero_copy_impl : public zero_copy_if{
public:
    e100_mmap_zero_copy_impl(e100_ctrl::sptr iface):
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
        UHD_LOG
            << "page_size:          " << page_size                   << std::endl
            << "frame_size:         " << _frame_size                 << std::endl
            << "num_pages_rx_flags: " << _rb_size.num_pages_rx_flags << std::endl
            << "num_rx_frames:      " << _rb_size.num_rx_frames      << std::endl
            << "num_pages_tx_flags: " << _rb_size.num_pages_tx_flags << std::endl
            << "num_tx_frames:      " << _rb_size.num_tx_frames      << std::endl
            << "map_size:           " << _map_size                   << std::endl
        ;

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
        UHD_LOG
            << "recv_info_off: " << recv_info_off << std::endl
            << "recv_buff_off: " << recv_buff_off << std::endl
            << "send_info_off: " << send_info_off << std::endl
            << "send_buff_off: " << send_buff_off << std::endl
        ;

        //pointers to sections in the mapped memory
        ring_buffer_info (*recv_info)[], (*send_info)[];
        char *recv_buff, *send_buff;

        //set the internal pointers for info and buffers
        typedef ring_buffer_info (*rbi_pta)[];
        char *rb_ptr = reinterpret_cast<char *>(_mapped_mem);
        recv_info = reinterpret_cast<rbi_pta>(rb_ptr + recv_info_off);
        recv_buff = rb_ptr + recv_buff_off;
        send_info = reinterpret_cast<rbi_pta>(rb_ptr + send_info_off);
        send_buff = rb_ptr + send_buff_off;

        //initialize the managed receive buffers
        for (size_t i = 0; i < get_num_recv_frames(); i++){
            _mrb_pool.push_back(boost::make_shared<e100_mmap_zero_copy_mrb>(
                recv_buff + get_recv_frame_size()*i, (*recv_info) + i
            ));
        }

        //initialize the managed send buffers
        for (size_t i = 0; i < get_num_recv_frames(); i++){
            _msb_pool.push_back(boost::make_shared<e100_mmap_zero_copy_msb>(
                send_buff + get_send_frame_size()*i, (*send_info) + i,
                get_send_frame_size(), _fd
            ));
        }
    }

    ~e100_mmap_zero_copy_impl(void){
        UHD_LOG << "cleanup: munmap" << std::endl;
        ::munmap(_mapped_mem, _map_size);
    }

    managed_recv_buffer::sptr get_recv_buff(double timeout){
        if (fp_verbose) UHD_LOGV(always) << "get_recv_buff: " << _recv_index << std::endl;
        e100_mmap_zero_copy_mrb &mrb = *_mrb_pool[_recv_index];

        //poll/wait for a ready frame
        if (not mrb.ready()){
            for (size_t i = 0; i < poll_breakout; i++){
                pollfd pfd;
                pfd.fd = _fd;
                pfd.events = POLLIN;
                ssize_t poll_ret = ::poll(&pfd, 1, size_t(timeout*1e3/poll_breakout));
                if (fp_verbose) UHD_LOGV(always) << "  POLLIN: " << poll_ret << std::endl;
                if (poll_ret > 0) goto found_user_frame; //good poll, continue on
            }
            return managed_recv_buffer::sptr(); //timed-out for real
        } found_user_frame:

        //increment the index for the next call
        if (++_recv_index == get_num_recv_frames()) _recv_index = 0;

        //return the managed buffer for this frame
        return mrb.get_new();
    }

    size_t get_num_recv_frames(void) const{
        return _rb_size.num_rx_frames;
    }

    size_t get_recv_frame_size(void) const{
        return _frame_size;
    }

    managed_send_buffer::sptr get_send_buff(double timeout){
        if (fp_verbose) UHD_LOGV(always) << "get_send_buff: " << _send_index << std::endl;
        e100_mmap_zero_copy_msb &msb = *_msb_pool[_send_index];

        //poll/wait for a ready frame
        if (not msb.ready()){
            pollfd pfd;
            pfd.fd = _fd;
            pfd.events = POLLOUT;
            ssize_t poll_ret = ::poll(&pfd, 1, size_t(timeout*1e3));
            if (fp_verbose) UHD_LOGV(always) << "  POLLOUT: " << poll_ret << std::endl;
            if (poll_ret <= 0) return managed_send_buffer::sptr();
        }

        //increment the index for the next call
        if (++_send_index == get_num_send_frames()) _send_index = 0;

        //return the managed buffer for this frame
        return msb.get_new();
    }

    size_t get_num_send_frames(void) const{
        return _rb_size.num_tx_frames;
    }

    size_t get_send_frame_size(void) const{
        return _frame_size;
    }

private:
    //file descriptor for mmap
    int _fd;

    //the mapped memory itself
    void *_mapped_mem;

    //mapped memory sizes
    usrp_e_ring_buffer_size_t _rb_size;
    size_t _frame_size, _map_size;

    //re-usable managed buffers
    std::vector<boost::shared_ptr<e100_mmap_zero_copy_mrb> > _mrb_pool;
    std::vector<boost::shared_ptr<e100_mmap_zero_copy_msb> > _msb_pool;

    //indexes into sub-sections of mapped memory
    size_t _recv_index, _send_index;
};

/***********************************************************************
 * The zero copy interface make function
 **********************************************************************/
zero_copy_if::sptr e100_make_mmap_zero_copy(e100_ctrl::sptr iface){
    return zero_copy_if::sptr(new e100_mmap_zero_copy_impl(iface));
}
