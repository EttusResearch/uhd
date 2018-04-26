//
// Copyright 2010-2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/utils/thread.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <complex>
#include <chrono>
#include <thread>

namespace po = boost::program_options;

int UHD_SAFE_MAIN(int argc, char *argv[]){
    uhd::set_thread_priority_safe();

    //variables to be set by po
    std::string args;
    std::string time_source;

    //setup the program options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""), "single uhd device address args")
        ("source", po::value<std::string>(&time_source)->default_value(""), "the time source (gpsdo, external) or blank for default")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //print the help message
    if (vm.count("help")){
        std::cout << boost::format("UHD Test PPS Input %s") % desc << std::endl;
        std::cout
                << std::endl
                << "Tests if the PPS input signal is working. Will throw an error if not."
                << std::endl
                << std::endl;
        return ~0;
    }

    //create a usrp device
    std::cout << std::endl;
    std::cout << boost::format("Creating the usrp device with: %s...") % args << std::endl;
    uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);
    std::cout << boost::format("Using Device: %s") % usrp->get_pp_string() << std::endl;

    //sleep off if gpsdo detected and time next pps already set
    std::this_thread::sleep_for(std::chrono::seconds(1));

    //set time source if specified
    if (not time_source.empty()) usrp->set_time_source(time_source);

    //set the time at an unknown pps (will throw if no pps)
    std::cout << std::endl << "Attempt to detect the PPS and set the time..." << std::endl << std::endl;
    usrp->set_time_unknown_pps(uhd::time_spec_t(0.0));
    std::cout << std::endl << "Success!" << std::endl << std::endl;
    return EXIT_SUCCESS;
}
