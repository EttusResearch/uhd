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
#include <uhd/types/dict.hpp>
#include <uhd/usrp/dboard_id.hpp>
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

//used with lexical cast to parse a hex string
template <class T> struct to_hex{
    T value;
    operator T() const {return value;}
    friend std::istream& operator>>(std::istream& in, to_hex& out){
        in >> std::hex >> out.value;
        return in;
    }
};

int UHD_SAFE_MAIN(int argc, char *argv[]){
    //command line variables
    std::string args, db_name, unit;
    static const uhd::dict<std::string, mboard_prop_t> unit_to_db_prop = boost::assign::map_list_of
        ("RX", MBOARD_PROP_RX_DBOARD) ("TX", MBOARD_PROP_TX_DBOARD)
    ;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""),    "device address args [default = \"\"]")
        ("db",   po::value<std::string>(&db_name)->default_value(""), "dboard name [default = \"\"]")
        ("unit", po::value<std::string>(&unit)->default_value(""),    "which unit [RX or TX]")
        ("id",   po::value<std::string>(),                            "dboard id to burn (hex string), omit for readback")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //print the help message
    if (vm.count("help")){
        std::cout << boost::format("UHD Burn DB EEPROM %s") % desc << std::endl;
        std::cout << boost::format(
            "Omit the id argument to perform readback,\n"
            "Or specify a new id to burn into the eeprom.\n"
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
    wax::obj dboard = (*dev)[DEVICE_PROP_MBOARD][named_prop_t(unit_to_db_prop[unit], db_name)];
    std::string prefix = (db_name == "")? unit : (unit + ":" + db_name);

    //read the current dboard id from eeprom
    if (vm.count("id") == 0){
        std::cout << boost::format("Getting dbid on %s dboard...") % prefix << std::endl;
        dboard_id_t id = dboard[DBOARD_PROP_DBOARD_ID].as<dboard_id_t>();
        std::cout << boost::format("  Current dbid: %s") % id.to_pp_string() << std::endl;
    }

    //write a new dboard id to eeprom
    else{
        dboard_id_t id = dboard_id_t::from_string(vm["id"].as<std::string>());
        std::cout << boost::format("Setting dbid on %s dboard...") % prefix << std::endl;
        std::cout << boost::format("  New dbid: %s") % id.to_pp_string() << std::endl;
        dboard[DBOARD_PROP_DBOARD_ID] = id;
    }

    std::cout << "  Done" << std::endl << std::endl;
    return 0;
}
