//
// Copyright 2017 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <rpc/client.h>
#include <rpc/rpc_error.h>
#include <boost/format.hpp>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace {

constexpr uint64_t DEFAULT_RPC_TIMEOUT_MS = 2000;

bool await_rpc_connected_state(
    std::shared_ptr<rpc::client> client, std::chrono::milliseconds timeout)
{
    const auto timeout_time             = std::chrono::steady_clock::now() + timeout;
    rpc::client::connection_state state = client->get_connection_state();
    while (state == rpc::client::connection_state::initial
           and std::chrono::steady_clock::now() < timeout_time) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        state = client->get_connection_state();
    }

    // guarantee at least one check at the end of timeout
    if (state == rpc::client::connection_state::initial) {
        state = client->get_connection_state();
    }

    return state == rpc::client::connection_state::connected;
}
} // namespace

namespace uhd {


/*! Abstraction for RPC client
 *
 * Purpose of this class is to wrap the underlying RPC implementation.
 * This class holds a connection to an RPC server (the connection is severed on
 * destruction).
 */
class rpc_client
{
public:
    using sptr = std::shared_ptr<rpc_client>;

    static sptr make(const std::string& addr,
        const uint16_t port,
        const uint64_t timeout_ms             = DEFAULT_RPC_TIMEOUT_MS,
        const std::string& get_last_error_cmd = "")
    {
        return std::make_shared<rpc_client>(addr, port, timeout_ms, get_last_error_cmd);
    }

    /*!
     * \param addr An IP address to connect to
     * \param port Port to connect to
     * \param get_last_error_cmd A command that queries an error string from
     *                           the RPC server. If set, the RPC client will
     *                           try and use this command to fetch information
     *                           about what went wrong on the client side.
     */
    rpc_client(const std::string& addr,
        const uint16_t port,
        const uint64_t timeout_ms             = DEFAULT_RPC_TIMEOUT_MS,
        std::string const& get_last_error_cmd = "")
        : _get_last_error_cmd(get_last_error_cmd), _default_timeout_ms(timeout_ms)
    {
        _client = std::make_shared<rpc::client>(addr, port);

        // wait for asynchronous creation to finish
        if (!await_rpc_connected_state(
                _client, std::chrono::milliseconds(DEFAULT_RPC_TIMEOUT_MS))) {
            throw uhd::runtime_error(str(
                boost::format(
                    "Unknown error during attempt to establish RPC connection at %s:%d")
                % addr % port));
        }

        _client->set_timeout(_default_timeout_ms);
    }

    /*! Perform an RPC request.
     *
     * Thread safe (locked). This function blocks until it receives a valid
     * response from the server.
     *
     * \param func_name The function name that is called via RPC
     * \param args All these arguments are passed to the RPC call
     *
     * \throws uhd::runtime_error in case of failure
     */
    template <typename return_type, typename... Args>
    return_type request(std::string const& func_name, Args&&... args)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        try {
            return _client->call(func_name, std::forward<Args>(args)...)
                .template as<return_type>();
        } catch (const ::rpc::rpc_error& ex) {
            const std::string error = _get_last_error_safe();
            if (not error.empty()) {
                UHD_LOG_ERROR("RPC", error);
            }
            throw uhd::runtime_error(
                str(boost::format("Error during RPC call to `%s'. Error message: %s")
                    % func_name % (error.empty() ? ex.what() : error)));
        } catch (const std::bad_cast& ex) {
            throw uhd::runtime_error(
                str(boost::format("Error during RPC call to `%s'. Error message: %s")
                    % func_name % ex.what()));
        }
    };

