//
// Copyright 2011 Ettus Research LLC
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
#include <uhd/transport/bounded_buffer.hpp>
#include <uhd/transport/buffer_pool.hpp>
#include <boost/foreach.hpp>
#include <vector>
#include <iostream>

using namespace uhd::transport;
bool debug = true;

static inline size_t next_boundary(size_t length, size_t boundary){
    //pad to the boundary, assumes boundary is a power of 2
    return (length + (boundary-1)) & ~(boundary-1);
}

/***********************************************************************
 * USB zero copy wrapper - managed receive buffer
 **********************************************************************/
class usb_zero_copy_wrapper_mrb : public managed_recv_buffer{
public:
    usb_zero_copy_wrapper_mrb(bounded_buffer<usb_zero_copy_wrapper_mrb *> &queue):
        _queue(queue){/*NOP*/}

    void release(void){
        if (_mrb.get() == NULL) return;
        _mrb->release();
        _queue.push_with_haste(this);
        _mrb.reset();
    }

    sptr get_new(managed_recv_buffer::sptr mrb, const void *mem, size_t len){
        _mrb = mrb;
        _mem = mem;
        _len = len;
        return make_managed_buffer(this);
    }

private:
    const void *get_buff(void) const{return _mem;}
    size_t get_size(void) const{return _len;}

    bounded_buffer<usb_zero_copy_wrapper_mrb *> &_queue;
    const void *_mem;
    size_t _len;
    managed_recv_buffer::sptr _mrb;
};

/***********************************************************************
 * USB zero copy wrapper - managed send buffer
 **********************************************************************/
class usb_zero_copy_wrapper_msb : public managed_send_buffer{
public:
    usb_zero_copy_wrapper_msb(bounded_buffer<usb_zero_copy_wrapper_msb *> &queue, size_t boundary):
        _queue(queue), _boundary(boundary){/*NOP*/}

    void commit(size_t len){
        if (_msb.get() == NULL) return;
        _msb->commit(len);
        _queue.push_with_haste(this);
        _msb.reset();
    }

    sptr get_new(managed_send_buffer::sptr msb){
        _msb = msb;
        return make_managed_buffer(this);
    }

private:
    void *get_buff(void) const{return _msb->cast<void *>();}
    size_t get_size(void) const{return _msb->size();}

    bounded_buffer<usb_zero_copy_wrapper_msb *> &_queue;
    size_t _boundary;
    managed_send_buffer::sptr _msb;
};

/***********************************************************************
 * USB zero copy wrapper implementation
 **********************************************************************/
class usb_zero_copy_wrapper : public usb_zero_copy{
public:
    usb_zero_copy_wrapper(
        sptr usb_zc, size_t usb_frame_boundary
    ):
        _internal_zc(usb_zc),
        _usb_frame_boundary(usb_frame_boundary),
        _available_recv_buffs(this->get_num_recv_frames()),
        _available_send_buffs(this->get_num_send_frames()),
        _mrb_pool(this->get_num_recv_frames(), usb_zero_copy_wrapper_mrb(_available_recv_buffs)),
        _msb_pool(this->get_num_send_frames(), usb_zero_copy_wrapper_msb(_available_send_buffs, usb_frame_boundary))
    {
        BOOST_FOREACH(usb_zero_copy_wrapper_mrb &mrb, _mrb_pool){
            _available_recv_buffs.push_with_haste(&mrb);
        }

        BOOST_FOREACH(usb_zero_copy_wrapper_msb &msb, _msb_pool){
            _available_send_buffs.push_with_haste(&msb);
        }
    }

    managed_recv_buffer::sptr get_recv_buff(double timeout){
        //attempt to get a managed recv buffer
        if (not _last_recv_buff.get()){
            _last_recv_buff = _internal_zc->get_recv_buff(timeout);
            _last_recv_offset = 0;
        }

        //attempt to get a wrapper for a managed recv buffer
        usb_zero_copy_wrapper_mrb *wmrb = NULL;
        if (_last_recv_buff.get() and _available_recv_buffs.pop_with_timed_wait(wmrb, timeout)){
            //extract this packet's memory address and length in bytes
            const char *mem = _last_recv_buff->cast<const char *>() + _last_recv_offset;
            const boost::uint32_t *mem32 = reinterpret_cast<const boost::uint32_t *>(mem);
            size_t len = (mem32[0] & 0xffff)*sizeof(boost::uint32_t); //length in bytes (from VRT header)
            
            managed_recv_buffer::sptr recv_buff; //the buffer to be returned to the user
            
            recv_buff = wmrb->get_new(_last_recv_buff, mem, len);
            _last_recv_offset = next_boundary(_last_recv_offset + len, 4);
            
            //check if this receive buffer has been exhausted
            if (_last_recv_offset >= _last_recv_buff->size()) {
                _last_recv_buff.reset();
            }
            
            return recv_buff;
        }

        //otherwise return a null sptr for failure
        return managed_recv_buffer::sptr();
    }

    size_t get_num_recv_frames(void) const{
        return _internal_zc->get_num_recv_frames();
    }

    size_t get_recv_frame_size(void) const{
        return _internal_zc->get_recv_frame_size();
    }

    managed_send_buffer::sptr get_send_buff(double timeout){
        managed_send_buffer::sptr send_buff = _internal_zc->get_send_buff(timeout);
        
        //attempt to get a wrapper for a managed send buffer
        usb_zero_copy_wrapper_msb *wmsb = NULL;
        if (send_buff.get() and _available_send_buffs.pop_with_haste(wmsb)){
            return wmsb->get_new(send_buff);
        }

        //otherwise return a null sptr for failure
        return managed_send_buffer::sptr();
    }

    size_t get_num_send_frames(void) const{
        return _internal_zc->get_num_send_frames();
    }

    size_t get_send_frame_size(void) const{
        return _internal_zc->get_send_frame_size();
    }

private:
    sptr _internal_zc;
    size_t _usb_frame_boundary;
    bounded_buffer<usb_zero_copy_wrapper_mrb *> _available_recv_buffs;
    bounded_buffer<usb_zero_copy_wrapper_msb *> _available_send_buffs;
    std::vector<usb_zero_copy_wrapper_mrb> _mrb_pool;
    std::vector<usb_zero_copy_wrapper_msb> _msb_pool;
    
    //buffer to store partially-received VRT packets in
    buffer_pool::sptr _fragment_mem;
    
    //state for last recv buffer to create multiple managed buffers
    managed_recv_buffer::sptr _last_recv_buff;
    size_t _last_recv_offset;
};

/***********************************************************************
 * USB zero copy wrapper factory function
 **********************************************************************/
usb_zero_copy::sptr usb_zero_copy::make_wrapper(
    sptr usb_zc, size_t usb_frame_boundary
){
    return sptr(new usb_zero_copy_wrapper(usb_zc, usb_frame_boundary));
}
