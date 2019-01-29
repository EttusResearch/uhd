//
// Copyright 2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_RFNOC_AYNC_MSG_HANDLER_HPP
#define INCLUDED_LIBUHD_RFNOC_AYNC_MSG_HANDLER_HPP

#include <uhd/rfnoc/graph.hpp>
#include <uhd/transport/zero_copy.hpp>
#include <uhd/types/sid.hpp>
#include <uhd/types/endianness.hpp>
#include <uhdlib/rfnoc/async_msg.hpp>
#include <uhd/utils/noncopyable.hpp>
#include <functional>

namespace uhd { namespace rfnoc {

/*! Async message handler for a uhd::rfnoc::graph
 *
 */
class async_msg_handler : uhd::noncopyable
{
public:
    typedef boost::shared_ptr<async_msg_handler> sptr;
    typedef std::function<void(const async_msg_t&)> async_handler_type;

    /*!
     * \param recv A transport on which async messages are received
     * \param send A transport on which to send response messages
     * \param sid The source part of this is taken as the local address of the
     *            transports. The remote part is ignored.
     */
    static sptr make(
        uhd::transport::zero_copy_if::sptr recv,
        uhd::transport::zero_copy_if::sptr send,
        uhd::sid_t sid,
        uhd::endianness_t endianness
    );

    /*! Register an event handler.
     *
     * When any message is received with the given event code,
     * \p handler is called with the async message data as an argument.
     *
     * Note that \p handler is called if a message includes a certain event
     * code, but it does not have to be exclusive. Example: If there are two
     * event handlers registered, one for EVENT_CODE_OVERRUN and one for
     * EVENT_CODE_BAD_PACKET, and a message includes both those event codes,
     * then both event handlers are called.
     *
     * Multiple handlers per event code may be registered. The order they are
     * called in is non-deterministic.
     *
     * \returns The number of event handlers registered for this event code.
     *          Should never return anything less than 1.
     */
    virtual int register_event_handler(
            const async_msg_t::event_code_t event_code,
            async_handler_type handler
    ) = 0;

    /*! Post async messages into this message handler.
     *
     * This is the entry point for all async messages. When a message
     * is posted here, the following actions take place:
     * - If applicable, an event handler is called with \p metadata as the
     *   argument
     * - Some messages print error codes (e.g. O, U, L, S)
     */
    virtual void post_async_msg(
            const async_msg_t &metadata
    ) = 0;

    /*! Return the 16-bit address of this async message
    */
    virtual uint32_t get_local_addr() const = 0;
};


}}; /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_AYNC_MSG_HANDLER_HPP */
