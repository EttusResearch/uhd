//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/types/metadata.hpp>
#include <boost/lockfree/queue.hpp>

namespace uhd { namespace rfnoc {

/*!
 *  Implements queue of async messages originating from the tx data transport
 *  and from the rfnoc graph.
 */
class tx_async_msg_queue
{
public:
    using sptr = std::shared_ptr<tx_async_msg_queue>;

    //! Constructor
    tx_async_msg_queue(size_t capacity);

    /*!
     *  Retrieve async message from queue
     *
     * \param async_metadata the metadata to be filled in
     * \param timeout_ms the timeout in milliseconds to wait for a message
     * \return true when the async_metadata is valid, false for timeout
     */
    bool recv_async_msg(async_metadata_t& async_metadata, int32_t timeout_ms);

    /*!
     *  Push an async message onto the queue
     *
     * \param async_metadata the metadata to be pushed
     */
    void enqueue(const async_metadata_t& async_metadata);

private:
    boost::lockfree::queue<async_metadata_t> _queue;
};

}} // namespace uhd::rfnoc
