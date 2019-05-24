//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_RX_STREAMER_ZERO_COPY_HPP
#define INCLUDED_LIBUHD_RX_STREAMER_ZERO_COPY_HPP

#include <uhd/config.hpp>
#include <uhd/exception.hpp>
#include <uhd/types/metadata.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/transport/get_aligned_buffs.hpp>
#include <boost/format.hpp>
#include <vector>

namespace uhd { namespace transport {

/*!
 * Implementation of rx streamer manipulation of frame buffers and packet info.
 * This class is part of rx_streamer_impl, split into a separate unit as it is
 * a mostly self-contained portion of the streamer logic.
 */
template <typename transport_t>
class rx_streamer_zero_copy
{
public:
    //! Constructor
    rx_streamer_zero_copy(const size_t num_ports)
        : _xports(num_ports)
        , _frame_buffs(num_ports)
        , _infos(num_ports)
        , _get_aligned_buffs(_xports, _frame_buffs, _infos)
    {
    }

    ~rx_streamer_zero_copy()
    {
        for (size_t i = 0; i < _frame_buffs.size(); i++) {
            if (_frame_buffs[i]) {
                _xports[i]->release_recv_buff(std::move(_frame_buffs[i]));
            }
        }
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

    //! Configures tick rate for conversion of timestamp
    void set_tick_rate(const double rate)
    {
        _tick_rate = rate;
    }

    //! Configures sample rate for conversion of timestamp
    void set_samp_rate(const double rate)
    {
        _samp_rate = rate;
    }

    //! Configures the size of each sample
    void set_bytes_per_item(const size_t bpi)
    {
        _bytes_per_item = bpi;
    }

    /*!
     * Gets a set of time-aligned buffers, one per channel.
     *
     * \param buffs returns a pointer to the buffer data
     * \param metadata returns the metadata corresponding to the buffer
     * \param timeout_ms timeout in milliseconds
     * \return the size in samples of each packet, or 0 if timeout
     */
    size_t get_recv_buffs(std::vector<const void*>& buffs,
        rx_metadata_t& metadata,
        const int32_t timeout_ms)
    {
        metadata.reset();

        switch (_get_aligned_buffs(timeout_ms)) {
            case get_aligned_buffs_t::SUCCESS:
                break;

            case get_aligned_buffs_t::BAD_PACKET:
                metadata.error_code = rx_metadata_t::ERROR_CODE_BAD_PACKET;
                return 0;

            case get_aligned_buffs_t::TIMEOUT:
                metadata.error_code = rx_metadata_t::ERROR_CODE_TIMEOUT;
                return 0;

            case get_aligned_buffs_t::ALIGNMENT_FAILURE:
                metadata.error_code = rx_metadata_t::ERROR_CODE_ALIGNMENT;
                return 0;

            case get_aligned_buffs_t::SEQUENCE_ERROR:
                metadata.has_time_spec = _last_read_time_info.has_time_spec;
                metadata.time_spec =
                    _last_read_time_info.time_spec
                    + time_spec_t::from_ticks(_last_read_time_info.num_samps, _samp_rate);
                metadata.out_of_sequence = true;
                metadata.error_code      = rx_metadata_t::ERROR_CODE_OVERFLOW;
                return 0;

            default:
                UHD_THROW_INVALID_CODE_PATH();
        }

        // Get payload pointers for each buffer and aggregate eob. We set eob to
        // true if any channel has it set, since no more data will be received for
        // that channel. In most cases, all channels should have the same value.
        bool eob = false;
        for (size_t i = 0; i < buffs.size(); i++) {
            buffs[i] = _infos[i].payload;
            eob |= _infos[i].eob;
        }

        // Set the metadata from the buffer information at index zero
        const auto& info_0 = _infos[0];

        metadata.has_time_spec  = info_0.has_tsf;
        metadata.time_spec      = time_spec_t::from_ticks(info_0.tsf, _tick_rate);
        metadata.start_of_burst = false;
        metadata.end_of_burst   = eob;
        metadata.error_code     = rx_metadata_t::ERROR_CODE_NONE;

        // Done with these packets, save timestamp info for next call
        _last_read_time_info.has_time_spec = metadata.has_time_spec;
        _last_read_time_info.time_spec     = metadata.time_spec;
        _last_read_time_info.num_samps     = info_0.payload_bytes / _bytes_per_item;

        return _last_read_time_info.num_samps;
    }

    /*!
     * Release the packet for the specified channel
     *
     * \param channel the channel for which to release the packet
     */
    void release_recv_buff(const size_t channel)
    {
        _xports[channel]->release_recv_buff(std::move(_frame_buffs[channel]));
        _frame_buffs[channel] = typename transport_t::buff_t::uptr();
    }

private:
    using get_aligned_buffs_t = get_aligned_buffs<transport_t>;

    // Information recorded by streamer about the last data packet processed,
    // used to create the metadata when there is a sequence error.
    struct last_read_time_info_t
    {
        size_t num_samps   = 0;
        bool has_time_spec = false;
        time_spec_t time_spec;
    };

    // Transports for each channel
    std::vector<typename transport_t::uptr> _xports;

    // Storage for buffers for each channel while they are in flight (between
    // calls to get_recv_buff and release_recv_buff).
    std::vector<typename transport_t::buff_t::uptr> _frame_buffs;

    // Packet info corresponding to the packets in flight
    std::vector<typename transport_t::packet_info_t> _infos;

    // Rate used in conversion of timestamp to time_spec_t
    double _tick_rate = 1.0;

    // Rate used in conversion of timestamp to time_spec_t
    double _samp_rate = 1.0;

    // Size of a sample on the device
    size_t _bytes_per_item = 0;

    // Implementation of packet time alignment
    get_aligned_buffs_t _get_aligned_buffs;

    // Information about the last data packet processed
    last_read_time_info_t _last_read_time_info;
};

}} // namespace uhd::transport

#endif /* INCLUDED_LIBUHD_RX_STREAMER_ZERO_COPY_HPP */
