//
// Copyright 2010-2011,2014-2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/device.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>

#include <uhd/utils/static.hpp>
#include <uhd/utils/algorithm.hpp>
#include <uhdlib/utils/prefs.hpp>


#include <boost/format.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/functional/hash.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/thread/mutex.hpp>

using namespace uhd;

static boost::mutex _device_mutex;

/***********************************************************************
 * Helper Functions
 **********************************************************************/
/*!
 * Make a device hash that maps 1 to 1 with a device address.
 * The hash will be used to identify created devices.
 * \param dev_addr the device address
 * \return the hash number
 */
static size_t hash_device_addr(
    const device_addr_t &dev_addr
){
    //combine the hashes of sorted keys/value pairs
    size_t hash = 0;

    // The device addr can contain all sorts of stuff, which sometimes gets in
    // the way of hashing reliably. TODO: Make this a whitelist
    const std::vector<std::string> hash_key_blacklist = {
        "claimed",
        "skip_dram",
        "skip_ddc",
        "skip_duc"
    };

    if(dev_addr.has_key("resource")) {
        boost::hash_combine(hash, "resource");
        boost::hash_combine(hash, dev_addr["resource"]);
    }
    else {
        for (const std::string &key: uhd::sorted(dev_addr.keys())) {
            if (std::find(hash_key_blacklist.begin(),
                          hash_key_blacklist.end(),
                          key) == hash_key_blacklist.end()) {
                boost::hash_combine(hash, key);
                boost::hash_combine(hash, dev_addr[key]);
            }
        }
    }
    return hash;
}

/***********************************************************************
 * Registration
 **********************************************************************/
typedef boost::tuple<device::find_t, device::make_t, device::device_filter_t> dev_fcn_reg_t;

// instantiate the device function registry container
UHD_SINGLETON_FCN(std::vector<dev_fcn_reg_t>, get_dev_fcn_regs)

void device::register_device(
    const find_t &find,
    const make_t &make,
    const device_filter_t filter
){
    // UHD_LOGGER_TRACE("UHD") << "registering device";
    get_dev_fcn_regs().push_back(dev_fcn_reg_t(find, make, filter));
}

device::~device(void){
    /* NOP */
}

/***********************************************************************
 * Discover
 **********************************************************************/
device_addrs_t device::find(const device_addr_t &hint, device_filter_t filter){
    boost::mutex::scoped_lock lock(_device_mutex);

    device_addrs_t device_addrs;

    for(const dev_fcn_reg_t &fcn:  get_dev_fcn_regs()) {
        try {
            if (filter == ANY or fcn.get<2>() == filter) {
                device_addrs_t discovered_addrs = fcn.get<0>()(hint);
                device_addrs.insert(
                    device_addrs.begin(),
                    discovered_addrs.begin(),
                    discovered_addrs.end()
                );
            }
        }
        catch (const std::exception &e) {
            UHD_LOGGER_ERROR("UHD") << "Device discovery error: " << e.what();
        }
    }

    return device_addrs;
}

/***********************************************************************
 * Make
 **********************************************************************/
device::sptr device::make(const device_addr_t &hint, device_filter_t filter, size_t which){
    boost::mutex::scoped_lock lock(_device_mutex);

    typedef boost::tuple<device_addr_t, make_t> dev_addr_make_t;
    std::vector<dev_addr_make_t> dev_addr_makers;

    for(const dev_fcn_reg_t &fcn:  get_dev_fcn_regs()){
        try{
            if(filter == ANY or fcn.get<2>() == filter){
                for(device_addr_t dev_addr:  fcn.get<0>()(hint)){
                    //append the discovered address and its factory function
                    dev_addr_makers.push_back(dev_addr_make_t(dev_addr, fcn.get<1>()));
                }
            }
        }
        catch(const std::exception &e){
            UHD_LOGGER_ERROR("UHD") << "Device discovery error: " << e.what() ;
        }
    }

    //check that we found any devices
    if (dev_addr_makers.size() == 0){
        throw uhd::key_error(str(
            boost::format("No devices found for ----->\n%s") % hint.to_pp_string()
        ));
    }

    //check that the which index is valid
    if (dev_addr_makers.size() <= which){
        throw uhd::index_error(str(
            boost::format("No device at index %d for ----->\n%s") % which % hint.to_pp_string()
        ));
    }

    //create a unique hash for the device address
    device_addr_t dev_addr; make_t maker;
    boost::tie(dev_addr, maker) = dev_addr_makers.at(which);
    size_t dev_hash = hash_device_addr(dev_addr);
    UHD_LOGGER_TRACE("UHD") << boost::format("Device hash: %u") % dev_hash ;

    //copy keys that were in hint but not in dev_addr
    //this way, we can pass additional transport arguments
    for(const std::string &key:  hint.keys()){
        if (not dev_addr.has_key(key)) dev_addr[key] = hint[key];
    }

    //map device address hash to created devices
    static uhd::dict<size_t, boost::weak_ptr<device> > hash_to_device;

    //try to find an existing device
    if (hash_to_device.has_key(dev_hash) and not hash_to_device[dev_hash].expired()){
        return hash_to_device[dev_hash].lock();
    }
    else {
        // Add keys from the config files (note: the user-defined keys will
        // always be applied, see also get_usrp_args()
        // Then, create and register a new device.
        device::sptr dev = maker(prefs::get_usrp_args(dev_addr));
        hash_to_device[dev_hash] = dev;
        return dev;
    }
}

uhd::property_tree::sptr
device::get_tree(void) const
{
    return _tree;
}

device::device_filter_t device::get_device_type() const {
    return _type;
}
