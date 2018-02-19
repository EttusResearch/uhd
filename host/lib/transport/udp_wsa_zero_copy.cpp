//
// Copyright 2010-2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "udp_common.hpp"
#include <uhd/transport/udp_zero_copy.hpp>
#include <uhd/transport/udp_simple.hpp> //mtu
#include <uhd/transport/buffer_pool.hpp>

#include <uhd/utils/log.hpp>
#include <boost/format.hpp>
#include <vector>

using namespace uhd;
using namespace uhd::transport;
namespace asio = boost::asio;

//A reasonable number of frames for send/recv and async/sync
static const size_t DEFAULT_NUM_FRAMES = 32;

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
        UHD_LOGGER_WARNING("UDP") << boost::format(
            "The MTU (%d) is larger than the FastSendDatagramThreshold (%d)!\n"
            "This will negatively affect the transmit performance.\n"
            "See the transport application notes for more detail.\n"
        ) % mtu % threshold ;
        warned = true;
    }
    reg_key.Close();
}
#endif /*HAVE_ATLBASE_H*/

/***********************************************************************
 * Static initialization to take care of WSA init and cleanup
 **********************************************************************/
struct uhd_wsa_control{
    uhd_wsa_control(void){
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData); /*windows socket startup */
    }

    ~uhd_wsa_control(void){
        WSACleanup();
    }
};

/***********************************************************************
 * Reusable managed receiver buffer:
 *  - Initialize with memory and a release callback.
 *  - Call get new with a length in bytes to re-use.
 **********************************************************************/
class udp_zero_copy_asio_mrb : public managed_recv_buffer{
public:
    udp_zero_copy_asio_mrb(void *mem, int sock_fd, const size_t frame_size):
        _sock_fd(sock_fd), _frame_size(frame_size)
    {
        _wsa_buff.buf = reinterpret_cast<char *>(mem);
        ZeroMemory(&_overlapped, sizeof(_overlapped));
        _overlapped.hEvent = WSACreateEvent();
        UHD_ASSERT_THROW(_overlapped.hEvent != WSA_INVALID_EVENT);
        this->release(); //makes buffer available via get_new
    }

    ~udp_zero_copy_asio_mrb(void){
        WSACloseEvent(_overlapped.hEvent);
    }

    void release(void){
        _wsa_buff.len = _frame_size;
        _flags = 0;
        WSARecv(_sock_fd, &_wsa_buff, 1, &_wsa_buff.len, &_flags, &_overlapped, NULL);
    }

    UHD_INLINE sptr get_new(const double timeout, size_t &index){
        const DWORD result = WSAWaitForMultipleEvents(
            1, &_overlapped.hEvent, true, DWORD(timeout*1000), true
        );
        if (result == WSA_WAIT_TIMEOUT) return managed_recv_buffer::sptr();
        index++; //advances the caller's buffer

        WSAGetOverlappedResult(_sock_fd, &_overlapped, &_wsa_buff.len, true, &_flags);

        WSAResetEvent(_overlapped.hEvent);
        return make(this, _wsa_buff.buf, _wsa_buff.len);
    }

private:
    int _sock_fd;
    const size_t _frame_size;
    WSAOVERLAPPED _overlapped;
    WSABUF _wsa_buff;
    DWORD _flags;
};

/***********************************************************************
 * Reusable managed send buffer:
 *  - committing the buffer calls the asynchronous socket send
 *  - getting a new buffer performs the blocking wait for completion
 **********************************************************************/
class udp_zero_copy_asio_msb : public managed_send_buffer{
public:
    udp_zero_copy_asio_msb(void *mem, int sock_fd, const size_t frame_size):
        _sock_fd(sock_fd), _frame_size(frame_size)
    {
        _wsa_buff.buf = reinterpret_cast<char *>(mem);
        ZeroMemory(&_overlapped, sizeof(_overlapped));
        _overlapped.hEvent = WSACreateEvent();
        UHD_ASSERT_THROW(_overlapped.hEvent != WSA_INVALID_EVENT);
        WSASetEvent(_overlapped.hEvent); //makes buffer available via get_new
    }

    ~udp_zero_copy_asio_msb(void){
        WSACloseEvent(_overlapped.hEvent);
    }

    void release(void){
        _wsa_buff.len = size();
        WSASend(_sock_fd, &_wsa_buff, 1, NULL, 0, &_overlapped, NULL);
    }

    UHD_INLINE sptr get_new(const double timeout, size_t &index){
        const DWORD result = WSAWaitForMultipleEvents(
            1, &_overlapped.hEvent, true, DWORD(timeout*1000), true
        );
        if (result == WSA_WAIT_TIMEOUT) return managed_send_buffer::sptr();
        index++; //advances the caller's buffer

        WSAResetEvent(_overlapped.hEvent);
        _wsa_buff.len = _frame_size;
        return make(this, _wsa_buff.buf, _wsa_buff.len);
    }

private:
    int _sock_fd;
    const size_t _frame_size;
    WSAOVERLAPPED _overlapped;
    WSABUF _wsa_buff;
};

