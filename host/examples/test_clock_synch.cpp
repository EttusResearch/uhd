//
// Copyright 2014 Ettus Research LLC
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

#include <iostream>

#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <boost/thread/thread.hpp>

#include <uhd/device.hpp>
#include <uhd/exception.hpp>
#include <uhd/usrp_clock/multi_usrp_clock.hpp>
#include <uhd/types/time_spec.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/utils/thread_priority.hpp>

namespace po = boost::program_options;

using namespace uhd::usrp_clock;
using namespace uhd::usrp;

void wait_for_pps(multi_usrp::sptr usrp, size_t chan, double timeout){
    boost::uint32_t last_pps_time = usrp->get_time_last_pps(chan).get_full_secs();
    boost::uint32_t system_time = uhd::time_spec_t::get_system_time().get_full_secs();
    boost::uint32_t exit_time = system_time + timeout;
    bool detected_pps = false;

    //Otherwise, this would hang if the USRP doesn't detect any PPS
    while(uhd::time_spec_t::get_system_time().get_full_secs() < exit_time){
        boost::uint32_t time_now = usrp->get_time_last_pps(chan).get_full_secs();
        if(last_pps_time < time_now){
            detected_pps = true;
            break;
        }
        else last_pps_time = time_now;
    }
    if(not detected_pps) throw uhd::runtime_error(str(boost::format("%s did not detect a PPS signal.")
                                                      % usrp->get_usrp_tx_info()["mboard_serial"]));

}

void get_usrp_time(multi_usrp::sptr usrp, size_t chan, std::vector<boost::uint32_t> *times){
    wait_for_pps(usrp, chan, 2);
    (*times)[chan] = usrp->get_time_now(chan).get_full_secs();
}

int UHD_SAFE_MAIN(int argc, char *argv[]){
    uhd::set_thread_priority_safe();

    //Variables to be set by command line options
    std::string clock_args, usrp_args;
    boost::uint32_t max_interval, num_tests;

    //Set up program options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "Display this help message")
        ("clock-args", po::value<std::string>(&clock_args), "Clock device arguments")
        ("usrp-args", po::value<std::string>(&usrp_args), "USRP device arguments")
        ("max-interval", po::value<boost::uint32_t>(&max_interval)->default_value(10000), "Maximum interval between comparisons (in ms)")
        ("num-tests", po::value<boost::uint32_t>(&num_tests)->default_value(10), "Number of times to compare device times")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //Print the help message
    if (vm.count("help")){
        std::cout << std::endl << "Test Clock Synchronization" << std::endl << std::endl;

        std::cout << "This example shows how to use a clock device to" << std::endl
                  << "synchronize the time on multiple USRP devices." << std::endl << std::endl;

        std::cout << desc << std::endl;
        return EXIT_SUCCESS;
    }

    //Create a Multi-USRP-Clock device (currently OctoClock only)
    std::cout << boost::format("\nCreating the Clock device with: %s") % clock_args << std::endl;
    multi_usrp_clock::sptr clock = multi_usrp_clock::make(clock_args);

    //Make sure Clock configuration is correct
    if(clock->get_sensor("gps_detected").value == "false"){
        throw uhd::runtime_error("No GPSDO detected on Clock.");
    }
    if(clock->get_sensor("using_ref").value != "internal"){
        throw uhd::runtime_error("Clock must be using an internal reference.");
    }

    //Create a Multi-USRP device
    std::cout << boost::format("\nCreating the USRP device with: %s") % usrp_args << std::endl;
    multi_usrp::sptr usrp = multi_usrp::make(usrp_args);

    //Store USRP device serials for useful output
    std::vector<std::string> serials;
    for(size_t ch = 0; ch < usrp->get_num_mboards(); ch++){
        serials.push_back(usrp->get_usrp_tx_info(ch)["mboard_serial"]);
    }

    std::cout << std::endl << "Checking USRP devices for lock." << std::endl;
    bool all_locked = true;
    for(size_t ch = 0; ch < usrp->get_num_mboards(); ch++){
        std::string ref_locked = usrp->get_mboard_sensor("ref_locked",ch).value;
        std::cout << boost::format(" * %s: %s") % serials[ch] % ref_locked << std::endl;

        if(ref_locked != "true") all_locked = false;
    }
    if(not all_locked) std::cout << std::endl << "WARNING: One or more devices not locked." << std::endl;

    //Get GPS time to initially set USRP devices
    std::cout << std::endl << "Querying Clock for time and setting USRP times..." << std::endl << std::endl;
    boost::uint32_t clock_time = clock->get_time();
    usrp->set_time_unknown_pps(uhd::time_spec_t(double(clock_time+2)));

    //Wait for next PPS to start polling
    wait_for_pps(usrp, 0, 2);

    srand(time(NULL));

    std::cout << boost::format("\nRunning %d comparisons at random intervals.") % num_tests << std::endl << std::endl;
    boost::uint32_t num_matches = 0;
    for(size_t i = 0; i < num_tests; i++){
        //Wait random time before querying
        boost::uint16_t wait_time = rand() % max_interval;
        boost::this_thread::sleep(boost::posix_time::milliseconds(wait_time));

        //Get all times before output
        std::vector<boost::uint32_t> usrp_times(usrp->get_num_mboards());
        boost::thread_group thread_group;
        clock_time = clock->get_time();
        for(size_t j = 0; j < usrp->get_num_mboards(); j++){
            thread_group.create_thread(boost::bind(&get_usrp_time, usrp, j, &usrp_times));
        }
        //Wait for threads to complete
        thread_group.join_all();

        std::cout << boost::format("Comparison #%d") % (i+1) << std::endl;
        bool all_match = true;
        std::cout << boost::format(" * Clock time: %d") % clock_time << std::endl;
        for(size_t j = 0; j < usrp->get_num_mboards(); j++){
            std::cout << boost::format(" * %s time: %d") % serials[j] % usrp_times[j] << std::endl;
            if(usrp_times[j] != clock_time) all_match = false;
        }
        if(all_match) num_matches++;
    }

    std::cout << std::endl << boost::format("Number of matches: %d/%d") % num_matches % num_tests << std::endl;

    return EXIT_SUCCESS;
}
