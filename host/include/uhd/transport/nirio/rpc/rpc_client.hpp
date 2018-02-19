//
// Copyright 2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_RPC_CLIENT_HPP
#define INCLUDED_RPC_CLIENT_HPP

#include <boost/asio.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/condition_variable.hpp>
#include "rpc_common.hpp"
#include <uhd/utils/log.hpp>

namespace uhd { namespace usrprio_rpc {

class rpc_client : private boost::noncopyable
{
public:
    static const uint32_t CURRENT_VERSION = 1;
    static const uint32_t OLDEST_COMPATIBLE_VERSION = 1;

    rpc_client(
        const std::string& server,
        const std::string& port,
        uint32_t process_id,
        uint32_t host_id);
    ~rpc_client();

    const boost::system::error_code& call(
        func_id_t func_id,
        const func_args_writer_t& in_args,
        func_args_reader_t &out_args,
        boost::posix_time::milliseconds timeout);

    inline const boost::system::error_code& status() const {
        return _exec_err;
    }

    /* [Possible boost::system::error_code values]
     * boost::asio::error::connection_aborted: Network connection failure
     * boost::asio::error::eof:                Network connection closed cleanly
     * boost::asio::error::connection_refused: Software handshake failure (version mismatch, etc)
     * boost::asio::error::timed_out:          RPC call timed out
     * boost::asio::error::operation_aborted:  RPC call aborted but connection alive
     */

private:
    void _handle_response_hdr(const boost::system::error_code& err, size_t transferred, size_t expected);
    void _handle_response_data(const boost::system::error_code& err, size_t transferred, size_t expected);
    void _wait_for_next_response_header();

    inline void _stop_io_service() {
        if (_io_service_thread.get()) {
            UHD_LOGGER_INFO("NIRIO") << "rpc_client stopping...";
            _io_service.stop();
            _io_service_thread->join();
            _io_service_thread.reset();
            UHD_LOGGER_INFO("NIRIO") << "rpc_client stopped.";
        }
    }

    //Services
    boost::asio::io_service             _io_service;
    boost::scoped_ptr<boost::thread>    _io_service_thread;
    boost::asio::ip::tcp::socket        _socket;
    //Handshake info
    hshake_args_t                       _hshake_args_client;
    hshake_args_t                       _hshake_args_server;
    //In-out function args
    func_xport_buf_t                    _request;
    func_xport_buf_t                    _response;
    //Synchronization
    boost::mutex                        _mutex;
    boost::condition_variable           _exec_gate;
    boost::system::error_code           _exec_err;
};

}}

#endif /* INCLUDED_RPC_CLIENT_HPP */
