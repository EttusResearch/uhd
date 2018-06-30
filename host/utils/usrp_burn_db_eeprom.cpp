//
// Copyright 2010-2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//


#include <uhd/utils/safe_main.hpp>
#include <uhd/device.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/utils/assert_has.hpp>
#include <uhd/property_tree.hpp>
#include <uhd/usrp/dboard_eeprom.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/assign.hpp>
#include <iostream>

using namespace uhd;
using namespace uhd::usrp;
namespace po = boost::program_options;

int UHD_SAFE_MAIN(int argc, char *argv[]){
    //command line variables
    std::string args, slot, unit;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""),    "device address args [default = \"\"]")
        ("slot", po::value<std::string>(&slot)->default_value(""),    "dboard slot name [default is blank for automatic]")
        ("unit", po::value<std::string>(&unit)->default_value(""),    "which unit [RX, TX, or GDB]")
        ("id",   po::value<std::string>(),                            "dboard id to burn, omit for readback")
        ("ser",  po::value<std::string>(),                            "serial to burn, omit for readback")
        ("rev",  po::value<std::string>(),                            "revision to burn, omit for readback")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //print the help message
    if (vm.count("help")){
        std::cout << boost::format("USRP Burn Daughterboard EEPROM %s") % desc << std::endl;
        std::cout << boost::format(
            "Omit the ID argument to perform readback,\n"
            "Or specify a new ID to burn into the EEPROM.\n"
        ) << std::endl;
        return EXIT_FAILURE;
    }

    //make the device and extract the dboard w/ property
    device::sptr dev = device::make(args, device::USRP);
    uhd::property_tree::sptr tree = dev->get_tree();
    const uhd::fs_path db_root = "/mboards/0/dboards";
    std::vector<std::string> dboard_names = tree->list(db_root);
    if (dboard_names.size() == 1 and slot.empty()) slot = dboard_names.front();
    uhd::assert_has(dboard_names, slot, "dboard slot name");

    std::cout << boost::format("Reading %s EEPROM on %s dboard...") % unit % slot << std::endl;
    boost::to_lower(unit);
    const uhd::fs_path db_path = db_root / slot / (unit + "_eeprom");
    dboard_eeprom_t db_eeprom = tree->access<dboard_eeprom_t>(db_path).get();

    //------------- handle the dboard ID -----------------------------//
    if (vm.count("id")){
        db_eeprom.id = dboard_id_t::from_string(vm["id"].as<std::string>());
        tree->access<dboard_eeprom_t>(db_path).set(db_eeprom);
    }
    std::cout << boost::format("  Current ID: %s") % db_eeprom.id.to_pp_string() << std::endl;

    //------------- handle the dboard serial--------------------------//
    if (vm.count("ser")){
        db_eeprom.serial = vm["ser"].as<std::string>();
        tree->access<dboard_eeprom_t>(db_path).set(db_eeprom);
    }
    std::cout << boost::format("  Current serial: \"%s\"") % db_eeprom.serial << std::endl;

    //------------- handle the dboard revision------------------------//
    if (vm.count("rev")){
        db_eeprom.revision = vm["rev"].as<std::string>();
        tree->access<dboard_eeprom_t>(db_path).set(db_eeprom);
    }
    std::cout << boost::format("  Current revision: \"%s\"") % db_eeprom.revision << std::endl;

    std::cout << "  Done" << std::endl << std::endl;
    return EXIT_SUCCESS;
}
