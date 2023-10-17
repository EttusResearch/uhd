//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/convert.hpp>
#include <uhd/exception.hpp>
#include <uhd/stream.hpp>
#include <uhd/types/endianness.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/transport/rx_streamer_zero_copy.hpp>
#include <algorithm>
#include <limits>
#include <vector>

namespace uhd { namespace transport {

namespace detail {

/*!
 * Cache of metadata for error handling
 *
 * If a recv call reads data from multiple packets, and an error occurs in the
 * second or later packets, recv stops short of the num samps requested and
 * returns no error. The error is cached for the next call to recv.
 *
 * Timeout errors are an exception. Timeouts that occur in the second or later
 * packets of a recv call stop the recv method but the error is not returned in
 * the next call. The user can check for this condition since fewer samples are
 * returned than the number requested.
 */
class rx_metadata_cache
{
public:
    //! Stores metadata in the cache, ignoring timeout errors
    UHD_FORCE_INLINE void store(const rx_metadata_t& metadata)
    {
        if (metadata.error_code != rx_metadata_t::ERROR_CODE_TIMEOUT) {
            _metadata_cache  = metadata;
            _cached_metadata = true;
        }
    }

    //! Checks for cached metadata
    UHD_FORCE_INLINE bool check(rx_metadata_t& metadata)
    {
        if (_cached_metadata) {
            metadata         = _metadata_cache;
            _cached_metadata = false;
            return true;
        }
        return false;
    }

private:
    // Whether there is a cached metadata object
    bool _cached_metadata = false;

    // Cached metadata value
    uhd::rx_metadata_t _metadata_cache;
};

} // namespace detail

/*!
 * Implementation of rx streamer API
 */
template <typename transport_t, bool ignore_seq_err = false>
class rx_streamer_impl : public rx_streamer
{
public:
    //! Constructor
    rx_streamer_impl(const size_t num_ports, const uhd::stream_args_t stream_args)
        : _zero_copy_streamer(num_ports)
        , _in_buffs(num_ports)
        , _chans_connected(num_ports, false)
    {
        if (stream_args.cpu_format.empty()) {
            throw uhd::value_error("[rx_stream] Must provide a cpu_format!");
        }
        if (stream_args.otw_format.empty()) {
            throw uhd::value_error("[rx_stream] Must provide a otw_format!");
        }
        _setup_converters(num_ports, stream_args);
        _zero_copy_streamer.set_samp_rate(_samp_rate);
        _zero_copy_streamer.set_bytes_per_item(_convert_info.bytes_per_otw_item);

        if (stream_args.args.has_key("spp")) {
            _spp = stream_args.args.cast<size_t>("spp", _spp);
        }
    }

    //! Connect a new channel to the streamer
    // FIXME: Needs some way to handle virtual channels, since xport could be shared among
    // them
    virtual void connect_channel(const size_t channel, typename transport_t::uptr xport)
    {
        const size_t mtu = xport->get_mtu();
        _hdr_len         = std::max(_hdr_len, xport->get_chdr_hdr_len());
        _zero_copy_streamer.connect_channel(channel, std::move(xport));
        // Note: The previous call also checks if the channel index was valid.
        _chans_connected[channel] = true;
        _all_chans_connected      = std::all_of(_chans_connected.cbegin(),
            _chans_connected.cend(),
            [](const bool connected) { return connected; });

        if (mtu < _mtu) {
            set_mtu(mtu);
        }
    }

    //! Implementation of rx_streamer API method
    size_t get_num_channels() const override
    {
        return _zero_copy_streamer.get_num_channels();
    }

    //! Implementation of rx_streamer API method
    size_t get_max_num_samps() const override
    {
        return _spp;
    }

    /*! Get width of each over-the-wire item component. For complex items,
     *  returns the width of one component only (real or imaginary).
     */
    size_t get_otw_item_comp_bit_width() const
    {
        return _convert_info.otw_item_bit_width;
    }

