//
// Copyright 2017 Ettus Research (National Instruments)
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

#ifndef INCLUDED_UTILS_RPC_HPP
#define INCLUDED_UTILS_RPC_HPP

#include <rpc/client.h>
#include <rpc/rpc_error.h>
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

    static sptr make(std::string const& addr, uint16_t port) {
        return std::make_shared<rpc_client>(addr, port);
    }

    /*!
     * \param addr An IP address to connect to
     * \param port Port to connect to
     */
    rpc_client(std::string const& addr, uint16_t port) : _client(addr, port) {}

    /*! Perform an RPC call.
     *
     * Thread safe (locked).
     *
     * \param func_name The function name that is called via RPC
     * \param args All these arguments are passed to the RPC call
     */
    template <typename return_type, typename... Args>
    return_type call(std::string const& func_name, Args&&... args)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        try {
            return _client.call(func_name, std::forward<Args>(args)...)
                .template as<return_type>();
        } catch (const ::rpc::rpc_error &ex) {
            throw uhd::runtime_error(str(
                boost::format("Error during RPC call to `%s'. Error message: %s")
                % func_name % ex.what()
            ));
        } catch (const std::bad_cast& ex) {
            throw uhd::runtime_error(str(
                boost::format("Error during RPC call to `%s'. Error message: %s")
                % func_name % ex.what()
            ));
        }
    };

    template <typename... Args>
    void call(std::string const& func_name, Args&&... args)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        try {
            _client.call(func_name, std::forward<Args>(args)...);
        } catch (const ::rpc::rpc_error &ex) {
            throw uhd::runtime_error(str(
                boost::format("Error during RPC call to `%s'. Error message: %s")
                % func_name % ex.what()
            ));
        } catch (const std::bad_cast& ex) {
            throw uhd::runtime_error(str(
                boost::format("Error during RPC call to `%s'. Error message: %s")
                % func_name % ex.what()
            ));
        }
    };

    /*! Perform an RPC call; also includes a token.
     *
     * The first argument to the actual RPC function call is the current token
     * value. To set a token value, call set_token()
     */
    template <typename return_type, typename... Args>
    return_type call_with_token(std::string const& func_name, Args&&... args)
    {
        return call<return_type>(func_name, _token, std::forward<Args>(args)...);
    };

    /*! Sets the token value. This is used by call_with_token().
     */
    void set_token(const std::string &token)
    {
        _token = token;
    }

  private:
    std::string _token;
    std::mutex _mutex;
    ::rpc::client _client;
};

}

#endif /* INCLUDED_UTILS_RPC_HPP */
