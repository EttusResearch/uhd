//
// Copyright 2011-2012 Ettus Research LLC
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

#include <uhd/transport/usb_zero_copy.hpp>
#include <uhd/transport/buffer_pool.hpp>
#include <uhd/utils/byteswap.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/utils/tasks.hpp>
#include <uhd/utils/atomic.hpp>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/bind.hpp>
#include <vector>
#include <iostream>

using namespace uhd;
using namespace uhd::transport;

static const boost::posix_time::time_duration AUTOFLUSH_TIMEOUT(boost::posix_time::milliseconds(1));

/***********************************************************************
 * USB zero copy wrapper - managed receive buffer
 **********************************************************************/
class usb_zero_copy_wrapper_mrb : public managed_recv_buffer{
public:
    usb_zero_copy_wrapper_mrb(void){/*NOP*/}

    void release(void){
        _mrb.reset(); //decrement ref count, other MRB's may hold a ref
        _claimer.release();
    }

    UHD_INLINE sptr get_new(
        managed_recv_buffer::sptr &mrb, size_t &offset_bytes,
        const double timeout, size_t &index
    ){
        if (not mrb or not _claimer.claim_with_wait(timeout)) return sptr();

        index++; //advances the caller's buffer

        //hold a copy of the buffer shared pointer
        _mrb = mrb;

        //extract this packet's memory address and length in bytes
        char *mem = mrb->cast<char *>() + offset_bytes;
        const boost::uint32_t *mem32 = reinterpret_cast<const boost::uint32_t *>(mem);
        const size_t words32 = (uhd::wtohx(mem32[0]) & 0xffff); //length in words32 (from VRT header)
        const size_t len = words32*sizeof(boost::uint32_t); //length in bytes

        //check if this receive buffer has been exhausted
        offset_bytes += len;
        if (offset_bytes >= mrb->size()) mrb.reset(); //drop caller's ref
        else if (uhd::wtohx(mem32[words32]) == 0) mrb.reset();

        return make(this, mem, len);
    }

private:
    managed_recv_buffer::sptr _mrb;
    simple_claimer _claimer;
};

/***********************************************************************
 * USB zero copy wrapper - managed send buffer
 **********************************************************************/
class usb_zero_copy_wrapper_msb : public managed_send_buffer{
public:
    usb_zero_copy_wrapper_msb(const usb_zero_copy::sptr internal, const size_t fragmentation_size):
        _internal(internal), _fragmentation_size(fragmentation_size)
    {
        _ok_to_auto_flush = false;
        _task = uhd::task::make(boost::bind(&usb_zero_copy_wrapper_msb::auto_flush, this));
    }

    ~usb_zero_copy_wrapper_msb(void)
    {
        //ensure the task has exited before anything auto deconstructs
        _task.reset();
    }

    void release(void){
        boost::mutex::scoped_lock lock(_mutex);
        _ok_to_auto_flush = true;

        //get a reference to the VITA header before incrementing
        const boost::uint32_t vita_header = reinterpret_cast<const boost::uint32_t *>(_mem_buffer_tip)[0];

        _bytes_in_buffer += size();
        _mem_buffer_tip += size();

        //extract VITA end of packet flag, we must force flush under eof conditions
        const bool eop = (uhd::wtohx(vita_header) & (0x1 << 24)) != 0;
        const bool full = _bytes_in_buffer >= (_last_send_buff->size() - _fragmentation_size);
        if (eop or full){
            _last_send_buff->commit(_bytes_in_buffer);
            _last_send_buff.reset();

            //notify the auto-flusher to restart its timed_wait
            lock.unlock(); _cond.notify_one();
        }
    }

