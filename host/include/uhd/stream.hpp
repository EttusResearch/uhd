//
// Copyright 2011-2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhd/types/metadata.hpp>
#include <uhd/types/ref_vector.hpp>
#include <uhd/types/stream_cmd.hpp>
#include <uhd/utils/noncopyable.hpp>
#include <boost/utility.hpp>
#include <memory>
#include <string>
#include <vector>

namespace uhd {

/*!
 * A struct of parameters to construct a streamer.
 *
 * Here is an example of how a stream args object could be used in conjunction
 * with uhd::device::get_rx_stream():
 *
 * \code{.cpp}
 * // 1. Create the stream args object and initialize the data formats to fc32 and sc16:
 * uhd::stream_args_t stream_args("fc32", "sc16");
 * // 2. Set the channel list, we want 3 streamers coming from channels
 * //    0, 1 and 2, in that order:
 * stream_args.channels = {0, 1, 2};
 * // 3. Set optional args:
 * stream_args.args["spp"] = "200"; // 200 samples per packet
 * // Now use these args to create an rx streamer:
 * // (We assume that usrp is a valid uhd::usrp::multi_usrp)
 * uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);
 * // Now, any calls to rx_stream must provide a vector of 3 buffers,
 * // one per channel.
 * \endcode
 *
 * \b Note: Not all combinations of CPU and OTW format have conversion support.
 * You may however write and register your own conversion routines.
 */
struct UHD_API stream_args_t
{
    //! Convenience constructor for streamer args
    stream_args_t(const std::string& cpu = "", const std::string& otw = "")
    {
        cpu_format = cpu;
        otw_format = otw;
    }

    /*!
     * The CPU format is a string that describes the format of host memory.
     * Conversions for the following CPU formats have been implemented:
     *  - fc64 - complex<double>
     *  - fc32 - complex<float>
     *  - sc16 - complex<int16_t>
     *  - sc8 - complex<int8_t>
     *
     * The following are not implemented, but are listed to demonstrate naming convention:
     *  - f32 - float
     *  - f64 - double
     *  - s16 - int16_t
     *  - s8 - int8_t
     *
     * The CPU format can be chosen depending on what the application requires.
     */
    std::string cpu_format;

    /*!
     * The OTW format is a string that describes the format over-the-wire.
     * The following over-the-wire formats have been implemented:
     *  - sc16 - Q16 I16
     *  - sc8 - Q8_1 I8_1 Q8_0 I8_0
     *  - sc12 (Only some devices)
     *
     * The following are not implemented, but are listed to demonstrate naming convention:
     *  - s16 - R16_1 R16_0
     *  - s8 - R8_3 R8_2 R8_1 R8_0
     *
     * Setting the OTW ("over-the-wire") format is, in theory, transparent to the
     * application, but changing this can have some side effects. Using less bits for
     * example (e.g. when going from `otw_format` `sc16` to `sc8`) will reduce the dynamic
     * range, and increases quantization noise. On the other hand, it reduces the load on
     * the data link and thus allows more bandwidth (a USRP N210 can work with 25 MHz
     * bandwidth for 16-Bit complex samples, and 50 MHz for 8-Bit complex samples).
     */
    std::string otw_format;

    /*!
     * The args parameter is used to pass arbitrary key/value pairs.
     * Possible keys used by args (depends on implementation):
     *
     * - fullscale: specifies the full-scale amplitude when using floats.
     * By default, the fullscale amplitude under floating point is 1.0.
     * Set the "fullscale" to scale the samples in the host to the
     * expected input range and/or output range of your application.
     *
     * - peak: specifies a fractional sample level to calculate scaling with the sc8 wire
     * format. When using sc8 samples over the wire, the device must scale samples (both
     * on the host and in the device) to satisfy the dynamic range needs. The peak value
     * specifies a fraction of the maximum sample level (1.0 = 100%). Set peak to
     * max_sample_level/full_scale_level to ensure optimum dynamic range.
     *
     * - underflow_policy: how the TX DSP should recover from underflow.
     * Possible options are "next_burst" or "next_packet".
     * In the "next_burst" mode, the DSP drops incoming packets until a new burst is
     * started. In the "next_packet" mode, the DSP starts transmitting again at the next
     * packet.
     *
     * - spp: (samples per packet) controls the size of RX packets.
     * When not specified, the packets are always maximum frame size.
     * Users should specify this option to request smaller than default
     * packets, probably with the intention of reducing packet latency.
     *
     * - noclear: Used by tx_dsp_core_200 and rx_dsp_core_200
     *
     * The following are not implemented, but are listed for conceptual purposes:
     * - function: magnitude or phase/magnitude
     * - units: numeric units like counts or dBm
     *
     * Other options are device-specific:
     * - port, addr: Alternative receiver streamer destination.
     */
    device_addr_t args;

