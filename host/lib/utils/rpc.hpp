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

    /*!
     * \param addr An IP address to connect to
     * \param port Port to connect to
     */
    rpc_client(std::string const& addr, uint16_t port) : _client(addr, port) {}

    /*! Perform an RPC call
     */
    template <typename return_type, typename... Args>
    return_type call(std::string const& func_name, Args&&... args)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _client.call(func_name, std::forward<Args>(args)...)
            .template as<return_type>();
    };

  private:
    std::mutex _mutex;
    ::rpc::client _client;
};

}

#endif /* INCLUDED_UTILS_RPC_HPP */
