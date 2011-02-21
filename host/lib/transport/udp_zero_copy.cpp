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

#include "udp_common.hpp"
#include <uhd/transport/udp_zero_copy.hpp>
#include <uhd/transport/udp_simple.hpp> //mtu
#include <uhd/transport/bounded_buffer.hpp>
#include <uhd/transport/buffer_pool.hpp>
#include <uhd/utils/warning.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <list>

using namespace uhd;
using namespace uhd::transport;
namespace asio = boost::asio;

//A reasonable number of frames for send/recv and async/sync
static const size_t DEFAULT_NUM_FRAMES = 32;

/***********************************************************************
 * Reusable managed receiver buffer:
 *  - Initialize with memory and a release callback.
 *  - Call get new with a length in bytes to re-use.
 **********************************************************************/
class udp_zero_copy_asio_mrb : public managed_recv_buffer{
public:
    typedef boost::function<void(udp_zero_copy_asio_mrb *)> release_cb_type;

    udp_zero_copy_asio_mrb(void *mem, const release_cb_type &release_cb):
        _mem(mem), _len(0), _release_cb(release_cb){/* NOP */}

    void release(void){
        if (_len == 0) return;
        this->_release_cb(this);
        _len = 0;
    }

    sptr get_new(size_t len){
        _len = len;
        return sptr(this, &udp_zero_copy_asio_mrb::fake_deleter);
    }

    template <class T> T cast(void) const{return static_cast<T>(_mem);}

private:
    static void fake_deleter(void *obj){
        static_cast<udp_zero_copy_asio_mrb *>(obj)->release();
    }

    const void *get_buff(void) const{return _mem;}
    size_t get_size(void) const{return _len;}

    void *_mem;
    size_t _len;
    release_cb_type _release_cb;
};

/***********************************************************************
 * Reusable managed send buffer:
 *  - Initialize with memory and a commit callback.
 *  - Call get new with a length in bytes to re-use.
 **********************************************************************/
class udp_zero_copy_asio_msb : public managed_send_buffer{
public:
    typedef boost::function<void(udp_zero_copy_asio_msb *, size_t)> commit_cb_type;

    udp_zero_copy_asio_msb(void *mem, const commit_cb_type &commit_cb):
        _mem(mem), _len(0), _commit_cb(commit_cb){/* NOP */}

    void commit(size_t len){
        if (_len == 0) return;
        this->_commit_cb(this, len);
        _len = 0;
    }

    sptr get_new(size_t len){
        _len = len;
        return sptr(this, &udp_zero_copy_asio_msb::fake_deleter);
    }

private:
    static void fake_deleter(void *obj){
        static_cast<udp_zero_copy_asio_msb *>(obj)->commit(0);
    }

    void *get_buff(void) const{return _mem;}
    size_t get_size(void) const{return _len;}

    void *_mem;
    size_t _len;
    commit_cb_type _commit_cb;
};

/***********************************************************************
 * Zero Copy UDP implementation with ASIO:
 *   This is the portable zero copy implementation for systems
 *   where a faster, platform specific solution is not available.
 *   However, it is not a true zero copy implementation as each
 *   send and recv requires a copy operation to/from userspace.
 **********************************************************************/
class udp_zero_copy_asio_impl : public udp_zero_copy{
public:
    typedef boost::shared_ptr<udp_zero_copy_asio_impl> sptr;

    udp_zero_copy_asio_impl(
        const std::string &addr,
        const std::string &port,
        const device_addr_t &hints
    ):
        _recv_frame_size(size_t(hints.cast<double>("recv_frame_size", udp_simple::mtu))),
        _num_recv_frames(size_t(hints.cast<double>("num_recv_frames", DEFAULT_NUM_FRAMES))),
        _send_frame_size(size_t(hints.cast<double>("send_frame_size", udp_simple::mtu))),
        _num_send_frames(size_t(hints.cast<double>("num_send_frames", DEFAULT_NUM_FRAMES))),
        _recv_buffer_pool(buffer_pool::make(_num_recv_frames, _recv_frame_size)),
        _send_buffer_pool(buffer_pool::make(_num_send_frames, _send_frame_size)),
        _pending_recv_buffs(_num_recv_frames),
        _pending_send_buffs(_num_send_frames)
    {
        //std::cout << boost::format("Creating udp transport for %s %s") % addr % port << std::endl;

        //resolve the address
        asio::ip::udp::resolver resolver(_io_service);
        asio::ip::udp::resolver::query query(asio::ip::udp::v4(), addr, port);
        asio::ip::udp::endpoint receiver_endpoint = *resolver.resolve(query);

        //create, open, and connect the socket
        _socket = socket_sptr(new asio::ip::udp::socket(_io_service));
        _socket->open(asio::ip::udp::v4());
        _socket->connect(receiver_endpoint);
        _sock_fd = _socket->native();

        //allocate re-usable managed receive buffers
        for (size_t i = 0; i < get_num_recv_frames(); i++){
            _mrb_pool.push_back(udp_zero_copy_asio_mrb(_recv_buffer_pool->at(i),
                boost::bind(&udp_zero_copy_asio_impl::release, this, _1))
            );
            handle_recv(&_mrb_pool.back());
        }

        //allocate re-usable managed send buffers
        for (size_t i = 0; i < get_num_send_frames(); i++){
            _msb_pool.push_back(udp_zero_copy_asio_msb(_send_buffer_pool->at(i),
                boost::bind(&udp_zero_copy_asio_impl::commit, this, _1, _2))
            );
            handle_send(&_msb_pool.back());
        }
    }