    //! Implementation of rx_streamer API method
    UHD_INLINE size_t recv(const uhd::rx_streamer::buffs_type& buffs,
        const size_t nsamps_per_buff,
        uhd::rx_metadata_t& metadata,
        const double timeout,
        const bool one_packet) override
    {
        if (!_all_chans_connected) {
            throw uhd::runtime_error("[rx_stream] Attempting to call recv() before all "
                                     "channels are connected!");
        }

        if (_error_metadata_cache.check(metadata)) {
            return 0;
        }

        const int32_t timeout_ms = static_cast<int32_t>(timeout * 1000);

        detail::eov_data_wrapper eov_positions(metadata);

        size_t total_samps_recv =
            _recv_one_packet(buffs, nsamps_per_buff, metadata, eov_positions, timeout_ms);

        if (one_packet or metadata.end_of_burst
            or (eov_positions.data() and eov_positions.remaining() == 0)) {
            return total_samps_recv;
        }

        // First set of packets recv had an error, return immediately
        if (metadata.error_code != rx_metadata_t::ERROR_CODE_NONE) {
            return total_samps_recv;
        }

        // Loop until buffer is filled or error code. This method returns the
        // metadata from the first packet received, with the exception of
        // end-of-burst and end-of-vector indications (if requested).
        uhd::rx_metadata_t loop_metadata;

        while (total_samps_recv < nsamps_per_buff) {
            size_t num_samps = _recv_one_packet(buffs,
                nsamps_per_buff - total_samps_recv,
                loop_metadata,
                eov_positions,
                timeout_ms,
                total_samps_recv * _convert_info.bytes_per_cpu_item);

            // If metadata had an error code set, store for next call and return
            if (loop_metadata.error_code != rx_metadata_t::ERROR_CODE_NONE) {
                _error_metadata_cache.store(loop_metadata);
                break;
            }

            total_samps_recv += num_samps;

            // Return immediately if end of burst
            if (loop_metadata.end_of_burst) {
                metadata.end_of_burst = true;
                break;
            }
            // Return if the end-of-vector position array has been exhausted
            if (eov_positions.data() and eov_positions.remaining() == 0) {
                break;
            }
        }

        return total_samps_recv;
    }

protected:
    //! Configures scaling factor for conversion
    void set_scale_factor(const size_t chan, const double scale_factor)
    {
        _converters[chan]->set_scalar(scale_factor);
    }

    //! set maximum number of sample (per packet)
    void set_max_num_samps(const size_t value)
    {
        _spp = value;
    }

    //! Returns the maximum payload size
    size_t get_mtu() const
    {
        return _mtu;
    }

    //! Sets the MTU and checks spp. If spp would exceed the new MTU, it is
    // reduced accordingly.
    void set_mtu(const size_t mtu)
    {
        _mtu                      = mtu;
        const size_t spp_from_mtu = (_mtu - _hdr_len) / _convert_info.bytes_per_otw_item;
        if (spp_from_mtu < _spp) {
            _spp = spp_from_mtu;
        }
    }

    //! Configures sample rate for conversion of timestamp
    void set_samp_rate(const double rate)
    {
        _samp_rate = rate;
        _zero_copy_streamer.set_samp_rate(rate);
    }

    //! Configures tick rate for conversion of timestamp
    void set_tick_rate(const double rate)
    {
        _zero_copy_streamer.set_tick_rate(rate);
    }

    //! Notifies the streamer that an overrun has occured
    void set_stopped_due_to_overrun()
    {
        _zero_copy_streamer.set_stopped_due_to_overrun();
    }

    //! Notifies the streamer that an overrun has occured
    void set_stopped_due_to_late_command()
    {
        _zero_copy_streamer.set_stopped_due_to_late_command();
    }

    //! Provides a callback to handle overruns
    void set_overrun_handler(
        typename rx_streamer_zero_copy<transport_t>::overrun_handler_t handler)
    {
        _zero_copy_streamer.set_overrun_handler(handler);
    }

private:
    //! Converter and associated item sizes
    struct convert_info
    {
        size_t bytes_per_otw_item;
        size_t bytes_per_cpu_item;
        size_t otw_item_bit_width;
    };

    //! Receive a single packet
    UHD_FORCE_INLINE size_t _recv_one_packet(const uhd::rx_streamer::buffs_type& buffs,
        const size_t nsamps_per_buff,
        uhd::rx_metadata_t& metadata,
        detail::eov_data_wrapper& eov_positions,
        const int32_t timeout_ms,
        const size_t buffer_offset_bytes = 0)
    {
        // A request to read zero samples should effectively be a no-op.
        // However, in 2af10ee9, a change was made to increase the probability
        // but not guarantee that calling recv() after a radio overflow event
        // would return the overflow condition to the user. That change
        // introduced a side effect that a read of zero samples (assuming there
        // were no samples available) would block for the entirety of the
        // timeout period and then return ERROR_CODE_TIMEOUT in the RX metadata
        // for the read. (Prior to this change, there was an explicit check for
        // a read of zero samples, which would return to the caller
        // immediately.) This of course is undesirable--a request to read zero
        // samples should always be fulfilled immediately, regardless of the
        // availability of samples. Furthermore, reading zero samples is
        // conventionally used to surface any stream errors, and it's that
        // behavior we would like to preserve.
        //
        // This change to call get_recv_buffs() with a zero timeout when
        // nsamps_per_buff is zero is an attempt to achieve the best of both
        // worlds. The call to get_recv_buffs() will surface any stream errors,
        // but using a timeout of 0 means that we'll return as quickly as
        // possible (with a maximum latency of 1ms; see
        // rx_streamer_zero_copy.hpp, line 219 or so). If there's any stream
        // error, it'll be returned in the metadata. However, if the stream
        // error is ERROR_CODE_TIMEOUT, we'll simply swallow the error, thus
        // preserving the old behavior.
        if (nsamps_per_buff == 0) {
            _zero_copy_streamer.get_recv_buffs(_in_buffs, metadata, eov_positions, 0);
            if (metadata.error_code == rx_metadata_t::ERROR_CODE_TIMEOUT) {
                metadata.error_code = rx_metadata_t::ERROR_CODE_NONE;
            }
            return 0;
        }

        if (_buff_samps_remaining == 0) {
            // Current set of buffers has expired, get the next one
            _buff_samps_remaining = _zero_copy_streamer.get_recv_buffs(
                _in_buffs, metadata, eov_positions, timeout_ms);
            _fragment_offset_in_samps = 0;
        } else {
            // There are samples still left in the current set of buffers
            metadata = _last_fragment_metadata;
            metadata.time_spec += time_spec_t::from_ticks(
                _fragment_offset_in_samps - metadata.fragment_offset, _samp_rate);
        }

        if (_buff_samps_remaining != 0) {
            const size_t num_samps = std::min(nsamps_per_buff, _buff_samps_remaining);

            // Convert samples to the streamer's output format
            for (size_t i = 0; i < get_num_channels(); i++) {
                char* b = reinterpret_cast<char*>(buffs[i]);
                const uhd::rx_streamer::buffs_type out_buffs(b + buffer_offset_bytes);
                _convert_to_out_buff(out_buffs, i, num_samps);
            }

            _buff_samps_remaining -= num_samps;

            // Write the fragment flags and offset
            metadata.more_fragments  = _buff_samps_remaining != 0;
            metadata.fragment_offset = _fragment_offset_in_samps;

            if (metadata.more_fragments) {
                _fragment_offset_in_samps += num_samps;
                _last_fragment_metadata = metadata;
            }

            return num_samps;
        } else {
            return 0;
        }
    }

