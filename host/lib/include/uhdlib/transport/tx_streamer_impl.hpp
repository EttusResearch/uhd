//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/convert.hpp>
#include <uhd/stream.hpp>
#include <uhd/types/metadata.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/tasks.hpp>
#include <uhdlib/transport/tx_streamer_zero_copy.hpp>
#include <algorithm>
#include <limits>
#include <vector>

namespace uhd { namespace transport {

namespace detail {

/*!
 * Cache of metadata for send calls with zero samples
 *
 * Metadata is cached when we get a send requesting a start of burst with no
 * samples. It is applied here on the next call to send() that actually has
 * samples to send.
 */
class tx_metadata_cache
{
public:
    //! Stores metadata in the cache
    UHD_FORCE_INLINE void store(const tx_metadata_t& metadata)
    {
        _metadata_cache  = metadata;
        _cached_metadata = true;
    }

    //! Checks for cached metadata
    UHD_FORCE_INLINE void check(tx_metadata_t& metadata)
    {
        if (_cached_metadata) {
            // Only use cached time_spec if metadata does not have one
            if (!metadata.has_time_spec) {
                metadata.has_time_spec = _metadata_cache.has_time_spec;
                metadata.time_spec     = _metadata_cache.time_spec;
            }
            metadata.start_of_burst = _metadata_cache.start_of_burst;
            metadata.end_of_burst   = _metadata_cache.end_of_burst;
            _cached_metadata        = false;
        }
    }

private:
    // Whether there is a cached metadata object
    bool _cached_metadata = false;

    // Cached metadata value
    uhd::tx_metadata_t _metadata_cache;
};

class tx_eov_data_wrapper
{
public:
    tx_eov_data_wrapper(const uhd::tx_metadata_t& metadata)
        : _eov_positions(metadata.eov_positions)
        , _remaining(metadata.eov_positions_size)
        , _read_pos(0)
    {
    }

    UHD_FORCE_INLINE size_t* data() const
    {
        return _eov_positions;
    }

    UHD_FORCE_INLINE size_t remaining() const
    {
        return _remaining;
    }

    UHD_FORCE_INLINE size_t pop_front()
    {
        assert(_eov_positions && _remaining > 0);
        _remaining--;
        return _eov_positions[_read_pos++];
    }

private:
    size_t* _eov_positions;
    size_t _remaining;
    size_t _read_pos;
};

} // namespace detail

/*!
 * Implementation of tx streamer API
 */
template <typename transport_t>
class tx_streamer_impl : public tx_streamer
{
public:
    tx_streamer_impl(const size_t num_chans, const uhd::stream_args_t stream_args)
        : _zero_copy_streamer(num_chans)
        , _zero_buffs(num_chans, &_zero)
        , _out_buffs(num_chans)
        , _chans_connected(num_chans, false)
    {
        _setup_converters(num_chans, stream_args);
        _zero_copy_streamer.set_bytes_per_item(_convert_info.bytes_per_otw_item);

        if (stream_args.args.has_key("spp")) {
            _spp = stream_args.args.cast<size_t>("spp", _spp);
        }
    }

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

    size_t get_num_channels() const override
    {
        return _zero_copy_streamer.get_num_channels();
    }

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

