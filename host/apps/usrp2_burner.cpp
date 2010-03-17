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
        ("new-ip", po::value<std::string>(), "new ip address (optional)")
        ("new-mac", po::value<std::string>(), "new mac address (optional)")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm); 

    //print the help message
    if (vm.count("help")){
        std::cout << boost::format("USRP2 Burner %s") % desc << std::endl;
        return ~0;
    }

    //load the options into the address
    uhd::device_addr_t device_addr;
    if (vm.count("addr")){
        device_addr["addr"] = vm["addr"].as<std::string>();
    }
    else{
        std::cerr << "Error: missing addr option" << std::endl;
        return ~0;
    }

    //create a usrp2 device
    uhd::device::sptr u2_dev = uhd::usrp::usrp2::make(device_addr);
    //FIXME usees the default mboard for now (until the mimo link is supported)
    wax::obj u2_mb = (*u2_dev)[uhd::DEVICE_PROP_MBOARD];

    //try to set the new ip (if provided)
    if (vm.count("new-ip")){
        std::cout << "Burning a new ip address into the usrp2 eeprom:" << std::endl;
        std::string old_ip = u2_mb[std::string("ip-addr")].as<std::string>();
        std::cout << boost::format("  Old IP Address: %s") % old_ip << std::endl;
        std::string new_ip = vm["new-ip"].as<std::string>();
        std::cout << boost::format("  New IP Address: %s") % new_ip << std::endl;
        u2_mb[std::string("ip-addr")] = new_ip;
        std::cout << "  Done" << std::endl;
    }

    //try to set the new mac (if provided)
    if (vm.count("new-mac")){
        std::cout << "Burning a new mac address into the usrp2 eeprom:" << std::endl;
        std::string old_mac = u2_mb[std::string("mac-addr")].as<std::string>();
        std::cout << boost::format("  Old MAC Address: %s") % old_mac << std::endl;
        std::string new_mac = vm["new-mac"].as<std::string>();
        std::cout << boost::format("  New MAC Address: %s") % new_mac << std::endl;
        u2_mb[std::string("mac-addr")] = new_mac;
        std::cout << "  Done" << std::endl;
    }

    std::cout << "Power-cycle the usrp2 for the changes to take effect." << std::endl;
    return 0;
}
