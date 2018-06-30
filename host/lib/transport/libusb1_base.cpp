//
// Copyright 2010-2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "libusb1_base.hpp"
#include <uhd/exception.hpp>

#include <uhd/utils/log.hpp>
#include <uhd/utils/tasks.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/types/serial.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/bind.hpp>
#include <cstdlib>
#include <iostream>

using namespace uhd;
using namespace uhd::transport;

/***********************************************************************
 * libusb session
 **********************************************************************/
libusb::session::~session(void) {
    /* NOP */
}

class libusb_session_impl : public libusb::session{
public:
    libusb_session_impl(void){
        UHD_ASSERT_THROW(libusb_init(&_context) == 0);
        libusb_set_debug(_context, debug_level);
        task_handler = task::make(boost::bind(&libusb_session_impl::libusb_event_handler_task, this, _context));
    }

    virtual ~libusb_session_impl(void);

    libusb_context *get_context(void) const{
        return _context;
    }

private:
    libusb_context *_context;
    task::sptr task_handler;

    /*
     * Task to handle libusb events.  There should only be one thread per libusb_context handling events.
     * Using more than one thread can result in excessive CPU usage in kernel space (presumably from locking/waiting).
     * The libusb documentation says it is safe, which it is, but it neglects to state the cost in CPU usage.
     * Just don't do it!
     */
    UHD_INLINE void libusb_event_handler_task(libusb_context *context)
    {
        timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000;
        int ret = libusb_handle_events_timeout(context, &tv);
        switch (ret)
        {
        case LIBUSB_SUCCESS:
        case LIBUSB_ERROR_TIMEOUT:
            break;
        case LIBUSB_ERROR_NO_DEVICE:
            throw uhd::io_error(libusb_strerror(LIBUSB_ERROR_NO_DEVICE));
        default:
            UHD_LOGGER_ERROR("USB") << __FUNCTION__ << ": " << libusb_strerror((libusb_error)ret) ;
            break;
        }
    }
};

libusb_session_impl::~libusb_session_impl(void){
    task_handler.reset();
    libusb_exit(_context);
}

libusb::session::sptr libusb::session::get_global_session(void){
    static boost::weak_ptr<session> global_session;

    //not expired -> get existing session
    if (not global_session.expired()) return global_session.lock();

    //create a new global session
    sptr new_global_session(new libusb_session_impl());
    global_session = new_global_session;

    //set logging if envvar is set
    const char *level_string = getenv("LIBUSB_DEBUG_LEVEL");
    if (level_string != NULL)
    {
        const int level = int(level_string[0] - '0'); //easy conversion to integer
        if (level >= 0 and level <= 3) libusb_set_debug(new_global_session->get_context(), level);
    }

    return new_global_session;
}

/***********************************************************************
 * libusb device
 **********************************************************************/
libusb::device::~device(void) {
    /* NOP */
}

class libusb_device_impl : public libusb::device{
public:
    libusb_device_impl(libusb_device *dev){
        _session = libusb::session::get_global_session();
        _dev = dev;
    }

    virtual ~libusb_device_impl(void);

    libusb_device *get(void) const{
        return _dev;
    }

private:
    libusb::session::sptr _session; //always keep a reference to session
    libusb_device *_dev;
};

libusb_device_impl::~libusb_device_impl(void){
    libusb_unref_device(this->get());
}

/***********************************************************************
 * libusb device list
 **********************************************************************/
libusb::device_list::~device_list(void){
    /* NOP */
}

class libusb_device_list_impl : public libusb::device_list{
public:
    libusb_device_list_impl(void){
        libusb::session::sptr sess = libusb::session::get_global_session();

        //allocate a new list of devices
        libusb_device** dev_list;
        ssize_t ret = libusb_get_device_list(sess->get_context(), &dev_list);
        if (ret < 0) throw uhd::os_error("cannot enumerate usb devices");

        //fill the vector of device references
        for (size_t i = 0; i < size_t(ret); i++) _devs.push_back(
            libusb::device::sptr(new libusb_device_impl(dev_list[i]))
        );

        //free the device list but dont unref (done in ~device)
        libusb_free_device_list(dev_list, false/*dont unref*/);
    }

