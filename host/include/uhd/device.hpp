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

#include <uhd/device_addr.hpp>
#include <uhd/props.hpp>
#include <uhd/metadata.hpp>
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
class device : boost::noncopyable, public wax::obj{

public:
    typedef boost::shared_ptr<device> sptr;

    /*!
     * \brief Discover usrp devices attached to the host.
     *
     * The hint device address should be used to narrow down the search
     * to particular transport types and/or transport arguments.
     *
     * \param hint a partially (or fully) filled in device address
     * \return a vector of device addresses for all usrps on the system
     */
    static device_addrs_t discover(const device_addr_t &hint);

    /*!
     * \brief Create a new usrp device from the device address hint.
     *
     * The make routine will call discover and pick one of the results.
     * By default, the first result will be used to create a new device.
     * Use the which parameter as an index into the list of results.
     *
     * \param hint a partially (or fully) filled in device address
     * \param which which address to use when multiple are discovered
     * \return a shared pointer to a new device instance
     */
    static sptr make(const device_addr_t &hint, size_t which = 0);

    /*!
     * Get the device address for this board.
     */
    device_addr_t get_device_addr(void);

    /*!
     * Send a buffer containing IF data with its metadata.
     *
     * Send handles fragmentation as follows:
     * If the buffer has more samples than the maximum supported,
     * the send method will send the maximum number of samples
     * as supported by the transport and return the number sent.
     * It is up to the caller to call send again on the un-sent
     * portions of the buffer, until the buffer is exhausted.
     *
     * This is a blocking call and will not return until the number
     * of samples returned have been read out of the buffer.
     *
     * \param buff a buffer pointing to some read-only memory
     * \param metadata data describing the buffer's contents
     * \param the type of data loaded in the buffer (32fc, 16sc)
     * \return the number of samples sent
     */
    virtual size_t send(
        const boost::asio::const_buffer &buff,
        const tx_metadata_t &metadata,
        const std::string &type = "32fc"
    ) = 0;

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
     *
     * This is a blocking call and will not return until the number
     * of samples returned have been written into the buffer.
     * However, a call to receive may timeout and return zero samples.
     * The timeout duration is decided by the underlying transport layer.
     * The caller should assume that the call to receive will not return
     * immediately when no packets are available to the transport layer,
     * and that the timeout duration is reasonably tuned for performance.
     *
     * \param buff the buffer to fill with IF data
     * \param metadata data to fill describing the buffer
     * \param the type of data to fill into the buffer (32fc, 16sc)
     * \return the number of samples received
     */
    virtual size_t recv(
        const boost::asio::mutable_buffer &buff,
        rx_metadata_t &metadata,
        const std::string &type = "32fc"
    ) = 0;
};

} //namespace uhd

#endif /* INCLUDED_UHD_DEVICE_HPP */
