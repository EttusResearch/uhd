//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/format.hpp>

namespace uhd { namespace transport {

// Number of iterations that get_aligned_buffs will attempt to time align
// packets before returning an alignment failure. get_aligned_buffs increments
// the iteration count when it finds a timestamp that is larger than the
// timestamps on channels it has already aligned and thus has to restart
// aligning timestamps on all channels to the new timestamp.
constexpr size_t ALIGNMENT_FAILURE_THRESHOLD = 1000;

/*!
 * Implementation of rx time alignment. This method reads packets from the
 * transports for each channel and discards any packets whose tsf does not
 * match those of other channels due to dropped packets. Packets that do not
 * have a tsf are not checked for alignment and never dropped.
 */
template <typename transport_t, bool ignore_seq_err = false>
class get_aligned_buffs
{
public:
    enum alignment_result_t {
        SUCCESS,
        TIMEOUT,
        SEQUENCE_ERROR,
        ALIGNMENT_FAILURE,
        BAD_PACKET
    };

    get_aligned_buffs(std::vector<typename transport_t::uptr>& xports,
        std::vector<typename transport_t::buff_t::uptr>& frame_buffs,
        std::vector<typename transport_t::packet_info_t>& infos)
        : _xports(xports)
        , _frame_buffs(frame_buffs)
        , _infos(infos)
        , _prev_tsf(_xports.size(), 0)
        , _channels_to_align(_xports.size())
    {
    }

    alignment_result_t operator()(const int32_t timeout_ms)
    {
        // Clear state
        _channels_to_align.set();
        bool time_valid   = false;
        uint64_t tsf      = 0;
        size_t iterations = 0;

        while (_channels_to_align.any()) {
            const size_t chan = _channels_to_align.find_first();
            auto& xport       = _xports[chan];
            auto& info        = _infos[chan];
            auto& frame_buff  = _frame_buffs[chan];
            bool seq_error    = false;

            // Receive a data packet for the channel if we don't have one. A
            // packet may already be there if the previous call was interrupted
            // by an error.
            if (!frame_buff) {
                try {
                    std::tie(frame_buff, info, seq_error) =
                        xport->get_recv_buff(timeout_ms);
                } catch (const uhd::value_error& e) {
                    // Bad packet
                    UHD_LOGGER_ERROR("STREAMER")
                        << boost::format(
                               "The receive transport caught a value exception.\n%s")
                               % e.what();
                    return BAD_PACKET;
                }
            }

            if (!frame_buff) {
                return TIMEOUT;
            }

            if (info.has_tsf) {
                const bool time_out_of_order = _prev_tsf[chan] > info.tsf;
                _prev_tsf[chan]              = info.tsf;

                // If the user changes the device time while streaming, we can
                // receive a packet that comes before the previous packet in
                // time. This would cause the alignment logic to discard future
                // received packets. Therefore, when this occurs, we reset the
                // info to restart the alignment.
                if (time_out_of_order) {
                    time_valid = false;
                }

                // Check if the time is larger than packets received for other
                // channels, and if so, use this time to align all channels
                if (!time_valid || info.tsf > tsf) {
                    // If we haven't found a set of aligned packets after many
                    // iterations, return an alignment failure
                    if (iterations++ > ALIGNMENT_FAILURE_THRESHOLD) {
                        UHD_LOGGER_ERROR("STREAMER")
                            << "The rx streamer failed to time-align packets.";
                        return ALIGNMENT_FAILURE;
                    }

                    // Release buffers for channels aligned previously. Keep
                    // buffers that don't have a tsf since we don't need to
                    // align those.
                    for (size_t i = 0; i < _xports.size(); i++) {
                        if (!_channels_to_align.test(i) && _infos[i].has_tsf) {
                            _xports[i]->release_recv_buff(std::move(_frame_buffs[i]));
                            _frame_buffs[i] = nullptr;
                        }
                    }

                    // Mark only this channel as aligned and save its tsf
                    _channels_to_align.set();
                    _channels_to_align.reset(chan);
                    time_valid = true;
                    tsf        = info.tsf;
                }

                // Check if the time matches that of other aligned channels
                else if (info.tsf == tsf) {
                    _channels_to_align.reset(chan);
                }

                // Otherwise, time is smaller than other channels, release the buffer
                else {
                    _xports[chan]->release_recv_buff(std::move(_frame_buffs[chan]));
                    _frame_buffs[chan] = nullptr;
                }
            } else {
                // Packet doesn't have a tsf, just mark it as aligned
                _channels_to_align.reset(chan);
            }

            // If this packet had a sequence error, stop to return the error.
            // Keep the packet for the next call to get_aligned_buffs.
            if (seq_error && !ignore_seq_err) {
                UHD_LOG_FASTPATH("D");
                return SEQUENCE_ERROR;
            }
        }

        // All channels aligned
        return SUCCESS;
    }

private:
    // Transports for each channel
    std::vector<typename transport_t::uptr>& _xports;

    // Storage for buffers resulting from alignment
    std::vector<typename transport_t::buff_t::uptr>& _frame_buffs;

    // Packet info corresponding to aligned buffers
    std::vector<typename transport_t::packet_info_t>& _infos;

    // Time of previous packet for each channel
    std::vector<uint64_t> _prev_tsf;

    // Keeps track of channels that are aligned
    boost::dynamic_bitset<> _channels_to_align;
};

}} // namespace uhd::transport
