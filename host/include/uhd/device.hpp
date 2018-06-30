//
// Copyright 2010-2011,2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_DEVICE_HPP
#define INCLUDED_UHD_DEVICE_HPP

#include <uhd/config.hpp>
#include <uhd/stream.hpp>
#include <uhd/deprecated.hpp>
#include <uhd/property_tree.hpp>
#include <uhd/types/device_addr.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

namespace uhd{

class property_tree; //forward declaration

/*!
 * The device interface represents the hardware.
 * The API allows for discovery, configuration, and streaming.
 */
class UHD_API device : boost::noncopyable{

public:
    typedef boost::shared_ptr<device> sptr;
    typedef boost::function<device_addrs_t(const device_addr_t &)> find_t;
    typedef boost::function<sptr(const device_addr_t &)> make_t;

    //! Device type, used as a filter in make
    enum device_filter_t {
        ANY,
        USRP,
        CLOCK
    };
    virtual ~device(void) = 0;

    /*!
     * Register a device into the discovery and factory system.
     *
     * \param find a function that discovers devices
     * \param make a factory function that makes a device
     * \param filter include only USRP devices, clock devices, or both
     */
    static void register_device(
        const find_t &find,
        const make_t &make,
        const device_filter_t filter
    );

    /*!
     * \brief Find devices attached to the host.
     *
     * The hint device address should be used to narrow down the search
     * to particular transport types and/or transport arguments.
     *
     * \param hint a partially (or fully) filled in device address
     * \param filter an optional filter to exclude USRP or clock devices
     * \return a vector of device addresses for all devices on the system
     */
    static device_addrs_t find(const device_addr_t &hint, device_filter_t filter = ANY);

    /*!
     * \brief Create a new device from the device address hint.
     *
     * The method will go through the registered device types and pick one of
     * the discovered devices.
     *
     * By default, the first result will be used to create a new device.
     * Use the which parameter as an index into the list of results.
     *
     * \param hint a partially (or fully) filled in device address
     * \param filter an optional filter to exclude USRP or clock devices
     * \param which which address to use when multiple are found
     * \return a shared pointer to a new device instance
     */
    static sptr make(const device_addr_t &hint, device_filter_t filter = ANY, size_t which = 0);

    /*! \brief Make a new receive streamer from the streamer arguments
     *
     * Note: There can always only be one streamer. When calling get_rx_stream()
     * a second time, the first streamer must be destroyed beforehand.
     */
    virtual rx_streamer::sptr get_rx_stream(const stream_args_t &args) = 0;

    /*! \brief Make a new transmit streamer from the streamer arguments
     *
     * Note: There can always only be one streamer. When calling get_tx_stream()
     * a second time, the first streamer must be destroyed beforehand.
     */
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
    uhd::property_tree::sptr get_tree(void) const;

    //! Get device type
    device_filter_t get_device_type() const;

protected:
    uhd::property_tree::sptr _tree;
    device_filter_t _type;
};

} //namespace uhd

#endif /* INCLUDED_UHD_DEVICE_HPP */
