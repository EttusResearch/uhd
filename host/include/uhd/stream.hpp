//
// Copyright 2011 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef INCLUDED_UHD_STREAM_HPP
#define INCLUDED_UHD_STREAM_HPP

#include <uhd/config.hpp>
#include <uhd/types/metadata.hpp>
#include <uhd/types/ref_vector.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <vector>
#include <string>

namespace uhd{

/*!
 * A struct of parameters to construct a streamer.
 *
 * Note:
 * Not all combinations of CPU and OTW format have conversion support.
 * You may however write and register your own conversion routines.
 */
struct UHD_API stream_args_t{

    //! Convenience constructor for streamer args
    stream_args_t(
        const std::string &cpu = "",
        const std::string &otw = ""
    ){
        cpu_format = cpu;
        otw_format = otw;
    }

    /*!
     * The CPU format is a string that describes the format of host memory.
     * Common CPU formats are:
     *  - fc32 - complex<float>
     *  - fc64 - complex<double>
     *  - sc16 - complex<int16_t>
     *  - sc8 - complex<int8_t>
     *  - f32 - float
     *  - f64 - double
     *  - s16 - int16_t
     *  - s8 - int8_t
     */
    std::string cpu_format;

    /*!
     * The OTW format is a string that describes the format over-the-wire.
     * Common OTW format are:
     *  - sc16 - Q16 I16
     *  - sc8 - Q8_1 I8_1 Q8_0 I8_0
     *  - s16 - R16_1 R16_0
     *  - s8 - R8_3 R8_2 R8_1 R8_0
     */
    std::string otw_format;

    /*!
     * The args parameter is currently unused. Leave it blank.
     * The intention is that a user with a custom DSP design
     * may want to pass args and do something special with it.
     */
    std::string args;

    /*!
     * The channels is a list of channel numbers.
     * Leave this blank to default to channel 0.
     * Set channels for a multi-channel application.
     * Channel mapping depends on the front-end selection.
     */
    std::vector<size_t> channels;
};

/*!
 * The RX streamer is the host interface to receiving samples.
 * It represents the layer between the samples on the host
 * and samples inside the device's receive DSP processing.
 */
class UHD_API rx_streamer : boost::noncopyable{
public:
    typedef boost::shared_ptr<rx_streamer> sptr;

    //! Get the number of channels associated with this streamer
    virtual size_t get_num_channels(void) const = 0;

    //! Get the max number of samples per buffer per packet
    virtual size_t get_max_num_samps(void) const = 0;

    //! Typedef for a pointer to a single, or a collection of recv buffers
    typedef ref_vector<void *> buffs_type;

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
     * The next call to receive, after the remainder becomes exahausted,
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
     * \param buffs a vector of writable memory to fill with samples
     * \param nsamps_per_buff the size of each buffer in number of samples
     * \param metadata data to fill describing the buffer
     * \param timeout the timeout in seconds to wait for a packet
     * \param one_packet return after the first packet is received
     * \return the number of samples received or 0 on error
     */
    virtual size_t recv(
        const buffs_type &buffs,
        const size_t nsamps_per_buff,
        rx_metadata_t &metadata,
        const double timeout = 0.1,
        const bool one_packet = false
    ) = 0;
};

/*!
 * The TX streamer is the host interface to transmitting samples.
 * It represents the layer between the samples on the host
 * and samples inside the device's transmit DSP processing.
 */
class UHD_API tx_streamer : boost::noncopyable{
public:
    typedef boost::shared_ptr<tx_streamer> sptr;

    //! Get the number of channels associated with this streamer
    virtual size_t get_num_channels(void) const = 0;

    //! Get the max number of samples per buffer per packet
    virtual size_t get_max_num_samps(void) const = 0;

    //! Typedef for a pointer to a single, or a collection of send buffers
    typedef ref_vector<const void *> buffs_type;

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
     * \param buffs a vector of read-only memory containing samples
     * \param nsamps_per_buff the number of samples to send, per buffer
     * \param metadata data describing the buffer's contents
     * \param timeout the timeout in seconds to wait on a packet
     * \return the number of samples sent
     */
    virtual size_t send(
        const buffs_type &buffs,
        const size_t nsamps_per_buff,
        const tx_metadata_t &metadata,
        const double timeout = 0.1
    ) = 0;
};

} //namespace uhd

#endif /* INCLUDED_UHD_STREAM_HPP */
