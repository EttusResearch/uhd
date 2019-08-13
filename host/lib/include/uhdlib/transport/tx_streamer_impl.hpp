//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_TX_STREAMER_IMPL_HPP
#define INCLUDED_LIBUHD_TX_STREAMER_IMPL_HPP

#include <uhd/config.hpp>
#include <uhd/convert.hpp>
#include <uhd/stream.hpp>
#include <uhd/types/metadata.hpp>
#include <uhd/utils/tasks.hpp>
#include <uhdlib/transport/tx_streamer_zero_copy.hpp>
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
    {
        _setup_converters(num_chans, stream_args);
        _zero_copy_streamer.set_bytes_per_item(_convert_info.bytes_per_otw_item);

        if (stream_args.args.has_key("spp")) {
            _spp = stream_args.args.cast<size_t>("spp", _spp);
            _mtu = _spp * _convert_info.bytes_per_otw_item;
        }
    }

    virtual void connect_channel(const size_t channel, typename transport_t::uptr xport)
    {
        const size_t mtu = xport->get_max_payload_size();
        _zero_copy_streamer.connect_channel(channel, std::move(xport));

        if (mtu < _mtu) {
            set_mtu(mtu);
        }
    }

    size_t get_num_channels() const
    {
        return _zero_copy_streamer.get_num_channels();
    }

    size_t get_max_num_samps() const
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
        const double timeout)
    {
        uhd::tx_metadata_t metadata(metadata_);

        if (nsamps_per_buff == 0 && metadata.start_of_burst) {
            _metadata_cache.store(metadata);
            return 0;
        }

        _metadata_cache.check(metadata);

        const int32_t timeout_ms = static_cast<int32_t>(timeout * 1000);

        if (nsamps_per_buff == 0) {
            // Send requests with no samples are handled here, such as end of
            // burst. Send packets need to have at least one sample based on the
            // chdr specification, so we use _zero_buffs here.
            _send_one_packet(_zero_buffs.data(),
                0, // buffer offset
                1, // num samples
                metadata,
                timeout_ms);

            return 0;
        } else if (nsamps_per_buff <= _spp) {
            return _send_one_packet(buffs, 0, nsamps_per_buff, metadata, timeout_ms);

        } else {
            size_t total_num_samps_sent = 0;
            const bool eob              = metadata.end_of_burst;
            metadata.end_of_burst       = false;

            const size_t num_fragments = (nsamps_per_buff - 1) / _spp;
            const size_t final_length  = ((nsamps_per_buff - 1) % _spp) + 1;

            for (size_t i = 0; i < num_fragments; i++) {
                const size_t num_samps_sent = _send_one_packet(
                    buffs, total_num_samps_sent, _spp, metadata, timeout_ms);

                total_num_samps_sent += num_samps_sent;

                if (num_samps_sent == 0) {
                    return total_num_samps_sent;
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
            metadata.end_of_burst = eob;

            size_t nsamps_sent =
                total_num_samps_sent
                + _send_one_packet(
                      buffs, total_num_samps_sent, final_length, metadata, timeout);

            return nsamps_sent;
        }
    }

protected:
    //! Returns the tick rate for conversion of timestamp
    double get_tick_rate() const
    {
        return _zero_copy_streamer.get_tick_rate();
    }

    //! Returns the maximum payload size
    size_t get_mtu() const
    {
        return _mtu;
    }

    //! Sets the MTU and calculates spp
    void set_mtu(const size_t mtu)
    {
        _mtu = mtu;
        _spp = _mtu / _convert_info.bytes_per_otw_item;
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
        const int32_t timeout_ms)
    {
        if (!_zero_copy_streamer.get_send_buffs(
                _out_buffs, num_samples, metadata, timeout_ms)) {
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

    // Maximum number of samples per packet
    size_t _spp = std::numeric_limits<std::size_t>::max();

    // Metadata cache for send calls with no data
    detail::tx_metadata_cache _metadata_cache;
};

}} // namespace uhd::transport

#endif /* INCLUDED_LIBUHD_TRANSPORT_TX_STREAMER_IMPL_HPP */
