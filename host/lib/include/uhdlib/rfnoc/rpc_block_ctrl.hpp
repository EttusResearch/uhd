//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_RFNOC_RPC_BLOCK_CTRL_HPP
#define INCLUDED_LIBUHD_RFNOC_RPC_BLOCK_CTRL_HPP

#include <uhd/types/device_addr.hpp>
#include <uhdlib/utils/rpc.hpp>

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
