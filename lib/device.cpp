//
// Copyright 2010 Ettus Research LLC
//

#include <usrp_uhd/device.hpp>
#include <boost/format.hpp>

using namespace usrp_uhd;

std::vector<device_addr_t> device::discover(const device_addr_t& hint){
    std::vector<device_addr_t> device_addrs;
    if (hint.type == DEVICE_ADDR_TYPE_VIRTUAL){
        //TODO device_addrs.push_back(...);
    }
    return device_addrs;
}

device::sptr device::make(const device_addr_t& hint, size_t which){
    std::vector<device_addr_t> device_addrs = discover(hint);
    //check that we found any devices
    if (device_addrs.size() == 0){
        throw std::runtime_error(str(
            boost::format("No devices found for %s") % hint.to_string()
        ));
    }
    //check that the which index is valid
    if (device_addrs.size() <= which){
        throw std::runtime_error(str(
            boost::format("No device at index %d for %s")
            % which % hint.to_string()
        ));
    }
    //create the new device with the discovered address
    return sptr(new device(device_addrs.at(which)));
}

device::device(const device_addr_t&){
    /* NOP */
}

device::~device(void){
    /* NOP */
}
