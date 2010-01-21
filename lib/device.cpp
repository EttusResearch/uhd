//
// Copyright 2010 Ettus Research LLC
//

#include <usrp_uhd/usrp/usrp.hpp>
#include <usrp_uhd/device.hpp>
#include <boost/format.hpp>
#include <stdexcept>

using namespace usrp_uhd;

std::vector<device_addr_t> device::discover(const device_addr_t & hint = device_addr_t()){
    std::vector<device_addr_t> device_addrs;
    if (hint.type == DEVICE_ADDR_TYPE_VIRTUAL){
        //make a copy of the hint for virtual testing
        device_addr_t virtual_device_addr = hint;
        device_addrs.push_back(virtual_device_addr);
    }
    return device_addrs;
}

device::sptr device::make(const device_addr_t & hint, size_t which){
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
    if (hint.type == DEVICE_ADDR_TYPE_VIRTUAL){
        return sptr(new usrp_uhd::usrp::usrp(device_addrs.at(which)));
    }
    throw std::runtime_error("cant make a device");
}

device::device(void){
    /* NOP */
}

device::~device(void){
    /* NOP */
}
