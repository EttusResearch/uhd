//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/device.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/usrp_clock/octoclock_eeprom.hpp>
#include <uhd/property_tree.hpp>
#include <uhd/types/device_addr.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <vector>

namespace po = boost::program_options;

using namespace uhd;
using namespace uhd::usrp_clock;

int UHD_SAFE_MAIN(int argc, char *argv[]){
    std::string args, input_str, key, val;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""), "device address args [default = \"\"]")
        ("values", po::value<std::string>(&input_str), "keys+values to read/write, separate multiple by \",\"")
        ("read-all", "Read all motherboard EEPROM values without writing")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //print the help message
    if (vm.count("help") or (not vm.count("values") and not vm.count("read-all"))){
        std::cout << boost::format("OctoClock Burn EEPROM %s") % desc << std::endl;
        std::cout << boost::format(
            "Omit the value argument to perform a readback,\n"
            "Or specify a new value to burn into the EEPROM.\n"
        ) << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Creating OctoClock device from args: " + args << std::endl;
    device::sptr oc = device::make(args, device::CLOCK);
    property_tree::sptr tree = oc->get_tree();
    octoclock_eeprom_t oc_eeprom = tree->access<octoclock_eeprom_t>("/mboards/0/eeprom").get();
    std::cout << std::endl;

    std::vector<std::string> keys_vec, vals_vec;
    if(vm.count("read-all")) keys_vec = oc_eeprom.keys(); //Leaving vals_vec empty will force utility to only read
    else if(vm.count("values")){
        //uhd::device_addr_t properly parses input values
        device_addr_t vals(input_str);
        keys_vec = vals.keys();
        vals_vec = vals.vals();
    }
    else throw std::runtime_error("Must specify --values or --read-all option!");

    std::cout << "Fetching current settings from EEPROM..." << std::endl;
    for(size_t i = 0; i < keys_vec.size(); i++){
        if (not oc_eeprom.has_key(keys_vec[i])){
            std::cerr << boost::format("Cannot find value for EEPROM[\"%s\"]") % keys_vec[i] << std::endl;
            return EXIT_FAILURE;
        }
        std::cout << boost::format("    EEPROM [\"%s\"] is \"%s\"") % keys_vec[i] % oc_eeprom[keys_vec[i]] << std::endl;
    }
    if(!vm.count("read-all")){
        std::cout << std::endl;
        for(size_t i = 0; i < vals_vec.size(); i++){
            if(vals_vec[i] != ""){
                oc_eeprom[keys_vec[i]] = vals_vec[i];
                std::cout << boost::format("Setting EEPROM [\"%s\"] to \"%s\"...") % keys_vec[i] % vals_vec[i] << std::endl;
            }
        }
        tree->access<octoclock_eeprom_t>("/mboards/0/eeprom").set(oc_eeprom);
    }
    std::cout << std::endl << "Power-cycle your device to allow any changes to take effect." << std::endl;
    return EXIT_SUCCESS;
}
