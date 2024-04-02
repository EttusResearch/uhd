//
// Copyright 2012,2014-2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/usrp_clock/multi_usrp_clock.hpp>
#include <uhd/utils/algorithm.hpp>
#include <uhd/utils/paths.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/utils/thread.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <chrono>
#include <cmath>
#include <complex>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <thread>

namespace po = boost::program_options;
namespace fs = boost::filesystem;

void print_notes(void)
{
    // Helpful notes
    std::cout << "**************************************Helpful Notes on Clock/PPS "
                 "Selection**************************************\n";
    std::cout << "As you can see, the default 10 MHz Reference and 1 PPS "
                 "signals are now from the GPSDO.\n";
    std::cout
        << "If you would like to use the internal reference(TCXO) in other applications, "
           "you must configure that explicitly.\n";
    std::cout << "***********************************************************************"
                 "*********"
                 "********************************\n";
}

int query_clock_sensors(const std::string& args)
{
    std::cout << boost::format("\nCreating the clock device with: %s...\n") % args;
    uhd::usrp_clock::multi_usrp_clock::sptr clock =
        uhd::usrp_clock::multi_usrp_clock::make(args);

    // Verify GPS sensors are present
    std::vector<std::string> sensor_names = clock->get_sensor_names(0);
    if (std::find(sensor_names.begin(), sensor_names.end(), "gps_locked")
        == sensor_names.end()) {
        std::cout << "\ngps_locked sensor not found.  This could mean that "
                     "this unit does not have a GPSDO.\n\n";
        return EXIT_FAILURE;
    }

    // Print NMEA strings
    try {
        uhd::sensor_value_t gga_string   = clock->get_sensor("gps_gpgga");
        uhd::sensor_value_t rmc_string   = clock->get_sensor("gps_gprmc");
        uhd::sensor_value_t servo_string = clock->get_sensor("gps_servo");
        std::cout << "\nPrinting available NMEA strings:\n";
        std::cout << boost::format("%s\n%s\n") % gga_string.to_pp_string()
                         % rmc_string.to_pp_string();
        std::cout << "\nPrinting GPS servo status:\n";
        std::cout << boost::format("%s\n\n") % servo_string.to_pp_string();
    } catch (const uhd::lookup_error&) {
        std::cout << "NMEA strings not implemented for this device." << std::endl;
    }
    std::cout << boost::format("GPS Epoch time: %.5f seconds\n")
                     % clock->get_sensor("gps_time").to_real();
    std::cout << boost::format("PC Clock time:  %.5f seconds\n") % time(NULL);

    // finished
    std::cout << "\nDone!\n\n";

    return EXIT_SUCCESS;
}