    //get size for internal socket buffer
    template <typename Opt> size_t get_buff_size(void) const{
        Opt option;
        _socket->get_option(option);
        return option.value();
    }

    //set size for internal socket buffer
    template <typename Opt> size_t resize_buff(size_t num_bytes){
        Opt option(num_bytes);
        _socket->set_option(option);
        return get_buff_size<Opt>();
    }

    /*******************************************************************
     * Receive implementation:
     *
     * Perform a non-blocking receive for performance,
     * and then fall back to a blocking receive with timeout.
     * Return the managed receive buffer with the new length.
     * When the caller is finished with the managed buffer,
     * the managed receive buffer is released back into the queue.
     ******************************************************************/
    managed_recv_buffer::sptr get_recv_buff(double timeout){
        udp_zero_copy_asio_mrb *mrb = NULL;
        if (_pending_recv_buffs.pop_with_timed_wait(mrb, timeout)){

            #ifdef MSG_DONTWAIT //try a non-blocking recv() if supported
            ssize_t ret = ::recv(_sock_fd, mrb->cast<char *>(), _recv_frame_size, MSG_DONTWAIT);
            if (ret > 0) return mrb->get_new(ret);
            #endif

            if (wait_for_recv_ready(_sock_fd, timeout)) return mrb->get_new(
                ::recv(_sock_fd, mrb->cast<char *>(), _recv_frame_size, 0)
            );
        }
        return managed_recv_buffer::sptr();
    }

    UHD_INLINE void handle_recv(udp_zero_copy_asio_mrb *mrb){
        _pending_recv_buffs.push_with_haste(mrb);
    }

    void release(udp_zero_copy_asio_mrb *mrb){
        handle_recv(mrb);
    }

    size_t get_num_recv_frames(void) const {return _num_recv_frames;}
    size_t get_recv_frame_size(void) const {return _recv_frame_size;}

    /*******************************************************************
     * Send implementation:
     *
     * Get a managed receive buffer immediately with max length set.
     * The caller will fill the buffer and commit it when finished.
     * The commit routine will perform a blocking send operation,
     * and push the managed send buffer back into the queue.
     ******************************************************************/
    managed_send_buffer::sptr get_send_buff(double timeout){
        udp_zero_copy_asio_msb *msb = NULL;
        if (_pending_send_buffs.pop_with_timed_wait(msb, timeout)){
            return msb->get_new(_send_frame_size);
        }
        return managed_send_buffer::sptr();
    }

    UHD_INLINE void handle_send(udp_zero_copy_asio_msb *msb){
        _pending_send_buffs.push_with_haste(msb);
    }

    void commit(udp_zero_copy_asio_msb *msb, size_t len){
        ::send(_sock_fd, msb->cast<const char *>(), len, 0);
        handle_send(msb);
    }

    size_t get_num_send_frames(void) const {return _num_send_frames;}
    size_t get_send_frame_size(void) const {return _send_frame_size;}

private:
    //memory management -> buffers and fifos
    const size_t _recv_frame_size, _num_recv_frames;
    const size_t _send_frame_size, _num_send_frames;
    buffer_pool::sptr _recv_buffer_pool, _send_buffer_pool;
    bounded_buffer<udp_zero_copy_asio_mrb *> _pending_recv_buffs;
    bounded_buffer<udp_zero_copy_asio_msb *> _pending_send_buffs;
    std::list<udp_zero_copy_asio_msb> _msb_pool;
    std::list<udp_zero_copy_asio_mrb> _mrb_pool;

    //asio guts -> socket and service
    asio::io_service        _io_service;
    socket_sptr             _socket;
    int                     _sock_fd;
};

/***********************************************************************
 * UDP zero copy make function
 **********************************************************************/
template<typename Opt> static void resize_buff_helper(
    udp_zero_copy_asio_impl::sptr udp_trans,
    const size_t target_size,
    const std::string &name
){
    std::string help_message;
    #if defined(UHD_PLATFORM_LINUX)
        help_message = str(boost::format(
            "Please run: sudo sysctl -w net.core.%smem_max=%d\n"
        ) % ((name == "recv")?"r":"w") % target_size);
    #endif /*defined(UHD_PLATFORM_LINUX)*/

    //resize the buffer if size was provided
    if (target_size > 0){
        size_t actual_size = udp_trans->resize_buff<Opt>(target_size);
        if (target_size != actual_size) std::cout << boost::format(
            "Target %s sock buff size: %d bytes\n"
            "Actual %s sock buff size: %d bytes"
        ) % name % target_size % name % actual_size << std::endl;
        else std::cout << boost::format(
            "Current %s sock buff size: %d bytes"
        ) % name % actual_size << std::endl;
        if (actual_size < target_size) uhd::warning::post(str(boost::format(
            "The %s buffer could not be resized sufficiently.\n"
            "See the transport application notes on buffer resizing.\n%s"
        ) % name % help_message));
    }
}

udp_zero_copy::sptr udp_zero_copy::make(
    const std::string &addr,
    const std::string &port,
    const device_addr_t &hints
){
    udp_zero_copy_asio_impl::sptr udp_trans(
        new udp_zero_copy_asio_impl(addr, port, hints)
    );

    //extract buffer size hints from the device addr
    size_t recv_buff_size = size_t(hints.cast<double>("recv_buff_size", 0.0));
    size_t send_buff_size = size_t(hints.cast<double>("send_buff_size", 0.0));

    //call the helper to resize send and recv buffers
    resize_buff_helper<asio::socket_base::receive_buffer_size>(udp_trans, recv_buff_size, "recv");
    resize_buff_helper<asio::socket_base::send_buffer_size>   (udp_trans, send_buff_size, "send");

    return udp_trans;
}
