//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/exception.hpp>
#include <uhd/types/metadata.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/transport/get_aligned_buffs.hpp>
#include <boost/format.hpp>
#include <atomic>
#include <vector>

namespace uhd { namespace transport {

namespace detail {

class eov_data_wrapper
{
public:
    eov_data_wrapper(uhd::rx_metadata_t& metadata)
        : _metadata(metadata)
        , _data(metadata.eov_positions)
        , _size(metadata.eov_positions_size)
        , _remaining(metadata.eov_positions_size)
        , _write_pos(0)
        , _running_sample_count(0)
    {
    }

    ~eov_data_wrapper()
    {
        _metadata.eov_positions       = _data;
        _metadata.eov_positions_size  = _size;
        _metadata.eov_positions_count = _write_pos;
    }

    UHD_FORCE_INLINE size_t* data() const
    {
        return _data;
    }

    UHD_FORCE_INLINE size_t remaining() const
    {
        return _remaining;
    }

    UHD_FORCE_INLINE void push_back(size_t value)
    {
        assert(_data && _remaining > 0);
        _data[_write_pos++] = value;
        _remaining--;
    }

    UHD_FORCE_INLINE void update_running_sample_count(size_t num_samples)
    {
        _running_sample_count += num_samples;
    }

    UHD_FORCE_INLINE size_t get_running_sample_count() const
    {
        return _running_sample_count;
    }

private:
    uhd::rx_metadata_t& _metadata;
    size_t* _data;
    size_t _size;
    size_t _remaining;
    size_t _write_pos;
    size_t _running_sample_count;
};

} // namespace detail

/*!
 * Implementation of rx streamer manipulation of frame buffers and packet info.
 * This class is part of rx_streamer_impl, split into a separate unit as it is
 * a mostly self-contained portion of the streamer logic.
 */
template <typename transport_t, bool ignore_seq_err = false>
class rx_streamer_zero_copy
{
public:
    using overrun_handler_t = std::function<void()>;

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

    //! Notifies the streamer that an overrun has occured
    void set_stopped_due_to_overrun()
    {
        _stopped_due_to_overrun = true;
    }

    //! Notifies the streamer that a late command has occured
    void set_stopped_due_to_late_command()
    {
        _stopped_due_to_late_cmd = true;
    }

