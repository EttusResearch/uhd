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

#include <uhd/utils/thread_priority.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/usrp/simple_usrp.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <iostream>
#include <complex>

namespace po = boost::program_options;

int UHD_SAFE_MAIN(int argc, char *argv[]){
    uhd::set_thread_priority_safe();

    //variables to be set by po
    std::string args;
    float seconds;

    //setup the program options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""), "simple uhd device address args")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //print the help message
    if (vm.count("help")){
        std::cout << boost::format("UHD Test PPS Input %s") % desc << std::endl;
        return ~0;
    }

    //create a usrp device
    std::cout << std::endl;
    std::cout << boost::format("Creating the usrp device with: %s...") % args << std::endl;
    uhd::usrp::simple_usrp::sptr sdev = uhd::usrp::simple_usrp::make(args);
    uhd::device::sptr dev = sdev->get_device();
    std::cout << boost::format("Using Device: %s") % sdev->get_pp_string() << std::endl;

    //set a known time value
    std::cout << "Set time to known value (100.0) without regard to pps:" << std::endl;
    sdev->set_time_now(uhd::time_spec_t(100.0));
    boost::this_thread::sleep(boost::posix_time::seconds(1));
    std::cout << boost::format("Reading time 1 second later: %f\n") % (sdev->get_time_now().get_full_secs()) << std::endl;

    //store the time to see if PPS resets it
    seconds = sdev->get_time_now().get_full_secs();

    //set a known time at next PPS, check that time increments
    uhd::time_spec_t time_spec = uhd::time_spec_t(0.0);
    std::cout << "Set time to known value (0.0) at next pps:" << std::endl;
    sdev->set_time_next_pps(time_spec);
    boost::this_thread::sleep(boost::posix_time::seconds(1));
    std::cout << boost::format("Reading time 1 second later: %f\n") % (sdev->get_time_now().get_full_secs()) << std::endl;

    //finished
    if (seconds > sdev->get_time_now().get_full_secs()){
        std::cout << std::endl << "Success!" << std::endl << std::endl;
        return 0;
    } else {
        std::cout << std::endl << "Failed!" << std::endl << std::endl 
            << "If you expected PPS to work:" << std::endl
            << "\tsee Device App Notes for PPS level information"
            << std::endl << std::endl;
        return -1;
    }
}