    /*! List of channel numbers (only used by non-RFNoC devices)
     *
     * Note: For RFNoC devices, this value is not used. To create a streamer
     * with multiple channels, the uhd::rfnoc::rfnoc_graph::create_tx_streamer()
     * and uhd::rfnoc::rfnoc_graph::create_rx_streamer() API calls have a
     * \p num_ports argument.
     *
     * For non-RFNoC devices (i.e., USRP1, B100, B200, N200), this argument
     * defines how streamer channels map to the front-end selection (see also
     * \ref config_subdev).
     *
     * A very simple example is a B210 with a subdev spec of `A:A A:B`. This
     * means the device has two channels available.
     *
     * Setting `stream_args.channels = {0, 1}` therefore configures MIMO
     * streaming from both channels. By switching the channel indexes,
     * `stream_args.channels = {1, 0}`, the channels are switched and the first
     * channel of the USRP is mapped to the second channel in the application.
     *
     * If only a single channel is used for streaming, e.g.,
     * `stream_args.channels = {1}` would only select a single channel (in this
     * case, the second one). When streaming a single channel from the B-side
     * radio of a USRP, this is a more versatile solution than setting the
     * subdev spec globally to "A:B".
     *
     * Leave this blank to default to channel 0 (single-channel application).
     */
    std::vector<size_t> channels;
};

/*!
 * The RX streamer is the host interface to receiving samples.
 * It represents the layer between the samples on the host
 * and samples inside the device's receive DSP processing.
 */
class UHD_API rx_streamer : uhd::noncopyable
{
public:
    typedef std::shared_ptr<rx_streamer> sptr;

    virtual ~rx_streamer(void);

    //! Get the number of channels associated with this streamer
    virtual size_t get_num_channels(void) const = 0;

    //! Get the max number of samples per buffer per packet
    virtual size_t get_max_num_samps(void) const = 0;

    //! Typedef for a pointer to a single, or a collection of recv buffers
    typedef ref_vector<void*> buffs_type;

    /*!
     * Receive buffers containing samples described by the metadata.
     *
     * Receive handles fragmentation as follows:
     * If the buffer has insufficient space to hold all samples
     * that were received in a single packet over-the-wire,
     * then the buffer will be completely filled and the implementation
     * will hold a pointer into the remaining portion of the packet.
     * Subsequent calls will load from the remainder of the packet,
     * and will flag the metadata to show that this is a fragment.
     * The next call to receive, after the remainder becomes exhausted,
     * will perform an over-the-wire receive as usual.
     * See the rx metadata fragment flags and offset fields for details.
     *
     * This is a blocking call and will not return until the number
     * of samples returned have been written into each buffer.
     * Under a timeout condition, the number of samples returned
     * may be less than the number of samples specified.
     *
     * The one_packet option allows the user to guarantee that
     * the call will return after a single packet has been processed.
     * This may be useful to maintain packet boundaries in some cases.
     *
     * Note on threading: recv() is *not* thread-safe, to avoid locking
     * overhead. The application calling recv() is responsible for making
     * sure that not more than one thread can call recv() on the same streamer
     * at the same time. If there are multiple streamers, receiving from
     * different sources, then those may be called from different threads
     * simultaneously.
     *
     * \section stream_rx_error_handling Error Handling
     *
     * \p metadata is a value that is set inside this function (effectively, a
     * return value), and should be checked
     * for potential error codes (see rx_metadata_t::error_code_t).
     *
     * The most common error code when something goes wrong is an overrun (also
     * referred to as overflow: error_code_t::ERROR_CODE_OVERFLOW). This error
     * code means that the device produced data faster than the application
     * could read, and various buffers filled up leaving no more space for the
     * device to write data to. Note that an overrun on the device will not
     * immediatiely show up when calling recv(). Depending on the device
     * implementation, there may be many more valid samples available before the
     * device had to stop writing samples to the FIFO. Only when all valid
     * samples are returned to the call site will the error code be set to
     * "overrun". When this happens, all valid samples have been returned to
     * application where recv() was called.
     * If the device is streaming continuously, it will reset itself when the
     * FIFO is cleared, and recv() can be called again to retrieve new, valid data.
     *
     * \param buffs a vector of writable memory to fill with samples
     * \param nsamps_per_buff the size of each buffer in number of samples
     * \param[out] metadata data to fill describing the buffer
     * \param timeout the timeout in seconds to wait for a packet
     * \param one_packet return after the first packet is received
     * \return the number of samples received or 0 on error
     */
    virtual size_t recv(const buffs_type& buffs,
        const size_t nsamps_per_buff,
        rx_metadata_t& metadata,
        const double timeout  = 0.1,
        const bool one_packet = false) = 0;

