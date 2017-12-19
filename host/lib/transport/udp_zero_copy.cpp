//
// Copyright 2010-2013 Ettus Research LLC
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
#include <uhd/transport/buffer_pool.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/atomic.hpp>
#include <boost/format.hpp>
#include <boost/make_shared.hpp>
#include <boost/thread/thread.hpp> //sleep
#include <vector>

using namespace uhd;
using namespace uhd::transport;
namespace asio = boost::asio;

//A reasonable number of frames for send/recv and async/sync
//static const size_t DEFAULT_NUM_FRAMES = 32;

/***********************************************************************
 * Check registry for correct fast-path setting (windows only)
 **********************************************************************/
#ifdef HAVE_ATLBASE_H
#define CHECK_REG_SEND_THRESH
#include <atlbase.h> //CRegKey
static void check_registry_for_fast_send_threshold(const size_t mtu){
    static bool warned = false;
    if (warned) return; //only allow one printed warning per process

    CRegKey reg_key;
    DWORD threshold = 1024; //system default when threshold is not specified
    if (
        reg_key.Open(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Services\\AFD\\Parameters", KEY_READ) != ERROR_SUCCESS or
        reg_key.QueryDWORDValue("FastSendDatagramThreshold", threshold) != ERROR_SUCCESS or threshold < mtu
    ){
        UHD_MSG(warning) << boost::format(
            "The MTU (%d) is larger than the FastSendDatagramThreshold (%d)!\n"
            "This will negatively affect the transmit performance.\n"
            "See the transport application notes for more detail.\n"
        ) % mtu % threshold << std::endl;
        warned = true;
    }
    reg_key.Close();
}
#endif /*HAVE_ATLBASE_H*/

/***********************************************************************
 * Reusable managed receiver buffer:
 *  - get_new performs the recv operation
 **********************************************************************/
class udp_zero_copy_asio_mrb : public managed_recv_buffer{
public:
    udp_zero_copy_asio_mrb(void *mem, int sock_fd, const size_t frame_size):
        _mem(mem), _sock_fd(sock_fd), _frame_size(frame_size), _len(0) { /*NOP*/ }

    void release(void){
        _claimer.release();
    }

    UHD_INLINE sptr get_new(const double timeout, size_t &index){
        if (not _claimer.claim_with_wait(timeout)) return sptr();

        #ifdef MSG_DONTWAIT //try a non-blocking recv() if supported
        _len = ::recv(_sock_fd, (char *)_mem, _frame_size, MSG_DONTWAIT);
        if (_len > 0){
            index++; //advances the caller's buffer
            return make(this, _mem, size_t(_len));
        }
        #endif

        if (wait_for_recv_ready(_sock_fd, timeout)){
            _len = ::recv(_sock_fd, (char *)_mem, _frame_size, 0);
            if (_len == 0)
                throw uhd::io_error("socket closed");
            if (_len < 0)
                throw uhd::io_error(str(boost::format("recv error on socket: %s") % strerror(errno)));
            index++; //advances the caller's buffer
            return make(this, _mem, size_t(_len));
        }

        _claimer.release(); //undo claim
        return sptr(); //null for timeout
    }

private:
    void *_mem;
    int _sock_fd;
    size_t _frame_size;
    ssize_t _len;
    simple_claimer _claimer;
};

/***********************************************************************
 * Reusable managed send buffer:
 *  - commit performs the send operation
 **********************************************************************/
class udp_zero_copy_asio_msb : public managed_send_buffer{
public:
    udp_zero_copy_asio_msb(void *mem, int sock_fd, const size_t frame_size):
        _mem(mem), _sock_fd(sock_fd), _frame_size(frame_size) { /*NOP*/ }

    void release(void){
        //Retry logic because send may fail with ENOBUFS.
        //This is known to occur at least on some OSX systems.
        //But it should be safe to always check for the error.
        while (true)
        {
            const ssize_t ret = ::send(_sock_fd, (const char *)_mem, size(), 0);
            if (ret == ssize_t(size())) break;
            if (ret == -1 and errno == ENOBUFS)
            {
                boost::this_thread::sleep(boost::posix_time::microseconds(1));
                continue; //try to send again
            }
            if (ret == -1)
            {
                throw uhd::io_error(str(boost::format("send error on socket: %s") % strerror(errno)));
            }
            UHD_ASSERT_THROW(ret == ssize_t(size()));
        }
        _claimer.release();
    }

    UHD_INLINE sptr get_new(const double timeout, size_t &index){
        if (not _claimer.claim_with_wait(timeout)) return sptr();
        index++; //advances the caller's buffer
        return make(this, _mem, _frame_size);
    }

private:
    void *_mem;
    int _sock_fd;
    size_t _frame_size;
    simple_claimer _claimer;
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
        const zero_copy_xport_params& xport_params
    ):
        _recv_frame_size(xport_params.recv_frame_size),
        _num_recv_frames(xport_params.num_recv_frames),
        _send_frame_size(xport_params.send_frame_size),
        _num_send_frames(xport_params.num_send_frames),
        _recv_buffer_pool(buffer_pool::make(xport_params.num_recv_frames, xport_params.recv_frame_size)),
        _send_buffer_pool(buffer_pool::make(xport_params.num_send_frames, xport_params.send_frame_size)),
        _next_recv_buff_index(0), _next_send_buff_index(0)
    {
        UHD_LOG << boost::format("Creating udp transport for %s %s") % addr % port << std::endl;

        #ifdef CHECK_REG_SEND_THRESH
        check_registry_for_fast_send_threshold(this->get_send_frame_size());
        #endif /*CHECK_REG_SEND_THRESH*/

        //resolve the address
        asio::ip::udp::resolver resolver(_io_service);
        asio::ip::udp::resolver::query query(asio::ip::udp::v4(), addr, port);
        asio::ip::udp::endpoint receiver_endpoint = *resolver.resolve(query);

        //create, open, and connect the socket
        _socket = socket_sptr(new asio::ip::udp::socket(_io_service));
        _socket->open(asio::ip::udp::v4());
        _socket->connect(receiver_endpoint);
        _sock_fd = _socket->native_handle();

        //allocate re-usable managed receive buffers
        for (size_t i = 0; i < get_num_recv_frames(); i++){
            _mrb_pool.push_back(boost::make_shared<udp_zero_copy_asio_mrb>(
                _recv_buffer_pool->at(i), _sock_fd, get_recv_frame_size()
            ));
        }

        //allocate re-usable managed send buffers
        for (size_t i = 0; i < get_num_send_frames(); i++){
            _msb_pool.push_back(boost::make_shared<udp_zero_copy_asio_msb>(
                _send_buffer_pool->at(i), _sock_fd, get_send_frame_size()
            ));
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
     * Block on the managed buffer's get call and advance the index.
     ******************************************************************/
    managed_recv_buffer::sptr get_recv_buff(double timeout){
        if (_next_recv_buff_index == _num_recv_frames) _next_recv_buff_index = 0;
        return _mrb_pool[_next_recv_buff_index]->get_new(timeout, _next_recv_buff_index);
    }

    size_t get_num_recv_frames(void) const {return _num_recv_frames;}
    size_t get_recv_frame_size(void) const {return _recv_frame_size;}

    /*******************************************************************
     * Send implementation:
     * Block on the managed buffer's get call and advance the index.
     ******************************************************************/
    managed_send_buffer::sptr get_send_buff(double timeout){
        if (_next_send_buff_index == _num_send_frames) _next_send_buff_index = 0;
        return _msb_pool[_next_send_buff_index]->get_new(timeout, _next_send_buff_index);
    }

    size_t get_num_send_frames(void) const {return _num_send_frames;}
    size_t get_send_frame_size(void) const {return _send_frame_size;}

private:
    //memory management -> buffers and fifos
    const size_t _recv_frame_size, _num_recv_frames;
    const size_t _send_frame_size, _num_send_frames;
    buffer_pool::sptr _recv_buffer_pool, _send_buffer_pool;
    std::vector<boost::shared_ptr<udp_zero_copy_asio_msb> > _msb_pool;
    std::vector<boost::shared_ptr<udp_zero_copy_asio_mrb> > _mrb_pool;
    size_t _next_recv_buff_index, _next_send_buff_index;

    //asio guts -> socket and service
    asio::io_service        _io_service;
    socket_sptr             _socket;
    int                     _sock_fd;
};

/***********************************************************************
 * UDP zero copy make function
 **********************************************************************/
template<typename Opt> static size_t resize_buff_helper(
    udp_zero_copy_asio_impl::sptr udp_trans,
    const size_t target_size,
    const std::string &name
){
    size_t actual_size = 0;
    std::string help_message;
    #if defined(UHD_PLATFORM_LINUX)
        help_message = str(boost::format(
            "Please run: sudo sysctl -w net.core.%smem_max=%d\n"
        ) % ((name == "recv")?"r":"w") % target_size);
    #endif /*defined(UHD_PLATFORM_LINUX)*/

    //resize the buffer if size was provided
    if (target_size > 0){
        actual_size = udp_trans->resize_buff<Opt>(target_size);
        UHD_LOG << boost::format(
            "Target %s sock buff size: %d bytes\n"
            "Actual %s sock buff size: %d bytes"
        ) % name % target_size % name % actual_size << std::endl;
        if (actual_size < target_size) UHD_MSG(warning) << boost::format(
            "The %s buffer could not be resized sufficiently.\n"
            "Target sock buff size: %d bytes.\n"
            "Actual sock buff size: %d bytes.\n"
            "See the transport application notes on buffer resizing.\n%s"
        ) % name % target_size % actual_size % help_message;
    }

    return actual_size;
}

udp_zero_copy::sptr udp_zero_copy::make(
    const std::string &addr,
    const std::string &port,
    const zero_copy_xport_params &default_buff_args,
    udp_zero_copy::buff_params& buff_params_out,
    const device_addr_t &hints
){
    //Initialize xport_params
    zero_copy_xport_params xport_params = default_buff_args;

    xport_params.recv_frame_size = size_t(hints.cast<double>("recv_frame_size", default_buff_args.recv_frame_size));
    xport_params.num_recv_frames = size_t(hints.cast<double>("num_recv_frames", default_buff_args.num_recv_frames));
    xport_params.send_frame_size = size_t(hints.cast<double>("send_frame_size", default_buff_args.send_frame_size));
    xport_params.num_send_frames = size_t(hints.cast<double>("num_send_frames", default_buff_args.num_send_frames));

    //extract buffer size hints from the device addr
    size_t usr_recv_buff_size = size_t(hints.cast<double>("recv_buff_size", xport_params.num_recv_frames * MAX_ETHERNET_MTU));
    size_t usr_send_buff_size = size_t(hints.cast<double>("send_buff_size", xport_params.num_send_frames * MAX_ETHERNET_MTU));

    if (hints.has_key("recv_buff_size")) {
        if (usr_recv_buff_size < xport_params.num_recv_frames * MAX_ETHERNET_MTU) {
            throw uhd::value_error((boost::format(
                "recv_buff_size must be equal to or greater than %d")
                % (xport_params.num_recv_frames * MAX_ETHERNET_MTU)).str());
        }
    }

    if (hints.has_key("send_buff_size")) {
        if (usr_send_buff_size < xport_params.num_send_frames * MAX_ETHERNET_MTU) {
            throw uhd::value_error((boost::format(
                "send_buff_size must be equal to or greater than %d")
                % (xport_params.num_send_frames * MAX_ETHERNET_MTU)).str());
        }
    }

    udp_zero_copy_asio_impl::sptr udp_trans(
        new udp_zero_copy_asio_impl(addr, port, xport_params)
    );

    //call the helper to resize send and recv buffers
    buff_params_out.recv_buff_size =
        resize_buff_helper<asio::socket_base::receive_buffer_size>(udp_trans, usr_recv_buff_size, "recv");
    buff_params_out.send_buff_size =
        resize_buff_helper<asio::socket_base::send_buffer_size>   (udp_trans, usr_send_buff_size, "send");

    return udp_trans;
}
