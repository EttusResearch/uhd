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

#include <uhd/device.hpp>
#include <uhd/props.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <iostream>

namespace po = boost::program_options;

int main(int argc, char *argv[]){
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("addr", po::value<std::string>(), "resolvable network address")
        ("node", po::value<std::string>(), "path to linux device node")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm); 

    //print the help message
    if (vm.count("help")){
        std::cout << boost::format("Discover USRPs %s") % desc << std::endl;
        return ~0;
    }

    //load the options into the address
    uhd::device_addr_t device_addr;
    if (vm.count("addr")){
        device_addr["addr"] = vm["addr"].as<std::string>();
    }
    if (vm.count("node")){
        device_addr["node"] = vm["node"].as<std::string>();
    }

    //discover the usrps and print the results
    uhd::device_addrs_t device_addrs = uhd::device::discover(device_addr);

    if (device_addrs.size() == 0){
        std::cerr << "No USRP Devices Found" << std::endl;
        return ~0;
    }

    for (size_t i = 0; i < device_addrs.size(); i++){
        std::cout << "--------------------------------------------------" << std::endl;
        std::cout << "-- USRP Device " << i << std::endl;
        std::cout << "--------------------------------------------------" << std::endl;
        std::cout << device_addrs[i] << std::endl << std::endl;
        //make each device just to test (TODO: remove this)
        uhd::device::sptr dev = uhd::device::make(device_addrs[i]);
        std::cout << wax::cast<std::string>((*dev)[uhd::DEVICE_PROP_MBOARD][uhd::MBOARD_PROP_NAME]) << std::endl;
    }

    return 0;
}