    /*!
     * Issue a stream command to the usrp device.
     * This tells the usrp to send samples into the host.
     * See the documentation for stream_cmd_t for more info.
     *
     * With multiple devices, the first stream command in a chain of commands
     * should have a time spec in the near future and stream_now = false;
     * to ensure that the packets can be aligned by their time specs.
     *
     * \param stream_cmd the stream command to issue
     */
    virtual void issue_stream_cmd(const stream_cmd_t& stream_cmd) = 0;
};

/*!
 * The TX streamer is the host interface to transmitting samples.
 * It represents the layer between the samples on the host
 * and samples inside the device's transmit DSP processing.
 */
class UHD_API tx_streamer : uhd::noncopyable
{
public:
    typedef std::shared_ptr<tx_streamer> sptr;

    virtual ~tx_streamer(void);

    //! Get the number of channels associated with this streamer
    virtual size_t get_num_channels(void) const = 0;

    //! Get the max number of samples per buffer per packet
    virtual size_t get_max_num_samps(void) const = 0;

    //! Typedef for a pointer to a single, or a collection of send buffers
    typedef ref_vector<const void*> buffs_type;

    /*!
     * Send buffers containing samples described by the metadata.
     *
     * Send handles fragmentation as follows:
     * If the buffer has more items than the maximum per packet,
     * the send method will fragment the samples across several packets.
     * Send will respect the burst flags when fragmenting to ensure
     * that start of burst can only be set on the first fragment and
     * that end of burst can only be set on the final fragment.
     *
     * This is a blocking call and will not return until the number
     * of samples returned have been read out of each buffer.
     * Under a timeout condition, the number of samples returned
     * may be less than the number of samples specified.
     *
     * Note on threading: send() is *not* thread-safe, to avoid locking
     * overhead. The application calling send() is responsible for making
     * sure that not more than one thread can call send() on the same streamer
     * at the same time. If there are multiple streamers, transmitting to
     * different destinations, then those may be called from different threads
     * simultaneously.
     *
     * \param buffs a vector of read-only memory containing samples
     * \param nsamps_per_buff the number of samples to send, per buffer
     * \param metadata data describing the buffer's contents
     * \param timeout the timeout in seconds to wait on a packet
     * \return the number of samples sent
     */
    virtual size_t send(const buffs_type& buffs,
        const size_t nsamps_per_buff,
        const tx_metadata_t& metadata,
        const double timeout = 0.1) = 0;

    /*!
     * Receive an asynchronous message from this TX stream.
     * \param async_metadata the metadata to be filled in
     * \param timeout the timeout in seconds to wait for a message
     * \return true when the async_metadata is valid, false for timeout
     */
    virtual bool recv_async_msg(
        async_metadata_t& async_metadata, double timeout = 0.1) = 0;
};

} // namespace uhd