    //! Provides a callback to handle overruns
    void set_overrun_handler(overrun_handler_t handler)
    {
        _overrun_handler = handler;
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
        detail::eov_data_wrapper& eov_positions,
        const int32_t timeout_ms)
    {
        // Function to set metadata based on alignment error
        auto set_metadata_for_error =
            [this](typename get_aligned_buffs_t::alignment_result_t error,
                rx_metadata_t& metadata) {
                switch (error) {
                    case get_aligned_buffs_t::BAD_PACKET:
                        metadata.error_code = rx_metadata_t::ERROR_CODE_BAD_PACKET;
                        break;

                    case get_aligned_buffs_t::TIMEOUT:
                        metadata.error_code = rx_metadata_t::ERROR_CODE_TIMEOUT;
                        break;

                    case get_aligned_buffs_t::ALIGNMENT_FAILURE:
                        metadata.error_code = rx_metadata_t::ERROR_CODE_ALIGNMENT;
                        break;

                    case get_aligned_buffs_t::SEQUENCE_ERROR:
                        std::tie(metadata.has_time_spec, metadata.time_spec) =
                            _last_read_time_info.get_next_packet_time(_samp_rate);
                        metadata.out_of_sequence = true;
                        metadata.error_code      = rx_metadata_t::ERROR_CODE_OVERFLOW;
                        break;

                    default:
                        UHD_THROW_INVALID_CODE_PATH();
                }
            };

        metadata.reset();

        // Try to get buffs with a 0 timeout first. This avoids needing to check
        // if radios are stopped due to overrun when packets are available.
        auto result = _get_aligned_buffs(0);

        if (result == get_aligned_buffs_t::TIMEOUT) {
            if (!_stopped_due_to_overrun) {
                // Packets were not available with zero timeout, wait for them
                // to arrive using the specified timeout.
                result = _get_aligned_buffs(std::max(1, timeout_ms));
            }
            if (result == get_aligned_buffs_t::TIMEOUT) {
                if (_stopped_due_to_overrun) {
                    // An overrun occurred and the user has read all the packets
                    // that were buffered prior to the overrun. Call the overrun
                    // handler and return overrun error.
                    _handle_overrun();
                    std::tie(metadata.has_time_spec, metadata.time_spec) =
                        _last_read_time_info.get_next_packet_time(_samp_rate);
                    metadata.error_code     = rx_metadata_t::ERROR_CODE_OVERFLOW;
                    _stopped_due_to_overrun = false;
                    return 0;
                } else if (_stopped_due_to_late_cmd) {
                    metadata.has_time_spec   = false;
                    metadata.error_code      = rx_metadata_t::ERROR_CODE_LATE_COMMAND;
                    _stopped_due_to_late_cmd = false;
                    return 0;
                }
            }
        }

        if (result != get_aligned_buffs_t::SUCCESS) {
            set_metadata_for_error(result, metadata);
            return 0;
        }

        // Get payload pointers for each buffer and aggregate eob. We set eob to
        // true if any channel has it set, since no more data will be received for
        // that channel. In most cases, all channels should have the same value.
        // We do the same for eov here, as it is expected that eov will be the
        // same for all channels.
        bool eob = false;
        bool eov = false;
        for (size_t i = 0; i < buffs.size(); i++) {
            buffs[i] = _infos[i].payload;
            eob |= _infos[i].eob;
            eov |= _infos[i].eov;
        }

        // Set the metadata from the buffer information at index zero
        const auto& info_0 = _infos[0];

        metadata.has_time_spec  = info_0.has_tsf;
        metadata.time_spec      = time_spec_t::from_ticks(info_0.tsf, _tick_rate);
        metadata.start_of_burst = false;
        metadata.end_of_burst   = eob;
        metadata.error_code     = rx_metadata_t::ERROR_CODE_NONE;

        // If the caller wants eov indications via metadata, then check
        // eov and set the metadata values appropriately. Note that only
        // channel 0 is checked for eov--in most cases, it should be the
        // same for all channels.
        if (eov_positions.data() && eov) {
            eov_positions.push_back(eov_positions.get_running_sample_count()
                                    + info_0.payload_bytes / _bytes_per_item);
        }

        // Done with these packets, save timestamp info for next call
        _last_read_time_info.has_time_spec = metadata.has_time_spec;
        _last_read_time_info.time_spec     = metadata.time_spec;
        _last_read_time_info.num_samps     = info_0.payload_bytes / _bytes_per_item;
        eov_positions.update_running_sample_count(_last_read_time_info.num_samps);

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
    using get_aligned_buffs_t = get_aligned_buffs<transport_t, ignore_seq_err>;

    void _handle_overrun()
    {
        // Flush any remaining packets. This method is called after any channel
        // times out, so here we ensure all channels are flushed prior to
        // calling the overrun handler to potentially restart the radios.
        for (size_t chan = 0; chan < _xports.size(); chan++) {
            if (_frame_buffs[chan]) {
                _xports[chan]->release_recv_buff(std::move(_frame_buffs[chan]));
                _frame_buffs[chan] = nullptr;
            }

            typename transport_t::buff_t::uptr buff;
            while (true) {
                std::tie(buff, std::ignore, std::ignore) =
                    _xports[chan]->get_recv_buff(0);
                if (!buff) {
                    break;
                }
                _xports[chan]->release_recv_buff(std::move(buff));
            }
        }

        // Now call the overrun handler
        if (_overrun_handler) {
            _overrun_handler();
        }
    }

    // Information recorded by streamer about the last data packet processed,
    // used to create the metadata when there is a sequence error.
    struct last_read_time_info_t
    {
        size_t num_samps   = 0;
        bool has_time_spec = false;
        time_spec_t time_spec;

        std::tuple<bool, time_spec_t> get_next_packet_time(double samp_rate)
        {
            if (has_time_spec) {
                return std::make_tuple(
                    true, time_spec + time_spec_t::from_ticks(num_samps, samp_rate));
            } else {
                return std::make_tuple(false, time_spec_t());
            }
        }
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

    // Total number of samples read, used in determining EOV positions
    size_t _total_num_samps = 0;

    // Flag that indicates an overrun occurred. The streamer will return an
    // overrun error when no more packets are available.
    std::atomic<bool> _stopped_due_to_overrun{false};

    // Flag that indicates a late command occurred. The streamer will return a
    // late command error when no more packets are available.
    std::atomic<bool> _stopped_due_to_late_cmd{false};

    // Callback for overrun
    overrun_handler_t _overrun_handler;
};

}} // namespace uhd::transport