    /*! Perform an RPC request.
     *
     * Thread safe (locked). This function blocks until it receives a valid
     * response from the server.
     *
     * \param timeout_ms is time limit for this RPC call.
     * \param func_name The function name that is called via RPC
     * \param args All these arguments are passed to the RPC call
     *
     * \throws uhd::runtime_error in case of failure
     */
    template <typename return_type, typename... Args>
    return_type request(uint64_t timeout_ms, std::string const& func_name, Args&&... args)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto holder = rpcc_timeout_holder(_client, timeout_ms, _default_timeout_ms);
        try {
            return _client->call(func_name, std::forward<Args>(args)...)
                .template as<return_type>();
        } catch (const ::rpc::rpc_error& ex) {
            const std::string error = _get_last_error_safe();
            if (not error.empty()) {
                UHD_LOG_ERROR("RPC", error);
            }
            throw uhd::runtime_error(
                str(boost::format("Error during RPC call to `%s'. Error message: %s")
                    % func_name % (error.empty() ? ex.what() : error)));
        } catch (const std::bad_cast& ex) {
            throw uhd::runtime_error(
                str(boost::format("Error during RPC call to `%s'. Error message: %s")
                    % func_name % ex.what()));
        }
    };


    /*! Perform an RPC notification.
     *
     * Thread safe (locked). This function does not require a response from the
     * server, although the underlying implementation may provide one.
     *
     * \param timeout_ms is time limit for this RPC call.
     * \param func_name The function name that is called via RPC
     * \param args All these arguments are passed to the RPC call
     *
     * \throws uhd::runtime_error in case of failure
     */
    template <typename... Args>
    void notify(uint64_t timeout_ms, std::string const& func_name, Args&&... args)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto holder = rpcc_timeout_holder(_client, timeout_ms, _default_timeout_ms);
        try {
            _client->call(func_name, std::forward<Args>(args)...);
        } catch (const ::rpc::rpc_error& ex) {
            const std::string error = _get_last_error_safe();
            if (not error.empty()) {
                UHD_LOG_ERROR("RPC", error);
            }
            throw uhd::runtime_error(
                str(boost::format("Error during RPC call to `%s'. Error message: %s")
                    % func_name % (error.empty() ? ex.what() : error)));
        } catch (const std::bad_cast& ex) {
            throw uhd::runtime_error(
                str(boost::format("Error during RPC call to `%s'. Error message: %s")
                    % func_name % ex.what()));
        }
    };

    /*! Perform an RPC notification.
     *
     * Thread safe (locked). This function does not require a response from the
     * server, although the underlying implementation may provide one.
     *
     * \param func_name The function name that is called via RPC
     * \param args All these arguments are passed to the RPC call
     *
     * \throws uhd::runtime_error in case of failure
     */
    template <typename... Args>
    void notify(std::string const& func_name, Args&&... args)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        try {
            _client->call(func_name, std::forward<Args>(args)...);
        } catch (const ::rpc::rpc_error& ex) {
            const std::string error = _get_last_error_safe();
            if (not error.empty()) {
                UHD_LOG_ERROR("RPC", error);
            }
            throw uhd::runtime_error(
                str(boost::format("Error during RPC call to `%s'. Error message: %s")
                    % func_name % (error.empty() ? ex.what() : error)));
        } catch (const std::bad_cast& ex) {
            throw uhd::runtime_error(
                str(boost::format("Error during RPC call to `%s'. Error message: %s")
                    % func_name % ex.what()));
        }
    };

    /*! Like request(), also provides a token.
     *
     * This is a convenience wrapper to directly call a function that requires
     * a token without having to have a copy of the token.
     */
    template <typename return_type, typename... Args>
    return_type request_with_token(std::string const& func_name, Args&&... args)
    {
        return request<return_type>(func_name, _token, std::forward<Args>(args)...);
    };

    /*! Like request_with_token(), but it can be specified different timeout than default.
     */
    template <typename return_type, typename... Args>
    return_type request_with_token(
        uint64_t timeout_ms, std::string const& func_name, Args&&... args)
    {
        return request<return_type>(
            timeout_ms, func_name, _token, std::forward<Args>(args)...);
    };

    /*! Like notify(), also provides a token.
     *
     * This is a convenience wrapper to directly call a function that requires
     * a token without having to have a copy of the token.
     */
    template <typename... Args>
    void notify_with_token(std::string const& func_name, Args&&... args)
    {
        notify(func_name, _token, std::forward<Args>(args)...);
    };

    /*! Like notify_with_token() but it can be specified different timeout than default.
     */
    template <typename... Args>
    void notify_with_token(
        uint64_t timeout_ms, std::string const& func_name, Args&&... args)
    {
        notify(timeout_ms, func_name, _token, std::forward<Args>(args)...);
    };

    /*! Sets the token value. This is used by the `_with_token` methods.
     */
    void set_token(const std::string& token)
    {
        _token = token;
    }

private:
    /*! This is internal object to hold timeout of the rpc client
     * it is used as an RAII in code block.
     */
    class rpcc_timeout_holder
    {
    public:
        rpcc_timeout_holder(std::shared_ptr<rpc::client> client,
            uint64_t set_timeout,
            uint64_t resume_timeout)
            : _rpcc(client), _save_timeout(resume_timeout)
        {
            _rpcc->set_timeout(set_timeout);
        }

        ~rpcc_timeout_holder()
        {
            _rpcc->set_timeout(_save_timeout);
        }

    private:
        std::shared_ptr<rpc::client> _rpcc;
        uint64_t _save_timeout;
    };

    /*! Pull the last error out of the RPC server. Not thread-safe, meant to
     * be called from notify() or request().
     *
     * This function will do its best not to get in anyone's way. If it can't
     * get an error string, it'll return an empty string.
     */
    std::string _get_last_error_safe()
    {
        if (_get_last_error_cmd.empty()) {
            return "";
        }
        try {
            return _client->call(_get_last_error_cmd).as<std::string>();
        } catch (const ::rpc::rpc_error&) {
            // nop
        } catch (const std::bad_cast&) {
            // nop
        } catch (...) {
            // nop
        }
        return "";
    }

    //! Reference the actual RPC client
    std::shared_ptr<rpc::client> _client;
    //! If set, this is the command that will retrieve an error
    const std::string _get_last_error_cmd;
    uint64_t _default_timeout_ms;
    std::string _token;
    std::mutex _mutex;
};

} /* namespace uhd */