int UHD_SAFE_MAIN(int argc, char* argv[])
{
    std::string args;

    // Set up program options
    po::options_description desc("Allowed options");
    // clang-format off
  desc.add_options()
    ("help", "help message")
    ("args", po::value<std::string>(&args)->default_value(""), "Device address arguments specifying a single USRP")
    ("clock", "query a clock device's sensors")
    ;
    // clang-format on
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    // Print the help message
    if (vm.count("help")) {
        std::cout << "Query GPSDO Sensors, try to lock the reference oscillator to "
                     "the GPS disciplined clock, and set the device time to GPS time"
                  << std::endl
                  << std::endl
                  << desc;
        return EXIT_FAILURE;
    }

    // If specified, query a clock device instead
    if (vm.count("clock")) {
        return query_clock_sensors(args);
    }

    uhd::device_addr_t new_args(vm["args"].as<std::string>());

    new_args["clock_source"] = "gpsdo";
    new_args["time_source"]  = "gpsdo";

    // Create a USRP device
    std::cout << "Creating the USRP device with: " << new_args.to_string() << "..."
              << std::endl;
    uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(new_args);
    std::cout << "Using Device: " << usrp->get_pp_string() << std::endl;

    // Verify GPS sensors are present (i.e. EEPROM has been burnt)
    std::vector<std::string> sensor_names = usrp->get_mboard_sensor_names(0);

    if (std::find(sensor_names.begin(), sensor_names.end(), "gps_locked")
        == sensor_names.end()) {
        std::cout << "\ngps_locked sensor not found.  This could mean that "
                     "you have not installed the GPSDO correctly.\n\n";
        std::cout << "Visit one of these pages if the problem persists:\n";
        std::cout << " * N2X0/E1X0: http://files.ettus.com/manual/page_gpsdo.html\n";
        std::cout << " * X3X0: http://files.ettus.com/manual/page_gpsdo_x3x0.html\n\n";
        return EXIT_FAILURE;
    }

    bool ref_set_to_gpsdo = false;

    // Set clock source to gpsdo if supported
    if (uhd::has(usrp->get_clock_sources(0), "gpsdo")) {
        std::cout << "Setting the reference clock source to \"gpsdo\"..." << std::endl;
        usrp->set_clock_source("gpsdo");
        ref_set_to_gpsdo = true;
    }
    std::cout << "Clock source is now " << usrp->get_clock_source(0) << std::endl;

    // Set time source to gpsdo if supported
    if (uhd::has(usrp->get_time_sources(0), "gpsdo")) {
        std::cout << "Setting the reference clock source to \"gpsdo\"..." << std::endl;
        usrp->set_time_source("gpsdo");
        ref_set_to_gpsdo = true;
    }
    std::cout << "Time source is now " << usrp->get_time_source(0) << std::endl;

    if (not ref_set_to_gpsdo) {
        std::cerr << "ERROR: Unable to set clock or time reference to \"gpsdo\""
                  << std::endl;
        return EXIT_FAILURE;
    }

    // Check for ref lock
    if (std::find(sensor_names.begin(), sensor_names.end(), "ref_locked")
        != sensor_names.end()) {
        std::cout << "Waiting for ref_locked..." << std::flush;
        uhd::sensor_value_t ref_locked = usrp->get_mboard_sensor("ref_locked", 0);
        auto end = std::chrono::steady_clock::now() + std::chrono::seconds(30);
        while (!ref_locked.to_bool() && std::chrono::steady_clock::now() < end) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            ref_locked = usrp->get_mboard_sensor("ref_locked", 0);
            std::cout << "." << std::flush;
        }
        if (not ref_locked.to_bool()) {
            std::cout << "USRP NOT Locked to Reference.\n";
            std::cout << "Double check installation instructions (N2X0/E1X0 only): "
                         "https://www.ettus.com/content/files/gpsdo-kit_4.pdf\n\n";
            return EXIT_FAILURE;
        } else {
            std::cout << "USRP Locked to Reference.\n";
        }
    } else {
        std::cout << "ref_locked sensor not present on this board.\n";
    }

    print_notes();

    // The TCXO has a long warm up time, so wait up to 30 seconds for sensor data
    // to show up
    std::cout << "Waiting for the GPSDO to warm up..." << std::flush;
    auto end = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    while (std::chrono::steady_clock::now() < end) {
        try {
            usrp->get_mboard_sensor("gps_locked", 0);
            break;
        } catch (std::exception&) {
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        std::cout << "." << std::flush;
    }
    std::cout << std::endl;
    try {
        usrp->get_mboard_sensor("gps_locked", 0);
    } catch (std::exception&) {
        std::cout << "No response from GPSDO in 30 seconds" << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "The GPSDO is warmed up and talking." << std::endl;

    // Check for GPS lock
    uhd::sensor_value_t gps_locked = usrp->get_mboard_sensor("gps_locked", 0);
    ;
    if (not gps_locked.to_bool()) {
        std::cout << "\nGPS does not have lock. Wait a few minutes and try again.\n";
        std::cout << "NMEA strings and device time may not be accurate "
                     "until lock is achieved.\n\n";
    } else {
        std::cout << "GPS Locked";
    }

    // Check PPS and compare UHD device time to GPS time
    uhd::sensor_value_t gps_time   = usrp->get_mboard_sensor("gps_time");
    uhd::time_spec_t last_pps_time = usrp->get_time_last_pps();

    // we only care about the full seconds
    signed gps_seconds    = gps_time.to_int();
    long long pps_seconds = last_pps_time.to_ticks(1.0);

    if (pps_seconds != gps_seconds) {
        std::cout << "\nTrying to align the device time to GPS time..." << std::endl;

        gps_time = usrp->get_mboard_sensor("gps_time");

        // set the device time to the GPS time
        // getting the GPS time returns just after the PPS edge, so just add a
        // second and set the device time at the next PPS edge
        usrp->set_time_next_pps(uhd::time_spec_t(gps_time.to_int() + 1.0));
        // allow some time to make sure the PPS has come…
        std::this_thread::sleep_for(std::chrono::milliseconds(1100));
        //…then ask
        gps_seconds = usrp->get_mboard_sensor("gps_time").to_int();
        pps_seconds = usrp->get_time_last_pps().to_ticks(1.0);
    }

    if (pps_seconds == gps_seconds) {
        std::cout << "GPS and UHD Device time are aligned.\n";
    } else {
        std::cout << "Could not align UHD Device time to GPS time. Giving up.\n";
    }
    std::cout << boost::format("last_pps: %ld vs gps: %ld.") % pps_seconds % gps_seconds
              << std::endl;

    // print NMEA strings
    std::cout << "Printing available NMEA strings:\n";
    try {
        std::cout << usrp->get_mboard_sensor("gps_gpgga").to_pp_string() << std::endl;
    } catch (uhd::lookup_error&) {
        std::cout << "GPGGA string not available for this device." << std::endl;
    }
    try {
        std::cout << usrp->get_mboard_sensor("gps_gprmc").to_pp_string() << std::endl;
    } catch (uhd::lookup_error&) {
        std::cout << "GPRMC string not available for this device." << std::endl;
    }
    std::cout << boost::format("GPS Epoch time at last PPS: %.5f seconds\n")
                     % usrp->get_mboard_sensor("gps_time").to_real();
    std::cout << boost::format("UHD Device time last PPS:   %.5f seconds\n")
                     % (usrp->get_time_last_pps().get_real_secs());
    std::cout << boost::format("UHD Device time right now:  %.5f seconds\n")
                     % (usrp->get_time_now().get_real_secs());
    std::cout << boost::format("PC Clock time:              %.5f seconds\n") % time(NULL);

    // finished
    std::cout << "\nDone!\n\n";

    return EXIT_SUCCESS;
}
