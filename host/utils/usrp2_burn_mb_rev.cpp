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
#include <uhd/utils/assert.hpp>
#include <uhd/usrp/dboard_id.hpp>
#include <uhd/usrp/device_props.hpp>
#include <uhd/usrp/mboard_props.hpp>
#include <uhd/usrp/mboard_rev.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/assign.hpp>
#include <iostream>

using namespace uhd;
using namespace uhd::usrp;
namespace po = boost::program_options;

int UHD_SAFE_MAIN(int argc, char *argv[]){
    //command line variables
    std::string args;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""),    "device address args [default = \"\"]")
        ("rev",   po::value<std::string>(),                           "mboard rev to burn, omit for readback")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //print the help message
    if (vm.count("help")){
        std::cout << boost::format("USRP Burn MB HW revision %s") % desc << std::endl;
        std::cout << boost::format(
            "Omit the rev argument to perform readback,\n"
            "Or specify a new rev to burn into the eeprom.\n"
        ) << std::endl;
        return ~0;
    }

    //make the device and extract the mboard
    device::sptr dev = device::make(args);
    wax::obj u2_mb = (*dev)[DEVICE_PROP_MBOARD];

    //read the current mboard rev from eeprom
    if (vm.count("rev") == 0){
        std::cout << "Getting rev..." << std::endl;
        uhd::usrp::mboard_rev_t rev = mboard_rev_t::from_string(u2_mb[std::string("hw-rev")].as<std::string>());
        std::cout << boost::format("  Current rev: %s") % rev.to_pp_string() << std::endl;
    }

    //write a new mboard rev to eeprom
    else{
        mboard_rev_t rev = mboard_rev_t::from_string(vm["rev"].as<std::string>());
        std::cout << "Setting mboard rev..." << std::endl;
        std::cout << boost::format("  New rev: %s") % rev.to_pp_string() << std::endl;
        u2_mb[std::string("hw-rev")] = rev.to_string();
    }

    std::cout << "  Done" << std::endl << std::endl;
    return 0;
}
