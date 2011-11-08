//
// Copyright 2010-2011 Ettus Research LLC
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
#include <uhd/stream.hpp>
#include <uhd/deprecated.hpp>
#include <uhd/types/device_addr.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

namespace uhd{

class property_tree; //forward declaration

/*!
 * The usrp device interface represents the usrp hardware.
 * The api allows for discovery, configuration, and streaming.
 */
class UHD_API device : boost::noncopyable{

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

    //! Make a new receive streamer from the streamer arguments
    virtual rx_streamer::sptr get_rx_stream(const stream_args_t &args) = 0;

    //! Make a new transmit streamer from the streamer arguments
    virtual tx_streamer::sptr get_tx_stream(const stream_args_t &args) = 0;

    /*!
     * Receive and asynchronous message from the device.
     * \param async_metadata the metadata to be filled in
     * \param timeout the timeout in seconds to wait for a message
     * \return true when the async_metadata is valid, false for timeout
     */
    virtual bool recv_async_msg(
        async_metadata_t &async_metadata, double timeout = 0.1
    ) = 0;

    //! Get access to the underlying property structure
    virtual boost::shared_ptr<property_tree> get_tree(void) const = 0;

    #include <uhd/device_deprecated.ipp>

};

} //namespace uhd

#endif /* INCLUDED_UHD_DEVICE_HPP */