/***********************************************************************
 * Zero Copy UDP implementation with WSA:
 *
 *   This is not a true zero copy implementation as each
 *   send and recv requires a copy operation to/from userspace.
 *
 *   For receive, use a blocking recv() call on the socket.
 *   This has better performance than the overlapped IO.
 *   For send, use overlapped IO to submit async sends.
 **********************************************************************/
class udp_zero_copy_wsa_impl : public udp_zero_copy{
public:
    typedef boost::shared_ptr<udp_zero_copy_wsa_impl> sptr;

    udp_zero_copy_wsa_impl(
        const std::string &addr,
        const std::string &port,
        zero_copy_xport_params& xport_params,
        const device_addr_t &hints
    ):
        _recv_frame_size(xport_params.recv_frame_size),
        _num_recv_frames(xport_params.num_recv_frames),
        _send_frame_size(xport_params.send_frame_size),
        _num_send_frames(xport_params.num_send_frames),
        _recv_buffer_pool(buffer_pool::make(xport_params.num_recv_frames, xport_params.recv_frame_size)),
        _send_buffer_pool(buffer_pool::make(xport_params.num_send_frames, xport_params.send_frame_size)),
        _next_recv_buff_index(0), _next_send_buff_index(0)
    {
        #ifdef CHECK_REG_SEND_THRESH
        check_registry_for_fast_send_threshold(this->get_send_frame_size());
        #endif /*CHECK_REG_SEND_THRESH*/

        UHD_LOGGER_TRACE("UDP")
            << boost::format("Creating WSA UDP transport to %s:%s")
               % addr % port;

        static uhd_wsa_control uhd_wsa; //makes wsa start happen via lazy initialization

        UHD_ASSERT_THROW(_num_send_frames <= WSA_MAXIMUM_WAIT_EVENTS);

        //resolve the address
        asio::io_service io_service;
        asio::ip::udp::resolver resolver(io_service);
        asio::ip::udp::resolver::query query(asio::ip::udp::v4(), addr, port);
        asio::ip::udp::endpoint receiver_endpoint = *resolver.resolve(query);

        //create the socket
        _sock_fd = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);
        if (_sock_fd == INVALID_SOCKET){
            const DWORD error = WSAGetLastError();
            throw uhd::os_error(str(boost::format("WSASocket() failed with error %d") % error));
        }

        //set the socket non-blocking for recv
        //u_long mode = 1;
        //ioctlsocket(_sock_fd, FIONBIO, &mode);

        //resize the socket buffers
        const int recv_buff_size = int(hints.cast<double>("recv_buff_size", 0.0));
        const int send_buff_size = int(hints.cast<double>("send_buff_size", 0.0));
        if (recv_buff_size > 0) setsockopt(_sock_fd, SOL_SOCKET, SO_RCVBUF, (const char *)&recv_buff_size, sizeof(recv_buff_size));
        if (send_buff_size > 0) setsockopt(_sock_fd, SOL_SOCKET, SO_SNDBUF, (const char *)&send_buff_size, sizeof(send_buff_size));

        //connect the socket so we can send/recv
        const asio::ip::udp::endpoint::data_type &servaddr = *receiver_endpoint.data();
        if (WSAConnect(_sock_fd, (const struct sockaddr *)&servaddr, sizeof(servaddr), NULL, NULL, NULL, NULL) != 0){
            const DWORD error = WSAGetLastError();
            closesocket(_sock_fd);
            throw uhd::os_error(str(boost::format("WSAConnect() failed with error %d") % error));
        }

        UHD_LOGGER_TRACE("UDP")
            << boost::format("Local WSA UDP socket endpoint: %s:%s")
            % get_local_addr() % get_local_port();

        //allocate re-usable managed receive buffers
        for (size_t i = 0; i < get_num_recv_frames(); i++){
            _mrb_pool.push_back(boost::shared_ptr<udp_zero_copy_asio_mrb>(
                new udp_zero_copy_asio_mrb(_recv_buffer_pool->at(i), _sock_fd, get_recv_frame_size())
            ));
        }

