//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UTILS_RPC_HPP
#define INCLUDED_UTILS_RPC_HPP

#include <rpc/client.h>
#include <rpc/rpc_error.h>
#include <uhd/utils/log.hpp>
#include <uhd/exception.hpp>
#include <boost/format.hpp>

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

    static sptr make(
            const std::string &addr,
            const uint16_t port,
            const std::string &get_last_error_cmd=""
    ) {
        return std::make_shared<rpc_client>(addr, port, get_last_error_cmd);
    }

    /*!
     * \param addr An IP address to connect to
     * \param port Port to connect to
     * \param get_last_error_cmd A command that queries an error string from
     *                           the RPC server. If set, the RPC client will
     *                           try and use this command to fetch information
     *                           about what went wrong on the client side.
     */
    rpc_client(
            const std::string &addr,
            const uint16_t port,
            std::string const &get_last_error_cmd=""
    ) : _client(addr, port)
      , _get_last_error_cmd(get_last_error_cmd)
    {
        // nop
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
            return _client.call(func_name, std::forward<Args>(args)...)
                .template as<return_type>();
        } catch (const ::rpc::rpc_error &ex) {
            const std::string error = _get_last_error_safe();
            if (not error.empty()) {
                UHD_LOG_ERROR("RPC", error);
            }
            throw uhd::runtime_error(str(
                boost::format("Error during RPC call to `%s'. Error message: %s")
                % func_name % (error.empty() ? ex.what() : error)
            ));
        } catch (const std::bad_cast& ex) {
            throw uhd::runtime_error(str(
                boost::format("Error during RPC call to `%s'. Error message: %s")
                % func_name % ex.what()
            ));
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
            _client.call(func_name, std::forward<Args>(args)...);
        } catch (const ::rpc::rpc_error &ex) {
            const std::string error = _get_last_error_safe();
            if (not error.empty()) {
                UHD_LOG_ERROR("RPC", error);
            }
            throw uhd::runtime_error(str(
                boost::format("Error during RPC call to `%s'. Error message: %s")
                % func_name % (error.empty() ? ex.what() : error)
            ));
        } catch (const std::bad_cast& ex) {
            throw uhd::runtime_error(str(
                boost::format("Error during RPC call to `%s'. Error message: %s")
                % func_name % ex.what()
            ));
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

    /*! Sets the token value. This is used by the `_with_token` methods.
     */
    void set_token(const std::string &token)
    {
        _token = token;
    }

    void set_timeout(size_t timeout_ms)
    {
        _client.set_timeout(timeout_ms);
    }

  private:
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
            return _client.call(_get_last_error_cmd).as<std::string>();
        } catch (const ::rpc::rpc_error &ex) {
            // nop
        } catch (const std::bad_cast& ex) {
            // nop
        } catch (...) {
            // nop
        }
        return "";
    }

    //! Reference the actual RPC client
    ::rpc::client _client;
    //! If set, this is the command that will retrieve an error
    const std::string _get_last_error_cmd;

    std::string _token;
    std::mutex _mutex;
};

} /* namespace uhd */

#endif /* INCLUDED_UTILS_RPC_HPP */
