//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/types/sid.hpp>
#include <uhd/types/time_spec.hpp>
#include <vector>

namespace uhd { namespace rfnoc {

/*!
 * Async message.
 */
struct async_msg_t
{
    //! Has time?
    bool has_time_spec;

    //! When the async event occurred.
    time_spec_t time_spec;

    /*!
     * The type of event for a receive async message call.
     */
    enum event_code_t {
        //! Nothing happened.
        EVENT_CODE_NONE = 0x00,
        //! A burst was successfully transmitted.
        EVENT_CODE_BURST_ACK = 0x1,
        //! An internal send buffer has emptied.
        EVENT_CODE_UNDERFLOW = 0x2,
        //! Same. We use the terms 'underrun' and 'underflow' interchangeably.
        EVENT_CODE_UNDERRUN = EVENT_CODE_UNDERFLOW,
        //! Packet loss or reordering between source and destination,
        //  at start of burst (i.e. the first packet after an EOB packet
        //  had the wrong sequence number).
        EVENT_CODE_SEQ_ERROR = 0x4,
        //! Like EVENT_CODE_SEQ_ERROR, but within a burst (i.e., any packet
        //  other than the first packet had an error)
        EVENT_CODE_SEQ_ERROR_IN_BURST = 0x20,
        //! Data packet had time that was late.
        EVENT_CODE_LATE_DATA_ERROR = 0x8,
        //! Command packet had time that was late.
        EVENT_CODE_LATE_CMD_ERROR = 0x8,
        //! Packet is carrying arbitrary payload
        EVENT_CODE_USER_PAYLOAD = 0x40,

        // TODO: For now, we combine legacy TX and RX messages.
        EVENT_CODE_OVERFLOW = 0x8,
        EVENT_CODE_OVERRUN  = EVENT_CODE_OVERFLOW,
        //! Multi-channel alignment failed.
        EVENT_CODE_ALIGNMENT = 0xc,
        //! The packet could not be parsed.
        EVENT_CODE_BAD_PACKET = 0xf
    } event_code;

    /*!
     * A special payload populated by custom FPGA fabric.
     */
    std::vector<uint32_t> payload;

    //! The SID on the async message packet
    uint32_t sid;

    async_msg_t(const size_t payload_size = 4)
        : has_time_spec(false)
        , time_spec(0.0)
        , event_code(EVENT_CODE_NONE)
        , payload(payload_size, 0)
        , sid(0)
    {
    }
    //! Return the the id of src block that throw eror
    uint32_t get_error_src() const
    {
        return sid_t(sid).get_src_endpoint();
    }
};

}} // namespace uhd::rfnoc