    size_t send(const uhd::tx_streamer::buffs_type& buffs,
        const size_t nsamps_per_buff,
        const uhd::tx_metadata_t& metadata_,
        const double timeout) override
    {
        if (!_all_chans_connected) {
            throw uhd::runtime_error("[tx_stream] Attempting to call send() before all "
                                     "channels are connected!");
        }
        uhd::tx_metadata_t metadata(metadata_);

        if (nsamps_per_buff == 0 && metadata.start_of_burst) {
            _metadata_cache.store(metadata);
            return 0;
        }

        _metadata_cache.check(metadata);

        const bool eob_on_last_packet = metadata.end_of_burst;

        const int32_t timeout_ms = static_cast<int32_t>(timeout * 1000);

        detail::tx_eov_data_wrapper eov_positions(metadata);

        // If there are EOVs specified in the metadata, it will be necessary
        // to break up the packet sends based on where the EOVs should be
        // generated in the sequence of packets.
        //
        // `nsamps_to_send_remaining` represents the total number of
        // samples remaining to send to fulfill the caller's request.
        size_t nsamps_to_send_remaining = nsamps_per_buff;

        // `nsamps_to_send` represents a subset of the total number of
        // samples to send based on whether or not the caller's metadata
        // specifies EOV positions.
        // * If there are no EOVs, it represents the entire send request
        //   made by the caller. It may be broken up into chunks no larger
        //   than _spp later on in the function, but it will not be broken up
        //   due to EOV. There will only be one iteration through the do/
        //   while loop.
        // * If there are EOVs, `nsamps_to_send` represents the number of
        //   samples to send to get to the next EOV position. Again, it may
        //   be broken up into chunks no larger than _spp, but note that the
        //   final chunk will have EOV signalled in its header. There may be
        //   multiple iterations through the do/while loop to fulfill the
        //   caller's entire send request.
        size_t nsamps_to_send;

        // `num_samps_sent` is the return value from each individual call
        // to `_send_one_packet()`.
        size_t num_samps_sent = 0;

        // `total_nsamps_sent` accumulates the total number of samples sent
        // in each chunk, and is used to determine the offset within `buffs`
        // to pass to `_send_one_packet()`.
        size_t total_nsamps_sent = 0;

        size_t last_eov_position = 0;
        bool eov;

        do {
            if (eov_positions.data() and eov_positions.remaining() > 0) {
                size_t next_eov_position = eov_positions.pop_front();
                // Check basic requirements: EOV positions must be monotonically
                // increasing
                if (next_eov_position <= last_eov_position) {
                    throw uhd::value_error("Invalid EOV position specified "
                                           "(violates eov_pos[n] > eov_pos[n-1])");
                }
                // EOV position must be within the range of the samples written
                if (next_eov_position > nsamps_per_buff) {
                    throw uhd::value_error("Invalid EOV position specified "
                                           "(violates eov_pos[n] <= nsamps_per_buff)");
                }
                nsamps_to_send = next_eov_position - last_eov_position;
                eov            = true;
            } else {
                // No EOVs, or the EOV position list has been exhausted:
                // simply send the remaining samples
                nsamps_to_send = nsamps_to_send_remaining;
                eov            = false;
            }

            if (nsamps_to_send == 0) {
                // Send requests with no samples are handled here, such as end of
                // burst. Send packets need to have at least one sample based on the
                // chdr specification, so we use _zero_buffs here.
                _send_one_packet(_zero_buffs,
                    0, // buffer offset
                    1, // num samples
                    metadata,
                    false,
                    timeout_ms);

                return 0;

            } else if (nsamps_to_send <= _spp) {
                // If last packet, apply saved EOB state to metadata
                metadata.end_of_burst =
                    (eob_on_last_packet and nsamps_to_send == nsamps_to_send_remaining);

                num_samps_sent = _send_one_packet(
                    buffs, total_nsamps_sent, nsamps_to_send, metadata, eov, timeout_ms);

                metadata.start_of_burst = false;
            } else {
                // Note: since `nsamps_to_send` is guaranteed to be > _spp
                // if the code reaches this else clause, `num_fragments` will
                // always be at least 1.
                const size_t num_fragments = (nsamps_to_send - 1) / _spp;
                const size_t final_length  = ((nsamps_to_send - 1) % _spp) + 1;

                metadata.end_of_burst = false;

                for (size_t i = 0; i < num_fragments; i++) {
                    num_samps_sent = _send_one_packet(
                        buffs, total_nsamps_sent, _spp, metadata, false, timeout_ms);

                    // Advance sample accumulator and decrement remaining
                    // samples for this segment
                    total_nsamps_sent += num_samps_sent;
                    nsamps_to_send_remaining -= num_samps_sent;

                    if (num_samps_sent == 0) {
                        return total_nsamps_sent;
                    }

                    // Setup timespec for the next fragment
                    if (metadata.has_time_spec) {
                        metadata.time_spec =
                            metadata.time_spec
                            + time_spec_t::from_ticks(num_samps_sent, _samp_rate);
                    }

                    metadata.start_of_burst = false;
                }

                // Send the final fragment
                metadata.end_of_burst =
                    (eob_on_last_packet and final_length == nsamps_to_send_remaining);

                num_samps_sent = _send_one_packet(
                    buffs, total_nsamps_sent, final_length, metadata, eov, timeout_ms);
            }

            // Advance sample accumulator and decrement remaining samples
            total_nsamps_sent += num_samps_sent;
            nsamps_to_send_remaining -= num_samps_sent;

            // Loop exit condition: return from `_send_one_packet()` indicates
            // an error
            if (num_samps_sent == 0) {
                break;
            }

            // If there are more samples to be sent, thus requiring another
            // trip around the do/while loop, update the timespec in the
            // metadata for the next fragment (if desired)
            if (nsamps_to_send_remaining > 0 and metadata.has_time_spec) {
                metadata.time_spec =
                    metadata.time_spec
                    + time_spec_t::from_ticks(num_samps_sent, _samp_rate);
            }

            last_eov_position = total_nsamps_sent;

        } while (nsamps_to_send_remaining > 0);

        return total_nsamps_sent;
    }

protected:
    //! Returns the tick rate for conversion of timestamp
    double get_tick_rate() const
    {
        return _zero_copy_streamer.get_tick_rate();
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

    //! Configures scaling factor for conversion
    void set_scale_factor(const size_t chan, const double scale_factor)
    {
        _converters[chan]->set_scalar(scale_factor);
    }

    //! Configures sample rate for conversion of timestamp
    void set_samp_rate(const double rate)
    {
        _samp_rate = rate;
    }

    //! Configures tick rate for conversion of timestamp
    void set_tick_rate(const double rate)
    {
        _zero_copy_streamer.set_tick_rate(rate);
    }

private:
    //! Converter and associated item sizes
    struct convert_info
    {
        size_t bytes_per_otw_item;
        size_t bytes_per_cpu_item;
        size_t otw_item_bit_width;
    };

    //! Convert samples for one channel and sends a packet
    size_t _send_one_packet(const uhd::tx_streamer::buffs_type& buffs,
        const size_t buffer_offset_in_samps,
        const size_t num_samples,
        const tx_metadata_t& metadata,
        const bool eov,
        const int32_t timeout_ms)
    {
        assert(buffs.size() == get_num_channels());

        if (!_zero_copy_streamer.get_send_buffs(
                _out_buffs, num_samples, metadata, eov, timeout_ms)) {
            return 0;
        }

        size_t byte_offset = buffer_offset_in_samps * _convert_info.bytes_per_cpu_item;

        for (size_t i = 0; i < get_num_channels(); i++) {
            const void* input_ptr = static_cast<const uint8_t*>(buffs[i]) + byte_offset;
            _converters[i]->conv(input_ptr, _out_buffs[i], num_samples);

            _zero_copy_streamer.release_send_buff(i);
        }

        return num_samples;
    }

    //! Create converters and initialize _bytes_per_cpu_item
    void _setup_converters(const size_t num_chans, const uhd::stream_args_t stream_args)
    {
        // Note to code archaeologists: In the past, we had to also specify the
        // endianness here, but that is no longer necessary because we can make
        // the wire endianness match the host endianness.
        convert::id_type id;
        id.input_format  = stream_args.cpu_format;
        id.num_inputs    = 1;
        id.output_format = stream_args.otw_format + "_chdr";
        id.num_outputs   = 1;

        auto starts_with = [](const std::string& s, const std::string v) {
            return s.find(v) == 0;
        };

        const bool otw_is_complex = starts_with(stream_args.otw_format, "fc")
                                    || starts_with(stream_args.otw_format, "sc");

        convert_info info;
        info.bytes_per_otw_item = convert::get_bytes_per_item(id.output_format);
        info.bytes_per_cpu_item = convert::get_bytes_per_item(id.input_format);

        if (otw_is_complex) {
            info.otw_item_bit_width = info.bytes_per_otw_item * 8 / 2;
        } else {
            info.otw_item_bit_width = info.bytes_per_otw_item * 8;
        }

        _convert_info = info;

        for (size_t i = 0; i < num_chans; i++) {
            _converters.push_back(convert::get_converter(id)());
            _converters.back()->set_scalar(32767.0);
        }
    }

    // Converter item sizes
    convert_info _convert_info;

    // Converters
    std::vector<uhd::convert::converter::sptr> _converters;

    // Manages frame buffers and packet info
    tx_streamer_zero_copy<transport_t> _zero_copy_streamer;

    // Buffer used to handle send calls with no data
    std::vector<const void*> _zero_buffs;

    const uint64_t _zero = 0;

    // Container for buffer pointers used in send method
    std::vector<void*> _out_buffs;

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

    // Metadata cache for send calls with no data
    detail::tx_metadata_cache _metadata_cache;

    // Store a list of channels that are already connected
    std::vector<bool> _chans_connected;

    // Flag to store if all channels are connected. This is to speed up the lookup
    // of all channels' connected-status.
    bool _all_chans_connected = false;
};

}} // namespace uhd::transport
