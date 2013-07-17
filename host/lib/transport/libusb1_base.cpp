//
// Copyright 2010-2013 Ettus Research LLC
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

#include "libusb1_base.hpp"
#include <uhd/exception.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/types/dict.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/foreach.hpp>
#include <cstdlib>
#include <iostream>

using namespace uhd;
using namespace uhd::transport;

/***********************************************************************
 * libusb session
 **********************************************************************/
class libusb_session_impl : public libusb::session{
public:
    libusb_session_impl(void){
        UHD_ASSERT_THROW(libusb_init(&_context) == 0);
        libusb_set_debug(_context, debug_level);
    }

    ~libusb_session_impl(void){
        libusb_exit(_context);
    }

    libusb_context *get_context(void) const{
        return _context;
    }

private:
    libusb_context *_context;
};

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
class libusb_device_impl : public libusb::device{
public:
    libusb_device_impl(libusb_device *dev){
        _session = libusb::session::get_global_session();
        _dev = dev;
    }

    ~libusb_device_impl(void){
        libusb_unref_device(this->get());
    }

    libusb_device *get(void) const{
        return _dev;
    }

private:
    libusb::session::sptr _session; //always keep a reference to session
    libusb_device *_dev;
};

/***********************************************************************
 * libusb device list
 **********************************************************************/
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

    size_t size(void) const{
        return _devs.size();
    }

    libusb::device::sptr at(size_t i) const{
        return _devs.at(i);
    }

private:
    std::vector<libusb::device::sptr> _devs;
};

libusb::device_list::sptr libusb::device_list::make(void){
    return sptr(new libusb_device_list_impl());
}

/***********************************************************************
 * libusb device descriptor
 **********************************************************************/
class libusb_device_descriptor_impl : public libusb::device_descriptor{
public:
    libusb_device_descriptor_impl(libusb::device::sptr dev){
        _dev = dev;
        UHD_ASSERT_THROW(libusb_get_device_descriptor(_dev->get(), &_desc) == 0);
    }

    const libusb_device_descriptor &get(void) const{
        return _desc;
    }

    std::string get_ascii_property(const std::string &what) const
    {
        boost::uint8_t off = 0;
        if (what == "serial") off = this->get().iSerialNumber;
        if (what == "product") off = this->get().iProduct;
        if (what == "manufacturer") off = this->get().iManufacturer;
        if (off == 0) return "";

        libusb::device_handle::sptr handle(
            libusb::device_handle::get_cached_handle(_dev)
        );

        unsigned char buff[512];
        ssize_t ret = libusb_get_string_descriptor_ascii(
            handle->get(), off, buff, sizeof(buff)
        );
        if (ret < 0) return ""; //on error, just return empty string

        return std::string((char *)buff, ret);
    }

private:
    libusb::device::sptr _dev; //always keep a reference to device
    libusb_device_descriptor _desc;
};

libusb::device_descriptor::sptr libusb::device_descriptor::make(device::sptr dev){
    return sptr(new libusb_device_descriptor_impl(dev));
}

/***********************************************************************
 * libusb device handle
 **********************************************************************/
class libusb_device_handle_impl : public libusb::device_handle{
public:
    libusb_device_handle_impl(libusb::device::sptr dev){
        _dev = dev;
        UHD_ASSERT_THROW(libusb_open(_dev->get(), &_handle) == 0);
    }

    ~libusb_device_handle_impl(void){
        //release all claimed interfaces
        for (size_t i = 0; i < _claimed.size(); i++){
            libusb_release_interface(this->get(), _claimed[i]);
        }
        libusb_close(_handle);
    }

    libusb_device_handle *get(void) const{
        return _handle;
    }

    void claim_interface(int interface){
        UHD_ASSERT_THROW(libusb_claim_interface(this->get(), interface) == 0);
        _claimed.push_back(interface);
    }

private:
    libusb::device::sptr _dev; //always keep a reference to device
    libusb_device_handle *_handle;
    std::vector<int> _claimed;
};

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
        UHD_MSG(error) <<
            "USB open failed: insufficient permissions.\n"
            "See the application notes for your device.\n"
        << std::endl;
        #else
        UHD_LOG << "USB open failed: device already claimed." << std::endl;
        #endif
        throw;
    }
}

/***********************************************************************
 * libusb special handle
 **********************************************************************/
class libusb_special_handle_impl : public libusb::special_handle{
public:
    libusb_special_handle_impl(libusb::device::sptr dev){
        _dev = dev;
    }

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

    boost::uint16_t get_vendor_id(void) const{
        return libusb::device_descriptor::make(this->get_device())->get().idVendor;
    }

    boost::uint16_t get_product_id(void) const{
        return libusb::device_descriptor::make(this->get_device())->get().idProduct;
    }

private:
    libusb::device::sptr _dev; //always keep a reference to device
};

libusb::special_handle::sptr libusb::special_handle::make(device::sptr dev){
    return sptr(new libusb_special_handle_impl(dev));
}

/***********************************************************************
 * list device handles implementations
 **********************************************************************/
std::vector<usb_device_handle::sptr> usb_device_handle::get_device_list(
    boost::uint16_t vid, boost::uint16_t pid
){
    std::vector<usb_device_handle::sptr> handles;

    libusb::device_list::sptr dev_list = libusb::device_list::make();
    for (size_t i = 0; i < dev_list->size(); i++){
        usb_device_handle::sptr handle = libusb::special_handle::make(dev_list->at(i));
        if (handle->get_vendor_id() == vid and handle->get_product_id() == pid){
            handles.push_back(handle);
        }
    }

    return handles;
}
