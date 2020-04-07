//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/stream.hpp>
#include <uhd/types/metadata.hpp>
#include <vector>

namespace uhd { namespace transport {

/*!
 * Implementation of rx streamer manipulation of frame buffers and packet info.
 * This class is part of tx_streamer_impl, split into a separate unit as it is
 * a mostly self-contained portion of the streamer logic.
 */
template <typename transport_t>
class tx_streamer_zero_copy
{
public:
    //! Constructor
    tx_streamer_zero_copy(const size_t num_chans)
        : _xports(num_chans), _frame_buffs(num_chans)
    {
    }

    //! Connect a new channel to the streamer
    void connect_channel(const size_t port, typename transport_t::uptr xport)
    {
        if (port >= get_num_channels()) {
            throw uhd::index_error(
                "Port number indexes beyond the number of streamer ports");
        }

        if (_xports[port]) {
            throw uhd::runtime_error(
                "Streamer port number is already connected to a port");
        }

        _xports[port] = std::move(xport);
    }

    //! Returns number of channels handled by this streamer
    size_t get_num_channels() const
    {
        return _xports.size();
    }

    //! Returns the tick rate for conversion of timestamp
    double get_tick_rate() const
    {
        return _tick_rate;
    }

    //! Configures tick rate for conversion of timestamp
    void set_tick_rate(const double rate)
    {
        _tick_rate = rate;
    }

    //! Configures the size of each sample
    void set_bytes_per_item(const size_t bpi)
    {
        _bytes_per_item = bpi;
    }

    /*!
     * Gets a set of frame buffers, one per channel.
     *
     * \param buffs returns a pointer to the buffer data
     * \param nsamps_per_buff the number of samples that will be written to each buffer
     * \param metadata the metadata to write to the packet header
     * \param eov EOV flag to write to the packet header
     * \param timeout_ms timeout in milliseconds
     * \return true if the operation was sucessful, false if timeout occurs
     */
    UHD_FORCE_INLINE bool get_send_buffs(std::vector<void*>& buffs,
        const size_t nsamps_per_buff,
        const tx_metadata_t& metadata,
        const bool eov,
        const int32_t timeout_ms)
    {
        // Try to get a buffer per channel
        for (; _next_buff_to_get < _xports.size(); _next_buff_to_get++) {
            _frame_buffs[_next_buff_to_get].first =
                _xports[_next_buff_to_get]->get_send_buff(timeout_ms);

            if (!_frame_buffs[_next_buff_to_get].first) {
                return false;
            }
        }

        // Got all the buffers, start from index 0 next call
        _next_buff_to_get = 0;

        // Store portions of metadata we care about
        typename transport_t::packet_info_t info;
        info.has_tsf = metadata.has_time_spec;

        if (metadata.has_time_spec) {
            info.tsf = metadata.time_spec.to_ticks(_tick_rate);
        }

        info.payload_bytes = nsamps_per_buff * _bytes_per_item;
        info.eob           = metadata.end_of_burst;
        info.eov           = eov;

        // Write packet header
        for (size_t i = 0; i < buffs.size(); i++) {
            std::tie(buffs[i], _frame_buffs[i].second) =
                _xports[i]->write_packet_header(_frame_buffs[i].first, info);
        }

        return true;
    }

    /*!
     * Send the packet for the specified channel
     *
     * \param channel the channel for which to release the packet
     */
    UHD_FORCE_INLINE void release_send_buff(const size_t channel)
    {
        _frame_buffs[channel].first->set_packet_size(_frame_buffs[channel].second);
        _xports[channel]->release_send_buff(std::move(_frame_buffs[channel].first));

        _frame_buffs[channel].first  = nullptr;
        _frame_buffs[channel].second = 0;
    }

private:
    // Transports for each channel
    std::vector<typename transport_t::uptr> _xports;

    // Container to hold frame buffers for each channel and their packet sizes
    std::vector<std::pair<typename transport_t::buff_t::uptr, size_t>> _frame_buffs;

    // Rate used in conversion of timestamp to time_spec_t
    double _tick_rate = 1.0;

    // Size of a sample on the device
    size_t _bytes_per_item = 0;

    // Next channel from which to get a buffer, stored as a member to
    // allow the streamer to continue where it stopped due to timeouts.
    size_t _next_buff_to_get = 0;
};

}} // namespace uhd::transport
