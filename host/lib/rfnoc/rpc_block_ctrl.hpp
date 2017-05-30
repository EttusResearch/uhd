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

#ifndef INCLUDED_LIBUHD_RFNOC_RPC_BLOCK_CTRL_HPP
#define INCLUDED_LIBUHD_RFNOC_RPC_BLOCK_CTRL_HPP

#include "../utils/rpc.hpp"

namespace uhd {
    namespace rfnoc {

/*! Abstraction for RPC client
 *
 * Purpose of this class is to wrap the underlying RPC implementation.
 * This class holds a connection to an RPC server (the connection is severed on
 * destruction).
 */
class rpc_block_ctrl
{
public:
    virtual ~rpc_block_ctrl() {}

    /*! Pass in an RPC client for the block to use
     *
     * \param rpcc Reference to the RPC client
     * \param block_args Additional block arguments
     */
    virtual void set_rpc_client(
        uhd::rpc_client::sptr rpcc,
        const uhd::device_addr_t &block_args
    ) = 0;

};

}}

#endif /* INCLUDED_LIBUHD_RFNOC_RPC_BLOCK_CTRL_HPP */
