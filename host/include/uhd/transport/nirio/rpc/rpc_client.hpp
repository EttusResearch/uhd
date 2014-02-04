//
// Copyright 2013 Ettus Research LLC
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
    static const boost::uint32_t CURRENT_VERSION = 1;
    static const boost::uint32_t OLDEST_COMPATIBLE_VERSION = 1;

    rpc_client(
        const std::string& server,
        const std::string& port,
        boost::uint32_t process_id,
        boost::uint32_t host_id);
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
            UHD_LOG << "rpc_client stopping..." << std::endl;
            _io_service.stop();
            _io_service_thread->join();
            _io_service_thread.reset();
            UHD_LOG << "rpc_client stopped." << std::endl;
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