    virtual ~libusb_device_list_impl(void);

    size_t size(void) const{
        return _devs.size();
    }

    libusb::device::sptr at(size_t i) const{
        return _devs.at(i);
    }

private:
    std::vector<libusb::device::sptr> _devs;
};

libusb_device_list_impl::~libusb_device_list_impl(void){
    /* NOP */
}

libusb::device_list::sptr libusb::device_list::make(void){
    return sptr(new libusb_device_list_impl());
}

/***********************************************************************
 * libusb device descriptor
 **********************************************************************/
libusb::device_descriptor::~device_descriptor(void){
    /* NOP */
}

class libusb_device_descriptor_impl : public libusb::device_descriptor{
public:
    libusb_device_descriptor_impl(libusb::device::sptr dev){
        _dev = dev;
        UHD_ASSERT_THROW(libusb_get_device_descriptor(_dev->get(), &_desc) == 0);
    }

    virtual ~libusb_device_descriptor_impl(void);

    const libusb_device_descriptor &get(void) const{
        return _desc;
    }

    std::string get_ascii_property(const std::string &what) const
    {
        uint8_t off = 0;
        if (what == "serial") off = this->get().iSerialNumber;
        if (what == "product") off = this->get().iProduct;
        if (what == "manufacturer") off = this->get().iManufacturer;
        if (off == 0) return "";

        libusb::device_handle::sptr handle(
            libusb::device_handle::get_cached_handle(_dev)
        );

        unsigned char buff[512];
        int ret = libusb_get_string_descriptor_ascii(
            handle->get(), off, buff, int(sizeof(buff))
        );
        if (ret < 0) return ""; //on error, just return empty string

        std::string string_descriptor((char *)buff, size_t(ret));
        byte_vector_t string_vec(string_descriptor.begin(), string_descriptor.end());
        std::string out;
        for(uint8_t byte:  string_vec){
            if (byte < 32 or byte > 127) return out;
            out += byte;
        }
        return out;
    }

private:
    libusb::device::sptr _dev; //always keep a reference to device
    libusb_device_descriptor _desc;
};

libusb_device_descriptor_impl::~libusb_device_descriptor_impl(void){
    /* NOP */
}

libusb::device_descriptor::sptr libusb::device_descriptor::make(device::sptr dev){
    return sptr(new libusb_device_descriptor_impl(dev));
}

/***********************************************************************
 * libusb device handle
 **********************************************************************/
libusb::device_handle::~device_handle(void){
    /* NOP */
}

class libusb_device_handle_impl : public libusb::device_handle{
public:
    libusb_device_handle_impl(libusb::device::sptr dev){
        _dev = dev;
        UHD_ASSERT_THROW(libusb_open(_dev->get(), &_handle) == 0);
    }

    virtual ~libusb_device_handle_impl(void);

    libusb_device_handle *get(void) const{
        return _handle;
    }

    void claim_interface(int interface){
        UHD_ASSERT_THROW(libusb_claim_interface(this->get(), interface) == 0);
        _claimed.push_back(interface);
    }

    void clear_endpoints(unsigned char recv_endpoint, unsigned char send_endpoint)
    {
        int ret;
        ret = libusb_clear_halt(this->get(), recv_endpoint  | 0x80);
        UHD_LOGGER_TRACE("USB") << "usb device handle: recv endpoint clear: " << libusb_error_name(ret) ;
        ret = libusb_clear_halt(this->get(), send_endpoint | 0x00);
        UHD_LOGGER_TRACE("USB") << "usb device handle: send endpoint clear: " << libusb_error_name(ret) ;
    }

    void reset_device(void)
    {
        int ret = libusb_reset_device(this->get());
        UHD_LOGGER_TRACE("USB") << "usb device handle: dev Reset: " << libusb_error_name(ret) ;
    }

private:
    libusb::device::sptr _dev; //always keep a reference to device
    libusb_device_handle *_handle;
    std::vector<int> _claimed;
};

