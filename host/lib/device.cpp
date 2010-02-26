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
// asize_t with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <uhd/usrp/usrp1e.hpp>
#include <uhd/usrp/usrp2.hpp>
#include <uhd/dict.hpp>
#include <uhd/utils.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/functional/hash.hpp>
#include <stdexcept>
#include <algorithm>

using namespace uhd;

/*!
 * Create a new device from a device address.
 * Based on the address, call the appropriate make functions.
 * \param dev_addr the device address
 * \return a smart pointer to a device
 */
static device::sptr make_device(const device_addr_t &dev_addr){

    //create a usrp1e
    if (dev_addr["type"] == "usrp1e"){
        return usrp::usrp1e::make(dev_addr);
    }

    //create a usrp2
    if (dev_addr["type"] == "usrp2"){
        return usrp::usrp2::make(dev_addr);
    }

    throw std::runtime_error("cant make a device");
}

/*!
 * Make a device hash that maps 1 to 1 with a device address.
 * The hash will be used to identify created devices.
 * \param dev_addr the device address
 * \return the hash number
 */
static size_t hash_device_addr(
    const device_addr_t &dev_addr
){
    //sort the keys of the device address
    std::vector<std::string> keys = dev_addr.get_keys();
    std::sort(keys.begin(), keys.end());

    //combine the hashes of sorted keys/value pairs
    size_t hash = 0;
    BOOST_FOREACH(std::string key, keys){
        boost::hash_combine(hash, key);
        boost::hash_combine(hash, dev_addr[key]);
    }
    return hash;
}

/***********************************************************************
 * Discover
 **********************************************************************/
device_addrs_t device::discover(const device_addr_t &hint){
    device_addrs_t device_addrs;

    //discover the usrp1es
    std::vector<device_addr_t> usrp2_addrs = usrp::usrp1e::discover(hint);
    device_addrs.insert(device_addrs.begin(), usrp2_addrs.begin(), usrp2_addrs.end());

    //discover the usrp2s
    if (hint.has_key("addr")){
        std::vector<device_addr_t> usrp2_addrs = usrp::usrp2::discover(hint);
        device_addrs.insert(device_addrs.begin(), usrp2_addrs.begin(), usrp2_addrs.end());
    }

    return device_addrs;
}

/***********************************************************************
 * Make
 **********************************************************************/
device::sptr device::make(const device_addr_t &hint, size_t which){
    std::vector<device_addr_t> device_addrs = discover(hint);

    //check that we found any devices
    if (device_addrs.size() == 0){
        throw std::runtime_error(str(
            boost::format("No devices found for %s") % device_addr::to_string(hint)
        ));
    }

    //check that the which index is valid
    if (device_addrs.size() <= which){
        throw std::runtime_error(str(
            boost::format("No device at index %d for %s") % which % device_addr::to_string(hint)
        ));
    }

    //create a unique hash for the device address
    device_addr_t dev_addr = device_addrs.at(which);
    size_t dev_hash = hash_device_addr(dev_addr);
    //std::cout << boost::format("Hash: %u") % dev_hash << std::endl;

    //map device address hash to created devices
    static uhd::dict<size_t, boost::weak_ptr<device> > hash_to_device;

    //try to find an existing device
    try{
        ASSERT_THROW(hash_to_device.has_key(dev_hash));
        ASSERT_THROW(not hash_to_device[dev_hash].expired());
        return hash_to_device[dev_hash].lock();
    }
    //create and register a new device
    catch(const std::assert_error &e){
        device::sptr dev = make_device(dev_addr);
        hash_to_device[dev_hash] = dev;
        return dev;
    }
}
