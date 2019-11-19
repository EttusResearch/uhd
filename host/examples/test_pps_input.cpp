//
// Copyright 2010-2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/safe_main.hpp>
#include <boost/program_options.hpp>
#include <chrono>
#include <iostream>
#include <thread>

namespace po = boost::program_options;

bool product_requires_reflock(const std::string& product)
{
    if (product.find("e31") == 0 || product.find("E31") == 0) {
        return true;
    }

    return false;
}

int UHD_SAFE_MAIN(int argc, char* argv[])
{
    // variables to be set by po
    std::string args;
    std::string time_source;

    // setup the program options
    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""), "single uhd device address args")
        ("source", po::value<std::string>(&time_source)->default_value(""), "the time source (gpsdo, external) or blank for default")
    ;
    // clang-format on
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    // print the help message
    if (vm.count("help")) {
        std::cout << "UHD Test PPS Input " << desc << std::endl;
        std::cout
            << std::endl
            << "Tests if the PPS input signal is working. Will throw an error if not."
            << std::endl
            << std::endl;
        return ~0;
    }

    // create a usrp device
    std::cout << std::endl;
    std::cout << "Creating the USRP device with: " << args << "..." << std::endl;
    auto usrp = uhd::usrp::multi_usrp::make(args);
    std::cout << "Using Device: " << usrp->get_pp_string() << std::endl;

    // sleep off if gpsdo detected and time next pps already set
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // set time source if specified
    if (not time_source.empty()) {
        usrp->set_time_source(time_source);
    }

    // set the time at an unknown pps (will throw if no pps)
    std::cout << std::endl
              << "Attempt to detect the PPS and set the time..." << std::endl
              << std::endl;
    usrp->set_time_unknown_pps(uhd::time_spec_t(0.0));
    std::cout << std::endl << "Success!" << std::endl << std::endl;

    if (product_requires_reflock(usrp->get_mboard_name())) {
        std::cout << "Product requires verification of ref_locked sensor!" << std::endl;
        std::cout << "Checking ref_locked sensor..." << std::flush;
        if (!usrp->get_mboard_sensor("ref_locked").to_bool()) {
            std::cout << "FAIL!" << std::endl;
            return EXIT_FAILURE;
        }
        std::cout << "PASS!" << std::endl;
    }

    return EXIT_SUCCESS;
}
