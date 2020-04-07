//
// Copyright 2019 Ettus Research, a National Instruments brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhdlib/transport/io_service.hpp>
#include <vector>

namespace uhd { namespace transport {

/*!
 * I/O service with offload thread
 *
 * Note: This I/O service can only be used with transports that allow releasing
 * frame buffers out of order, since flow control packets are handled entirely
 * within the offload thread.
 */
class offload_io_service : public io_service
{
public:
    enum client_type_t { RECV_ONLY, SEND_ONLY, BOTH_SEND_AND_RECV };

    enum wait_mode_t { POLL, BLOCK };

    /*!
     * Options for configuring offload I/O service
     */
    struct params_t
    {
        //! Array of CPU numbers to which to affinitize the offload thread.
        std::vector<size_t> cpu_affinity_list;
        //! The types of client that the I/O service needs to support.
        client_type_t client_type = BOTH_SEND_AND_RECV;
        //! The thread behavior when waiting for incoming packets If set to
        //! BLOCK, the client type must be set to either RECV_ONLY or SEND_ONLY.
        wait_mode_t wait_mode = POLL;
    };

    /*!
     * Creates an io service that offloads I/O to a worker thread and
     * passes configuration parameters to it.
     *
     *  \param io_srv The io service to perform the actual work in the worker
     *                thread.
     *  \param params Parameters to pass to the offload I/O service.
     *  \return A composite I/O service that executes the provided io service
     *          in its own thread.
     */
    static sptr make(io_service::sptr io_srv, const params_t& params);
};

}} // namespace uhd::transport