    //! Convert samples for one channel into its buffer
    UHD_FORCE_INLINE void _convert_to_out_buff(
        const uhd::rx_streamer::buffs_type& out_buffs,
        const size_t chan,
        const size_t num_samps)
    {
        const char* buffer_ptr = reinterpret_cast<const char*>(_in_buffs[chan]);

        _converters[chan]->conv(buffer_ptr, out_buffs, num_samps);

        // Advance the pointer for the source buffer
        _in_buffs[chan] = buffer_ptr + num_samps * _convert_info.bytes_per_otw_item;

        if (_buff_samps_remaining == num_samps) {
            _zero_copy_streamer.release_recv_buff(chan);
        }
    }

    //! Create converters and initialize _convert_info
    void _setup_converters(const size_t num_ports, const uhd::stream_args_t stream_args)
    {
        // Note to code archaeologists: In the past, we had to also specify the
        // endianness here, but that is no longer necessary because we can make
        // the wire endianness match the host endianness.
        convert::id_type id;
        id.input_format  = stream_args.otw_format + "_chdr";
        id.num_inputs    = 1;
        id.output_format = stream_args.cpu_format;
        id.num_outputs   = 1;

        auto starts_with = [](const std::string& s, const std::string v) {
            return s.find(v) == 0;
        };

        const bool otw_is_complex = starts_with(stream_args.otw_format, "fc")
                                    || starts_with(stream_args.otw_format, "sc");

        convert_info info;
        info.bytes_per_otw_item = convert::get_bytes_per_item(id.input_format);
        info.bytes_per_cpu_item = convert::get_bytes_per_item(id.output_format);

        if (otw_is_complex) {
            info.otw_item_bit_width = info.bytes_per_otw_item * 8 / 2;
        } else {
            info.otw_item_bit_width = info.bytes_per_otw_item * 8;
        }

        _convert_info = info;

        for (size_t i = 0; i < num_ports; i++) {
            _converters.push_back(convert::get_converter(id)());
            _converters.back()->set_scalar(1 / 32767.0);
        }
    }

    // Converter and item sizes
    convert_info _convert_info;

    // Converters
    std::vector<uhd::convert::converter::sptr> _converters;

    // Implementation of frame buffer management and packet info
    rx_streamer_zero_copy<transport_t, ignore_seq_err> _zero_copy_streamer;

    // Container for buffer pointers used in recv method
    std::vector<const void*> _in_buffs;

    // Sample rate used to calculate metadata time_spec_t
    double _samp_rate = 1.0;

    // MTU, determined when xport is connected and modifiable by subclass
    size_t _mtu = std::numeric_limits<std::size_t>::max();

    // Size of CHDR header in bytes
    size_t _hdr_len = 0;

    // Maximum number of samples per packet. Note that this is not necessarily
    // related to the MTU, it is a user-chosen value. However, it is always
    // bounded by the MTU.
    size_t _spp = std::numeric_limits<std::size_t>::max();

    // Num samps remaining in buffer currently held by zero copy streamer
    size_t _buff_samps_remaining = 0;

    // Metadata cache for error handling
    detail::rx_metadata_cache _error_metadata_cache;

    // Fragment (partially read packet) information
    size_t _fragment_offset_in_samps = 0;
    rx_metadata_t _last_fragment_metadata;

    // Store a list of channels that are already connected
    std::vector<bool> _chans_connected;

    // Flag to store if all channels are connected. This is to speed up the lookup
    // of all channels' connected-status.
    bool _all_chans_connected = false;
};

}} // namespace uhd::transport