libusb_device_handle_impl::~libusb_device_handle_impl(void){
    //release all claimed interfaces
    for (size_t i = 0; i < _claimed.size(); i++){
        libusb_release_interface(this->get(), _claimed[i]);
    }
    libusb_close(_handle);
}

libusb::device_handle::sptr libusb::device_handle::get_cached_handle(device::sptr dev){
    static uhd::dict<libusb_device *, boost::weak_ptr<device_handle> > handles;

    //lock for atomic access to static table above
    static boost::mutex mutex;
    boost::mutex::scoped_lock lock(mutex);

    //not expired -> get existing handle
    if (handles.has_key(dev->get()) and not handles[dev->get()].expired()){
        return handles[dev->get()].lock();
    }

    //create a new cached handle
    try{
        sptr new_handle(new libusb_device_handle_impl(dev));
        handles[dev->get()] = new_handle;
        return new_handle;
    }
    catch(const uhd::exception &){
        #ifdef UHD_PLATFORM_LINUX
        UHD_LOGGER_ERROR("USB") <<
            "USB open failed: insufficient permissions.\n"
            "See the application notes for your device.\n"
        ;
        #else
        UHD_LOGGER_DEBUG("USB") << "USB open failed: device already claimed." ;
        #endif
        throw;
    }
}

/***********************************************************************
 * libusb special handle
 **********************************************************************/
libusb::special_handle::~special_handle(void){
    /* NOP */
}

class libusb_special_handle_impl : public libusb::special_handle{
public:
    libusb_special_handle_impl(libusb::device::sptr dev){
        _dev = dev;
    }

    virtual ~libusb_special_handle_impl(void);

    libusb::device::sptr get_device(void) const{
        return _dev;
    }

    std::string get_serial(void) const{
        return libusb::device_descriptor::make(this->get_device())->get_ascii_property("serial");
    }

    std::string get_manufacturer() const{
        return libusb::device_descriptor::make(this->get_device())->get_ascii_property("manufacturer");
    }

    std::string get_product() const{
        return libusb::device_descriptor::make(this->get_device())->get_ascii_property("product");
    }

    uint16_t get_vendor_id(void) const{
        return libusb::device_descriptor::make(this->get_device())->get().idVendor;
    }

    uint16_t get_product_id(void) const{
        return libusb::device_descriptor::make(this->get_device())->get().idProduct;
    }

    bool firmware_loaded() {
        return (get_manufacturer() == "Ettus Research LLC") or
               (get_manufacturer() == "National Instruments Corp.") or
               (get_manufacturer() == "Free Software Folks");
    }

private:
    libusb::device::sptr _dev; //always keep a reference to device
};

libusb_special_handle_impl::~libusb_special_handle_impl(void){
    /* NOP */
}

libusb::special_handle::sptr libusb::special_handle::make(device::sptr dev){
    return sptr(new libusb_special_handle_impl(dev));
}

/***********************************************************************
 * list device handles implementations
 **********************************************************************/
usb_device_handle::~usb_device_handle(void) {
    /* NOP */
}

std::vector<usb_device_handle::sptr> usb_device_handle::get_device_list(
    uint16_t vid, uint16_t pid
){
    return usb_device_handle::get_device_list(std::vector<usb_device_handle::vid_pid_pair_t>(1,usb_device_handle::vid_pid_pair_t(vid,pid)));
}

std::vector<usb_device_handle::sptr> usb_device_handle::get_device_list(const std::vector<usb_device_handle::vid_pid_pair_t>& vid_pid_pair_list)
{
    std::vector<usb_device_handle::sptr> handles;
    libusb::device_list::sptr dev_list = libusb::device_list::make();
    for(size_t iter = 0; iter < vid_pid_pair_list.size(); ++iter)
    {
       for (size_t i = 0; i < dev_list->size(); i++){
           usb_device_handle::sptr handle = libusb::special_handle::make(dev_list->at(i));
           if (handle->get_vendor_id() == vid_pid_pair_list[iter].first and handle->get_product_id() == vid_pid_pair_list[iter].second){
               handles.push_back(handle);
           }
       }
    }
    return handles;
}
