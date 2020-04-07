//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/utils/log.hpp>
#include <uhdlib/rfnoc/rfnoc_common.hpp>

namespace uhd { namespace rfnoc {

//! Class to manage rx flow control state
class rx_flow_ctrl_state
{
public:
    //! Constructor
    rx_flow_ctrl_state(
        const rfnoc::sep_id_pair_t epids, const stream_buff_params_t fc_freq)
        : _fc_freq(fc_freq), _epids(epids)
    {
    }

    //! Resynchronize with transfer counts from the sender
    void resynchronize(const stream_buff_params_t counts)
    {
        if (_recv_counts.bytes != counts.bytes
            || _recv_counts.packets != counts.packets) {
            // If there is a discrepancy between the amount of data sent by
            // the device and received by the transport, adjust the counts
            // of data received and transferred to include the dropped data.
            auto bytes_dropped = counts.bytes - _recv_counts.bytes;
            auto pkts_dropped  = counts.packets - _recv_counts.packets;
            _xfer_counts.bytes += bytes_dropped;
            _xfer_counts.packets += pkts_dropped;

            UHD_LOGGER_DEBUG("rx_flow_ctrl_state")
                << "Flow control state mismatch: bytes reported: " << counts.bytes
                << " bytes counted locally: " << _recv_counts.bytes
                << " delta: " << (counts.bytes - _recv_counts.bytes)
                << " Packets reported: " << counts.packets
                << " Packets counted locally: " << _recv_counts.packets
                << " delta: " << (counts.packets - _recv_counts.packets)
                << " src_epid=" << _epids.first << " dst_epid=" << _epids.second
                << std::endl;

            _recv_counts = counts;
        }
    }

    //! Update state when data is received
    void data_received(const size_t bytes)
    {
        _recv_counts.bytes += bytes;
        _recv_counts.packets++;
    }

    //! Update state when transfer is complete (buffer space freed)
    void xfer_done(const size_t bytes)
    {
        _xfer_counts.bytes += bytes;
        _xfer_counts.packets++;
    }

    //! Returns whether a flow control response is needed
    bool fc_resp_due() const
    {
        stream_buff_params_t accum_counts = {
            _xfer_counts.bytes - _last_fc_resp_counts.bytes,
            _xfer_counts.packets - _last_fc_resp_counts.packets};

        return accum_counts.bytes >= _fc_freq.bytes
               || accum_counts.packets >= _fc_freq.packets;
    }

    //! Update state after flow control response was sent
    void fc_resp_sent()
    {
        _last_fc_resp_counts = _xfer_counts;
    }

    //! Returns counts for completed transfers
    stream_buff_params_t get_xfer_counts() const
    {
        return _xfer_counts;
    }

    //! Returns counts for completed transfers
    stream_buff_params_t get_recv_counts() const
    {
        return _recv_counts;
    }

    //! Returns configured flow control frequency
    stream_buff_params_t get_fc_freq() const
    {
        return _fc_freq;
    }

private:
    // Counts for data received, including any data still in use
    stream_buff_params_t _recv_counts{0, 0};

    // Counts for data read and whose buffer space is ok to reuse
    stream_buff_params_t _xfer_counts{0, 0};

    // Counts sent in last flow control response
    stream_buff_params_t _last_fc_resp_counts{0, 0};

    // Frequency of flow control responses
    stream_buff_params_t _fc_freq{0, 0};

    // Endpoint ID for log messages
    const sep_id_pair_t _epids;
};

}} // namespace uhd::rfnoc
