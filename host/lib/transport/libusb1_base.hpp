//
// Copyright 2010-2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_TRANSPORT_LIBUSB_HPP
#define INCLUDED_LIBUHD_TRANSPORT_LIBUSB_HPP

#include <uhd/config.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <uhd/transport/usb_device_handle.hpp>
#include <libusb.h>

//! Define LIBUSB_CALL when its missing (non-windows)
#ifndef LIBUSB_CALL
    #define LIBUSB_CALL
#endif /*LIBUSB_CALL*/

//! libusb_handle_events_timeout_completed is only in newer API
#ifndef HAVE_LIBUSB_HANDLE_EVENTS_TIMEOUT_COMPLETED
    #define libusb_handle_events_timeout_completed(ctx, tx, completed) \
        libusb_handle_events_timeout(ctx, tx)
#endif /* HAVE_LIBUSB_HANDLE_EVENTS_TIMEOUT_COMPLETED */

//! libusb_error_name is only in newer API
#ifndef HAVE_LIBUSB_ERROR_NAME
    #define libusb_error_name(code) \
        str(boost::format("LIBUSB_ERROR_CODE %d") % code)
#endif /* HAVE_LIBUSB_ERROR_NAME */

//! libusb_strerror is only in newer API
#ifndef HAVE_LIBUSB_STRERROR
    #define libusb_strerror(code) \
        libusb_error_name(code)
#endif /* HAVE_LIBUSB_STRERROR */

/***********************************************************************
 * Libusb object oriented smart pointer wrappers:
 * The following wrappers provide allocation and automatic deallocation
 * for various libusb data types and handles. The construction routines
 * also store tables of already allocated structures to avoid multiple
 * occurrences of opened handles (for example).
 **********************************************************************/
namespace uhd { namespace transport {

namespace libusb {

    /*!
     * This session class holds a global libusb context for this process.
     * The get global session call will create a new context if none exists.
     * When all references to session are destroyed, the context will be freed.
     */
    class session : boost::noncopyable {
    public:
        typedef boost::shared_ptr<session> sptr;

        virtual ~session(void);

        /*!
         *   Level 0: no messages ever printed by the library (default)
         *   Level 1: error messages are printed to stderr
         *   Level 2: warning and error messages are printed to stderr
         *   Level 3: informational messages are printed to stdout, warning
         *            and error messages are printed to stderr
         */
        static const int debug_level = 0;

        //! get a shared pointer to the global session
        static sptr get_global_session(void);

        //! get the underlying libusb context pointer
        virtual libusb_context *get_context(void) const = 0;
    };

    /*!
     * Holds a device pointer with a reference to the session.
     */
    class device : boost::noncopyable {
    public:
        typedef boost::shared_ptr<device> sptr;

        virtual ~device(void);

        //! get the underlying device pointer
        virtual libusb_device *get(void) const = 0;
    };

    /*!
     * This device list class holds a device list that will be
     * automatically freed when the last reference is destroyed.
     */
    class device_list : boost::noncopyable {
    public:
        typedef boost::shared_ptr<device_list> sptr;

        virtual ~device_list(void);

        //! make a new device list
        static sptr make(void);

        //! the number of devices in this list
        virtual size_t size() const = 0;

        //! get the device pointer at a particular index
        virtual device::sptr at(size_t index) const = 0;
    };

    /*!
     * Holds a device descriptor and a reference to the device.
     */
    class device_descriptor : boost::noncopyable {
    public:
        typedef boost::shared_ptr<device_descriptor> sptr;

        virtual ~device_descriptor(void);

        //! make a new descriptor from a device reference
        static sptr make(device::sptr);

        //! get the underlying device descriptor
        virtual const libusb_device_descriptor &get(void) const = 0;

        virtual std::string get_ascii_property(const std::string &what) const = 0;
    };

    /*!
     * Holds a device handle and a reference to the device.
     */
    class device_handle : boost::noncopyable {
    public:
        typedef boost::shared_ptr<device_handle> sptr;

        virtual ~device_handle(void);

        //! get a cached handle or make a new one given the device
        static sptr get_cached_handle(device::sptr);

        //! get the underlying device handle
        virtual libusb_device_handle *get(void) const = 0;

        /*!
         * Open USB interfaces for control using magic value
         * IN interface:      2
         * OUT interface:     1
         * Control interface: 0
         */
        virtual void claim_interface(int) = 0;

        virtual void clear_endpoints(unsigned char recv_endpoint, unsigned char send_endpoint) = 0;

        virtual void reset_device(void) = 0;
    };

    /*!
     * The special handle is our internal implementation of the
     * usb device handle which is used publicly to identify a device.
     */
    class special_handle : public usb_device_handle {
    public:
        typedef boost::shared_ptr<special_handle> sptr;

        virtual ~special_handle(void);

        //! make a new special handle from device
        static sptr make(device::sptr);

        //! get the underlying device reference
        virtual device::sptr get_device(void) const = 0;
    };

}

}} //namespace

#endif /* INCLUDED_LIBUHD_TRANSPORT_LIBUSB_HPP */