        //allocate re-usable managed send buffers
        for (size_t i = 0; i < get_num_send_frames(); i++){
            _msb_pool.push_back(boost::shared_ptr<udp_zero_copy_asio_msb>(
                new udp_zero_copy_asio_msb(_send_buffer_pool->at(i), _sock_fd, get_send_frame_size())
            ));
        }
    }

    ~udp_zero_copy_wsa_impl(void){
        closesocket(_sock_fd);
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

    uint16_t get_local_port(void) const {
        struct sockaddr_in addr_info;
        int addr_len = sizeof(addr_info);
        uint16_t local_port = 0;
        if (getsockname( _sock_fd, (SOCKADDR*) &addr_info,
                         &addr_len) == 0){
            local_port = ntohs(addr_info.sin_port);
        }
        return local_port;
    }

    std::string get_local_addr(void) const {
        // Behold the beauty of winsock
        struct sockaddr_in addr_info;
        int addr_len = sizeof(addr_info);
        std::string local_addr;
        if (getsockname(_sock_fd, (SOCKADDR*) &addr_info, &addr_len) == 0) {
            // inet_ntoa() guarantees either NULL or null-terminated array
            char *local_ip = inet_ntoa(addr_info.sin_addr);
            if (local_ip) {
                local_addr = std::string(local_ip);
            }
        }
        return local_addr;
    }

    //! Read back the socket's buffer space reserved for receives
    size_t get_recv_buff_size(void) {
        int recv_buff_size = 0;
        int opt_len = sizeof(recv_buff_size);
        getsockopt(
                _sock_fd,
                SOL_SOCKET,
                SO_RCVBUF,
                (char *)&recv_buff_size,
                (int *)&opt_len
        );

        return (size_t) recv_buff_size;
    }

    //! Read back the socket's buffer space reserved for sends
    size_t get_send_buff_size(void) {
        int send_buff_size = 0;
        int opt_len = sizeof(send_buff_size);
        getsockopt(
                _sock_fd,
                SOL_SOCKET,
                SO_SNDBUF,
                (char *)&send_buff_size,
                (int *)&opt_len
        );

        return (size_t) send_buff_size;
    }

private:
    //memory management -> buffers and fifos
    const size_t _recv_frame_size, _num_recv_frames;
    const size_t _send_frame_size, _num_send_frames;
    buffer_pool::sptr _recv_buffer_pool, _send_buffer_pool;
    std::vector<boost::shared_ptr<udp_zero_copy_asio_msb> > _msb_pool;
    std::vector<boost::shared_ptr<udp_zero_copy_asio_mrb> > _mrb_pool;
    size_t _next_recv_buff_index, _next_send_buff_index;

    //socket guts
    SOCKET                  _sock_fd;
};

/***********************************************************************
 * UDP zero copy make function
 **********************************************************************/
void check_usr_buff_size(
    size_t actual_buff_size,
    size_t user_buff_size, // Set this to zero for no user-defined preference
    const std::string tx_rx
){
    UHD_LOGGER_DEBUG("UDP")
        << boost::format("Target/actual %s sock buff size: %d/%d bytes")
           % tx_rx
           % user_buff_size
           % actual_buff_size
    ;
    if ((user_buff_size != 0.0) and (actual_buff_size < user_buff_size)) UHD_LOGGER_WARNING("UDP") << boost::format(
        "The %s buffer could not be resized sufficiently.\n"
        "Target sock buff size: %d bytes.\n"
        "Actual sock buff size: %d bytes.\n"
        "See the transport application notes on buffer resizing.\n"
    ) % tx_rx % user_buff_size % actual_buff_size;
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

    //extract buffer size hints from the device addr and check if they match up
    size_t usr_recv_buff_size = size_t(hints.cast<double>("recv_buff_size", 0.0));
    size_t usr_send_buff_size = size_t(hints.cast<double>("send_buff_size", 0.0));
    if (hints.has_key("recv_buff_size")) {
        if (usr_recv_buff_size < xport_params.recv_frame_size * xport_params.num_recv_frames) {
            throw uhd::value_error((boost::format(
                "recv_buff_size must be equal to or greater than (num_recv_frames * recv_frame_size) where num_recv_frames=%d, recv_frame_size=%d")
                % xport_params.num_recv_frames % xport_params.recv_frame_size).str());
        }
    }
    if (hints.has_key("send_buff_size")) {
        if (usr_send_buff_size < xport_params.send_frame_size * xport_params.num_send_frames) {
            throw uhd::value_error((boost::format(
                "send_buff_size must be equal to or greater than (num_send_frames * send_frame_size) where num_send_frames=%d, send_frame_size=%d")
                % xport_params.num_send_frames % xport_params.send_frame_size).str());
        }
    }

    udp_zero_copy_wsa_impl::sptr udp_trans(
        new udp_zero_copy_wsa_impl(addr, port, xport_params, hints)
    );

    // Read back the actual socket buffer sizes
    buff_params_out.recv_buff_size = udp_trans->get_recv_buff_size();
    buff_params_out.send_buff_size = udp_trans->get_send_buff_size();
    check_usr_buff_size(buff_params_out.recv_buff_size, usr_recv_buff_size, "recv");
    check_usr_buff_size(buff_params_out.send_buff_size, usr_send_buff_size, "send");

    return udp_trans;
}
