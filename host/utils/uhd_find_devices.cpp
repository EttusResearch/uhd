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

#include <uhd/utils/safe_main.hpp>
#include <uhd/device.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <cstdlib>
namespace po = boost::program_options;

int UHD_SAFE_MAIN(int argc, char *argv[]){
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>()->default_value(""), "device address args")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //print the help message
    if (vm.count("help")){
        std::cout << boost::format("UHD Find Devices %s") % desc << std::endl;
        return EXIT_FAILURE;
    }

    //discover the usrps and print the results
    uhd::device_addrs_t device_addrs = uhd::device::find(vm["args"].as<std::string>());

    if (device_addrs.size() == 0){
        std::cerr << "No UHD Devices Found" << std::endl;
        return EXIT_FAILURE;
    }

    typedef std::map<std::string, std::set<std::string> > device_multi_addrs_t;
    typedef std::map<std::string, device_multi_addrs_t> device_addrs_filtered_t;
    device_addrs_filtered_t found_devices;
    for (auto it = device_addrs.begin(); it != device_addrs.end(); ++it) {
        std::string serial = (*it)["serial"];
        found_devices[serial] = device_multi_addrs_t();
        for(std::string key: it->keys()) {
            if (key != "serial") {
                found_devices[serial][key].insert(it->get(key));
            }
        }
        for (auto sit = it + 1; sit != device_addrs.end();) {
            if ((*sit)["serial"] == serial) {
              for(std::string key: sit->keys()) {
                    if (key != "serial") {
                        found_devices[serial][key].insert(sit->get(key));
                    }
                }
                sit = device_addrs.erase(sit);
            } else {
                sit++;
            }
        }
    }

    int i = 0;
    for (auto dit = found_devices.begin(); dit != found_devices.end(); ++dit) {
        std::cout << "--------------------------------------------------"
                  << std::endl;
        std::cout << "-- UHD Device " << i << std::endl;
        std::cout << "--------------------------------------------------"
                  << std::endl;
        std::stringstream ss;
        ss << "Device Address:" << std::endl;
        ss << boost::format("    serial: %s") % dit->first << std::endl;
        for (auto mit = dit->second.begin(); mit != dit->second.end(); ++mit) {
            for (auto vit = mit->second.begin(); vit != mit->second.end();
                 ++vit) {
                ss << boost::format("    %s: %s") % mit->first % *vit
                   << std::endl;
            }
        }
        std::cout << ss.str() << std::endl << std::endl;
        i++;
    }

    return EXIT_SUCCESS;
}
