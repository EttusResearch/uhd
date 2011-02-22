//
// Copyright 2010-2011 Ettus Research LLC
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
#include <uhd/types/dict.hpp>
#include <uhd/utils/assert.hpp>
#include <uhd/usrp/dboard_eeprom.hpp>
#include <uhd/usrp/device_props.hpp>
#include <uhd/usrp/mboard_props.hpp>
#include <uhd/usrp/dboard_props.hpp>
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
    static const uhd::dict<std::string, mboard_prop_t> unit_to_db_prop = boost::assign::map_list_of
        ("RX", MBOARD_PROP_RX_DBOARD) ("TX", MBOARD_PROP_TX_DBOARD)
    ;
    static const uhd::dict<std::string, mboard_prop_t> unit_to_db_names_prop = boost::assign::map_list_of
        ("RX", MBOARD_PROP_RX_DBOARD_NAMES) ("TX", MBOARD_PROP_TX_DBOARD_NAMES)
    ;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""),    "device address args [default = \"\"]")
        ("slot", po::value<std::string>(&slot)->default_value(""),    "dboard slot name [default is blank for automatic]")
        ("unit", po::value<std::string>(&unit)->default_value(""),    "which unit [RX or TX]")
        ("id",   po::value<std::string>(),                            "dboard id to burn, omit for readback")
        ("ser",  po::value<std::string>(),                            "serial to burn, omit for readback")
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
        return ~0;
    }

    //check inputs
    if (not unit_to_db_prop.has_key(unit)){
        std::cout << "Error: specify RX or TX for unit" << std::endl;
        return ~0;
    }

    //make the device and extract the dboard w/ property
    device::sptr dev = device::make(args);
    uhd::prop_names_t dboard_names = (*dev)[DEVICE_PROP_MBOARD][unit_to_db_names_prop[unit]].as<uhd::prop_names_t>();
    if (dboard_names.size() == 1 and slot.empty()) slot = dboard_names.front();
    uhd::assert_has(dboard_names, slot, "dboard slot name");
    wax::obj dboard = (*dev)[DEVICE_PROP_MBOARD][named_prop_t(unit_to_db_prop[unit], slot)];
    std::string prefix = unit + ":" + slot;

    std::cout << boost::format("Reading EEPROM on %s dboard...") % prefix << std::endl;
    dboard_eeprom_t db_eeprom = dboard[DBOARD_PROP_DBOARD_EEPROM].as<dboard_eeprom_t>();

    //------------- handle the dboard ID -----------------------------//
    if (vm.count("id")){
        db_eeprom.id = dboard_id_t::from_string(vm["id"].as<std::string>());
        dboard[DBOARD_PROP_DBOARD_EEPROM] = db_eeprom;
    }
    std::cout << boost::format("  Current ID: %s") % db_eeprom.id.to_pp_string() << std::endl;

    //------------- handle the dboard serial--------------------------//
    if (vm.count("ser")){
        db_eeprom.serial = vm["ser"].as<std::string>();
        dboard[DBOARD_PROP_DBOARD_EEPROM] = db_eeprom;
    }
    std::cout << boost::format("  Current serial: \"%s\"") % db_eeprom.serial << std::endl;

    std::cout << "  Done" << std::endl << std::endl;
    return 0;
}
