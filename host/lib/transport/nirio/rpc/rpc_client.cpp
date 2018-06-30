///
// Copyright 2013,2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/transport/nirio/rpc/rpc_client.hpp>
#include <boost/bind.hpp>
#include <boost/version.hpp>
#include <boost/format.hpp>
#include <boost/asio/error.hpp>

#define CHAIN_BLOCKING_XFER(func, exp, status) \
    if (status) { \
        status = (static_cast<size_t>((func)) == exp); \
    } else { \
        UHD_LOGGER_DEBUG("NIRIO") << "rpc_client operation skipped: " #func "\n"; \
    } \

namespace uhd { namespace usrprio_rpc {

using boost::asio::ip::tcp;

rpc_client::rpc_client (
    const std::string& server,
    const std::string& port,
    uint32_t process_id,
    uint32_t host_id
) : _socket(_io_service)
{
    //Fill in handshake info
    _hshake_args_client.version = CURRENT_VERSION;
    _hshake_args_client.oldest_comp_version = OLDEST_COMPATIBLE_VERSION;
    _hshake_args_client.client_id = build_client_id(host_id, process_id);
    _hshake_args_client.boost_archive_version = boost_serialization_archive_utils::get_version();

    try {
        //Synchronous resolve + connect
        tcp::resolver resolver(_io_service);
        //Create flags object with all special flags disabled. Especially the following:
        //- address_configured: Only return addresses if a non-loopback address is configured for the system.
        //- numeric_host: No name resolution should be attempted for host
        //- numeric_service: No name resolution should be attempted for service
        tcp::resolver::query::flags query_flags(tcp::resolver::query::passive);
        tcp::resolver::query query(tcp::v4(), server, port, query_flags);
        tcp::resolver::iterator iterator = resolver.resolve(query);
        boost::asio::connect(_socket, iterator);

        UHD_LOGGER_TRACE("NIRIO") << "rpc_client connected to server." ;

        try {
            //Perform handshake
            bool status = true;
            CHAIN_BLOCKING_XFER(
                boost::asio::write(_socket, boost::asio::buffer(&_hshake_args_client, sizeof(_hshake_args_client))),
                sizeof(_hshake_args_client), status);
            CHAIN_BLOCKING_XFER(
                boost::asio::read(_socket, boost::asio::buffer(&_hshake_args_server, sizeof(_hshake_args_server))),
                sizeof(_hshake_args_server), status);

            _request.header.client_id = _hshake_args_server.client_id;

            if (_hshake_args_server.version >= _hshake_args_client.oldest_comp_version &&
                _hshake_args_client.version >= _hshake_args_server.oldest_comp_version &&
                status)
            {
                UHD_LOGGER_TRACE("NIRIO") << "rpc_client bound to server." ;
                _wait_for_next_response_header();

                //Spawn a thread for the io_service callback handler. This thread will run until rpc_client is destroyed.
                _io_service_thread.reset(new boost::thread(boost::bind(&boost::asio::io_service::run, &_io_service)));
            } else {
                UHD_LOGGER_DEBUG("NIRIO") << "rpc_client handshake failed." ;
                _exec_err.assign(boost::asio::error::connection_refused, boost::asio::error::get_system_category());
            }
            UHD_LOGGER_TRACE("NIRIO") << boost::format("rpc_client archive = %d, rpc_server archive = %d\n.") %
                _hshake_args_client.boost_archive_version %
                _hshake_args_server.boost_archive_version;
        } catch (boost::exception&) {
            UHD_LOGGER_DEBUG("NIRIO") << "rpc_client handshake aborted." ;
            _exec_err.assign(boost::asio::error::connection_refused, boost::asio::error::get_system_category());
        }
    } catch (boost::exception&) {
        UHD_LOGGER_DEBUG("NIRIO") << "rpc_client connection request cancelled/aborted." ;
        _exec_err.assign(boost::asio::error::connection_aborted, boost::asio::error::get_system_category());
    }
}

rpc_client::~rpc_client () {
    _stop_io_service();
}

const boost::system::error_code& rpc_client::call(
    func_id_t func_id,
    const func_args_writer_t& in_args,
    func_args_reader_t &out_args,
    boost::posix_time::milliseconds timeout
)
{
    boost::mutex::scoped_lock lock(_mutex);

    if (_io_service_thread.get()) {
        _request.header.func_id = func_id;
        in_args.store(_request.data);
        _request.header.func_args_size = _request.data.size();

        _exec_err.clear();

        //Send function call header and args
        bool status = true;
        try {
            CHAIN_BLOCKING_XFER(
                boost::asio::write(_socket, boost::asio::buffer(&_request.header, sizeof(_request.header))),
                sizeof(_request.header), status);
            if (not _request.data.empty())
            {
                CHAIN_BLOCKING_XFER(
                    boost::asio::write(_socket, boost::asio::buffer(&(*_request.data.begin()), _request.data.size())),
                    _request.data.size(), status);
            }
        } catch (boost::exception&) {
            status = false;
        }

        //Wait for response using condition variable
        if (status) {
            if (!_exec_gate.timed_wait(lock, timeout)) {
                UHD_LOGGER_DEBUG("NIRIO") << "rpc_client function timed out." ;
                _exec_err.assign(boost::asio::error::timed_out, boost::asio::error::get_system_category());
            }
        } else {
            UHD_LOGGER_DEBUG("NIRIO") << "rpc_client connection dropped." ;
            _exec_err.assign(boost::asio::error::connection_aborted, boost::asio::error::get_system_category());
            _stop_io_service();
        }

        //Verify that we are talking to the correct endpoint
        if ((_request.header.client_id != _response.header.client_id) && !_exec_err) {
            UHD_LOGGER_DEBUG("NIRIO") << "rpc_client confused about who its talking to." ;
            _exec_err.assign(boost::asio::error::operation_aborted, boost::asio::error::get_system_category());
        }

        if (!_exec_err) out_args.load(_response.data);
    }

    return _exec_err;
}

void rpc_client::_handle_response_hdr(const boost::system::error_code& err, size_t transferred, size_t expected)
{
    boost::mutex::scoped_lock lock(_mutex);
    _exec_err = err;
    if (!_exec_err && (transferred == expected)) {
        //Response header received. Verify that it is expected
        if (func_args_header_t::match_function(_request.header, _response.header)) {
            if (_response.header.func_args_size)
            {
                _response.data.resize(_response.header.func_args_size);

                //Wait for response data
                boost::asio::async_read(_socket,
                    boost::asio::buffer(&(*_response.data.begin()), _response.data.size()),
                    boost::bind(&rpc_client::_handle_response_data, this,
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred,
                        _response.data.size()));
            } else {
                _handle_response_data(err, 0, 0);
            }
        } else {
            //Unexpected response. Ignore it.
            UHD_LOGGER_DEBUG("NIRIO") << "rpc_client received garbage responses." ;
            _exec_err.assign(boost::asio::error::operation_aborted, boost::asio::error::get_system_category());

            _wait_for_next_response_header();
        }
    }

    if (_exec_err) _exec_gate.notify_all();
}

void rpc_client::_handle_response_data(const boost::system::error_code& err, size_t transferred, size_t expected)
{
    boost::mutex::scoped_lock lock(_mutex);
    _exec_err = err;
    if (transferred != expected) {
        _exec_err.assign(boost::asio::error::operation_aborted, boost::asio::error::get_system_category());
    }

    _exec_gate.notify_all();

    _wait_for_next_response_header();
}

void rpc_client::_wait_for_next_response_header() {
    //_mutex must be locked when this call is made
    boost::asio::async_read(
        _socket,
        boost::asio::buffer(&_response.header, sizeof(_response.header)),
        boost::bind(&rpc_client::_handle_response_hdr, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred,
            sizeof(_response.header)));
}

}}