    UHD_INLINE sptr get_new(const double timeout){
        boost::mutex::scoped_lock lock(_mutex);
        _ok_to_auto_flush = false;

        if (not _last_send_buff){
            _last_send_buff = _internal->get_send_buff(timeout);
            if (not _last_send_buff) return sptr();
            _mem_buffer_tip = _last_send_buff->cast<char *>();
            _bytes_in_buffer = 0;
        }

        return make(this, _mem_buffer_tip, _fragmentation_size);
    }

private:
    usb_zero_copy::sptr _internal;
    const size_t _fragmentation_size;
    managed_send_buffer::sptr _last_send_buff;
    size_t _bytes_in_buffer;
    char *_mem_buffer_tip;

    //private variables for auto flusher
    boost::mutex _mutex;
    boost::condition_variable _cond;
    uhd::task::sptr _task;
    bool _ok_to_auto_flush;

    /*!
     * The auto flusher ensures that buffers are force committed when
     * the user has not called get_new() within a certain time window.
     */
    void auto_flush(void)
    {
        boost::mutex::scoped_lock lock(_mutex);
        const bool timeout = not _cond.timed_wait(lock, AUTOFLUSH_TIMEOUT);
        if (timeout and _ok_to_auto_flush and _last_send_buff and _bytes_in_buffer != 0)
        {
            _last_send_buff->commit(_bytes_in_buffer);
            _last_send_buff.reset();
        }
    }
};

/***********************************************************************
 * USB zero copy wrapper implementation
 **********************************************************************/
class usb_zero_copy_wrapper : public usb_zero_copy{
public:
    usb_zero_copy_wrapper(sptr usb_zc, const size_t frame_boundary):
        _internal_zc(usb_zc),
        _frame_boundary(frame_boundary),
        _next_recv_buff_index(0)
    {
        for (size_t i = 0; i < this->get_num_recv_frames(); i++){
            _mrb_pool.push_back(boost::make_shared<usb_zero_copy_wrapper_mrb>());
        }
        _the_only_msb = boost::make_shared<usb_zero_copy_wrapper_msb>(usb_zc, frame_boundary);
    }

    managed_recv_buffer::sptr get_recv_buff(double timeout){
        //attempt to get a managed recv buffer
        if (not _last_recv_buff){
            _last_recv_buff = _internal_zc->get_recv_buff(timeout);
            _last_recv_offset = 0; //reset offset into buffer
        }

        //get the buffer to be returned to the user
        if (_next_recv_buff_index == _mrb_pool.size()) _next_recv_buff_index = 0;
        return _mrb_pool[_next_recv_buff_index]->get_new(
            _last_recv_buff, _last_recv_offset, timeout, _next_recv_buff_index
        );
    }

    size_t get_num_recv_frames(void) const{
        return _internal_zc->get_num_recv_frames();
    }

    size_t get_recv_frame_size(void) const{
        return std::min(_frame_boundary, _internal_zc->get_recv_frame_size());
    }

    managed_send_buffer::sptr get_send_buff(double timeout){
        return _the_only_msb->get_new(timeout);
    }

    size_t get_num_send_frames(void) const{
        return _internal_zc->get_num_send_frames();
    }

    size_t get_send_frame_size(void) const{
        return std::min(_frame_boundary, _internal_zc->get_send_frame_size());
    }

private:
    sptr _internal_zc;
    size_t _frame_boundary;
    std::vector<boost::shared_ptr<usb_zero_copy_wrapper_mrb> > _mrb_pool;
    boost::shared_ptr<usb_zero_copy_wrapper_msb> _the_only_msb;

    //state for last recv buffer to create multiple managed buffers
    managed_recv_buffer::sptr _last_recv_buff;
    size_t _last_recv_offset;
    size_t _next_recv_buff_index;
};

/***********************************************************************
 * USB zero copy wrapper factory function
 **********************************************************************/
usb_zero_copy::sptr usb_zero_copy::make_wrapper(
    sptr usb_zc, size_t usb_frame_boundary
){
    return sptr(new usb_zero_copy_wrapper(usb_zc, usb_frame_boundary));
}
