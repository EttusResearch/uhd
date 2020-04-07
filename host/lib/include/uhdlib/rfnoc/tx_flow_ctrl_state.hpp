//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhdlib/rfnoc/rfnoc_common.hpp>

namespace uhd { namespace rfnoc {

//! Class to manage tx flow control state
class tx_flow_ctrl_state
{
public:
    //! Constructor
    tx_flow_ctrl_state(const stream_buff_params_t& capacity) : _dest_capacity(capacity) {}

    //! Updates destination received count
    void update_dest_recv_count(const stream_buff_params_t& recv_count)
    {
        _recv_counts = recv_count;
    }

    /*! Returns whether the destination has buffer space for the requested
     *  packet size
     */
    bool dest_has_space(const size_t packet_size)
    {
        // The stream endpoint only cares about bytes, the packet count is not
        // important to determine the space available.
        const auto buffer_fullness = _xfer_counts.bytes - _recv_counts.bytes;
        const auto space_available = _dest_capacity.bytes - buffer_fullness;
        return space_available >= packet_size;
    }

    //! Increments transfer count with amount of data sent
    void data_sent(const size_t packet_size)
    {
        _xfer_counts.bytes += packet_size;
        _xfer_counts.packets++;

        // Request an fc resync after we have transferred a number of bytes >=
        // to the destination capacity. There is no strict requirement on how
        // often we need to send this, as it is only needed to correct for
        // dropped packets. One buffer's worth of bytes is probably a good
        // cadence.
        if (_xfer_counts.bytes - _last_fc_resync_bytes >= _dest_capacity.bytes) {
            _fc_resync_req = true;
        }
    }

    /*! Returns whether an fc resync request is pending. The policy we use
     * here is to send an fc resync every time we send a number of bytes
     * equal to the destination buffer capacity.
     */
    bool get_fc_resync_req_pending() const
    {
        return _fc_resync_req;
    }

    //! Clears fc resync request pending status
    void clear_fc_resync_req_pending()
    {
        _fc_resync_req        = false;
        _last_fc_resync_bytes = _xfer_counts.bytes;
    }

    //! Returns counts for packets sent
    stream_buff_params_t get_xfer_counts() const
    {
        return _xfer_counts;
    }

private:
    // Counts for data sent
    stream_buff_params_t _xfer_counts{0, 0};

    // Counts for data received by the destination
    stream_buff_params_t _recv_counts{0, 0};

    // Buffer size at the destination
    stream_buff_params_t _dest_capacity{0, 0};

    // Counts sent in last flow control resync
    size_t _last_fc_resync_bytes = 0;

    // Track when to send ack packets
    bool _fc_resync_req = false;
};

}} // namespace uhd::rfnoc
