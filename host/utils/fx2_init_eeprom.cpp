//
// Copyright 2010,2014 Ettus Research LLC
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

#include <uhd/utils/safe_main.hpp>
#include <uhd/device.hpp>
#include <uhd/property_tree.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <cstdlib>

const std::string FX2_VENDOR_ID("0x04b4");
const std::string FX2_PRODUCT_ID("0x8613");

namespace po = boost::program_options;

int UHD_SAFE_MAIN(int argc, char *argv[]){
    std::string type;
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("image", po::value<std::string>(), "BIN image file")
        ("vid", po::value<std::string>(), "VID of device to program")
        ("pid", po::value<std::string>(), "PID of device to program")
        ("type", po::value<std::string>(), "device type (usrp1 or b100)")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //print the help message
    if (vm.count("help")){
        std::cout << boost::format("USRP EEPROM initialization %s") % desc << std::endl;
        return EXIT_FAILURE;
    }

    //cant find a uninitialized usrp with this mystery module in the way...
    if (std::system("/sbin/rmmod usbtest") != 0){
        std::cerr << "Did not rmmod usbtest, this may be ok..." << std::endl;
    }

    //load the options into the address
    uhd::device_addr_t device_addr;
    device_addr["type"] = type;
    if(vm.count("vid") or vm.count("pid") or vm.count("type")) {
        if(not (vm.count("vid") and vm.count("pid") and vm.count("type"))) {
            std::cerr << "ERROR: Must specify vid, pid, and type if specifying any of the three args" << std::endl;
        } else {
            device_addr["vid"] = vm["vid"].as<std::string>();
            device_addr["pid"] = vm["pid"].as<std::string>();
            device_addr["type"] = vm["type"].as<std::string>();
        }
    } else {
        device_addr["vid"] = FX2_VENDOR_ID;
        device_addr["pid"] = FX2_PRODUCT_ID;
    }

    //find and create a control transport to do the writing.

    uhd::device_addrs_t found_addrs = uhd::device::find(device_addr, uhd::device::USRP);

    if (found_addrs.size() == 0){
        std::cerr << "No USRP devices found" << std::endl;
        return EXIT_FAILURE;
    }

    for (size_t i = 0; i < found_addrs.size(); i++){
        std::cout << "Writing EEPROM data..." << std::endl;
        //uhd::device_addrs_t devs = uhd::device::find(found_addrs[i]);
        uhd::device::sptr dev = uhd::device::make(found_addrs[i], uhd::device::USRP);
        uhd::property_tree::sptr tree = dev->get_tree();
        tree->access<std::string>("/mboards/0/load_eeprom").set(vm["image"].as<std::string>());
    }


    std::cout << "Power-cycle the usrp for the changes to take effect." << std::endl;
    return EXIT_SUCCESS;
}
