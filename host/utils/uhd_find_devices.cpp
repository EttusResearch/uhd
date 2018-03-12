//
// Copyright 2010 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/utils/safe_main.hpp>
#include <uhd/device.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <cstdlib>

namespace {
    //! Conditionally append find_all=1 if the key isn't there yet
    uhd::device_addr_t append_findall(const uhd::device_addr_t& device_args)
    {
        uhd::device_addr_t new_device_args(device_args);
        if (!new_device_args.has_key("find_all")) {
            new_device_args["find_all"] = "1";
        }

        return new_device_args;
    }
}

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
        return EXIT_SUCCESS;
    }

    //discover the usrps and print the results
    const uhd::device_addr_t args(vm["args"].as<std::string>());
    uhd::device_addrs_t device_addrs =
        uhd::device::find(append_findall(args));
    if (device_addrs.empty()) {
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
