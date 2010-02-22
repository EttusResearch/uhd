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

#include <uhd/usrp/usrp2.hpp>
#include <uhd/device.hpp>
#include <boost/format.hpp>
#include <stdexcept>

using namespace uhd;

device_addrs_t device::discover(const device_addr_t &hint){
    device_addrs_t device_addrs;
    if (not hint.has_key("type")){
        //TODO call discover for others and append results
    }
    else if (hint["type"] == "udp"){
        std::vector<device_addr_t> usrp2_addrs = usrp::usrp2::discover(hint);
        device_addrs.insert(device_addrs.begin(), usrp2_addrs.begin(), usrp2_addrs.end());
    }
    return device_addrs;
}

device::sptr device::make(const device_addr_t &hint, size_t which){
    std::vector<device_addr_t> device_addrs = discover(hint);

    //check that we found any devices
    if (device_addrs.size() == 0){
        throw std::runtime_error(str(
            boost::format("No devices found for %s") % device_addr_to_string(hint)
        ));
    }

    //check that the which index is valid
    if (device_addrs.size() <= which){
        throw std::runtime_error(str(
            boost::format("No device at index %d for %s") % which % device_addr_to_string(hint)
        ));
    }

    //create the new device with the discovered address
    //TODO only a usrp2 device will be made (until others are supported)
    if (hint.has_key("type") and hint["type"] == "udp"){
        return usrp::usrp2::make(device_addrs.at(which));
    }
    throw std::runtime_error("cant make a device");
}

device::device(void){
    /* NOP */
}

device::~device(void){
    /* NOP */
}
