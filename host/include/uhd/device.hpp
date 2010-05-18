//
// Copyright 2010 Ettus Research LLC
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

#ifndef INCLUDED_UHD_DEVICE_HPP
#define INCLUDED_UHD_DEVICE_HPP

#include <uhd/config.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhd/types/metadata.hpp>
#include <uhd/types/io_type.hpp>
#include <uhd/wax.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/asio/buffer.hpp>

namespace uhd{

/*!
 * The usrp device interface represents the usrp hardware.
 * The api allows for discovery, configuration, and streaming.
 */
class UHD_API device : boost::noncopyable, public wax::obj{

public:
    typedef boost::shared_ptr<device> sptr;
    typedef boost::function<device_addrs_t(const device_addr_t &)> find_t;
    typedef boost::function<sptr(const device_addr_t &)> make_t;

    /*!
     * Register a device into the discovery and factory system.
     *
     * \param find a function that discovers devices
     * \param make a factory function that makes a device
     */
    static void register_device(
        const find_t &find,
        const make_t &make
    );

    /*!
     * \brief Find usrp devices attached to the host.
     *
     * The hint device address should be used to narrow down the search
     * to particular transport types and/or transport arguments.
     *
     * \param hint a partially (or fully) filled in device address
     * \return a vector of device addresses for all usrps on the system
     */
    static device_addrs_t find(const device_addr_t &hint);

    /*!
     * \brief Create a new usrp device from the device address hint.
     *
     * The make routine will call find and pick one of the results.
     * By default, the first result will be used to create a new device.
     * Use the which parameter as an index into the list of results.
     *
     * \param hint a partially (or fully) filled in device address
     * \param which which address to use when multiple are found
     * \return a shared pointer to a new device instance
     */
    static sptr make(const device_addr_t &hint, size_t which = 0);

    /*!
     * Send modes for the device send routine.
     */
    enum send_mode_t{
        //! Tells the send routine to send the entire buffer
        SEND_MODE_FULL_BUFF = 0,
        //! Tells the send routine to return after one packet
        SEND_MODE_ONE_PACKET = 1
    };

    /*!
     * Send a buffer containing IF data with its metadata.
     *
     * Send handles fragmentation as follows:
     * If the buffer has more samples than the maximum per packet,
     * the send method will fragment the samples across several packets.
     * Send will respect the burst flags when fragmenting to ensure
     * that start of burst can only be set on the first fragment and
     * that end of burst can only be set on the final fragment.
     * Fragmentation only applies in the full buffer send mode.
     *
     * This is a blocking call and will not return until the number
     * of samples returned have been read out of the buffer.
     *
     * \param buff a buffer pointing to some read-only memory
     * \param metadata data describing the buffer's contents
     * \param io_type the type of data loaded in the buffer
     * \param send_mode tells send how to unload the buffer
     * \return the number of samples sent
     */
    virtual size_t send(
        const boost::asio::const_buffer &buff,
        const tx_metadata_t &metadata,
        const io_type_t &io_type,
        send_mode_t send_mode = SEND_MODE_ONE_PACKET
    ) = 0;

    /*!
     * Recv modes for the device recv routine.
     */
    enum recv_mode_t{
        //! Tells the recv routine to recv the entire buffer
        RECV_MODE_FULL_BUFF = 0,
        //! Tells the recv routine to return after one packet
        RECV_MODE_ONE_PACKET = 1
    };

    /*!
     * Receive a buffer containing IF data and its metadata.
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
     * of samples returned have been written into the buffer.
     * However, a call to receive may timeout and return zero samples.
     * The timeout duration is decided by the underlying transport layer.
     * The caller should assume that the call to receive will not return
     * immediately when no packets are available to the transport layer,
     * and that the timeout duration is reasonably tuned for performance.
     *
     * When using the full buffer recv mode, the metadata only applies
     * to the first packet received and written into the recv buffer.
     * Use the one packet recv mode to get per packet metadata.
     *
     * \param buff the buffer to fill with IF data
     * \param metadata data to fill describing the buffer
     * \param io_type the type of data to fill into the buffer
     * \param recv_mode tells recv how to load the buffer
     * \return the number of samples received
     */
    virtual size_t recv(
        const boost::asio::mutable_buffer &buff,
        rx_metadata_t &metadata,
        const io_type_t &io_type,
        recv_mode_t recv_mode = RECV_MODE_ONE_PACKET
    ) = 0;
};

} //namespace uhd

#endif /* INCLUDED_UHD_DEVICE_HPP */
