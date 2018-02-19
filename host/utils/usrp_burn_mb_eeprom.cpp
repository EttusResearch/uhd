//
// Copyright 2010,2013-2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/utils/safe_main.hpp>
#include <uhd/device.hpp>
#include <uhd/property_tree.hpp>
#include <uhd/usrp/mboard_eeprom.hpp>
#include <uhd/types/device_addr.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <vector>

namespace po = boost::program_options;

int UHD_SAFE_MAIN(int argc, char *argv[]){
    std::string args, input_str;

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
        std::cout << boost::format("USRP Burn Motherboard EEPROM %s") % desc << std::endl;
        std::cout << boost::format(
            "Omit the value argument to perform a readback,\n"
            "Or specify a new value to burn into the EEPROM.\n"
            "Example (write to ip-addr0 and read out ip-addr1):\n"
            "    usrp_burn_mb_eeprom --args=<device args> --values\"ip-addr0=192.168.10.3,ip-addr1\""
        ) << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Creating USRP device from address: " + args << std::endl;
    uhd::device::sptr dev = uhd::device::make(args, uhd::device::USRP);
    uhd::property_tree::sptr tree = dev->get_tree();
    uhd::usrp::mboard_eeprom_t mb_eeprom = tree->access<uhd::usrp::mboard_eeprom_t>("/mboards/0/eeprom").get();
    std::cout << std::endl;

    std::vector<std::string> keys_vec, vals_vec;
    if(vm.count("read-all")) keys_vec = mb_eeprom.keys(); //Leaving vals_vec empty will force utility to only read
    else if(vm.count("values")){
        //uhd::device_addr_t properly parses input values
        uhd::device_addr_t vals(input_str);
        keys_vec = vals.keys();
        vals_vec = vals.vals();
    }

    // 1. Read out values
    std::cout << "Fetching current settings from EEPROM..." << std::endl;
    for(size_t i = 0; i < keys_vec.size(); i++){
        if (not mb_eeprom.has_key(keys_vec[i])){
            std::cerr << boost::format("Cannot find value for EEPROM[%s]") % keys_vec[i] << std::endl;
            return EXIT_FAILURE;
        }
        std::cout << boost::format("    EEPROM [\"%s\"] is \"%s\"") % keys_vec[i] % mb_eeprom[keys_vec[i]] << std::endl;
    }
    std::cout << std::endl;

    // 2. Write new values if given
    mb_eeprom = uhd::usrp::mboard_eeprom_t();
    for(size_t i = 0; i < vals_vec.size(); i++){
        if(vals_vec[i] != ""){
            mb_eeprom[keys_vec[i]] = vals_vec[i];
            std::cout << boost::format("Setting EEPROM [\"%s\"] to \"%s\"...") % keys_vec[i] % vals_vec[i] << std::endl;
        }
    }
    tree->access<uhd::usrp::mboard_eeprom_t>("/mboards/0/eeprom").set(mb_eeprom);
    std::cout << "Power-cycle the USRP device for the changes to take effect." << std::endl;
    std::cout << std::endl;

    std::cout << "Done" << std::endl;
    return EXIT_SUCCESS;
}
