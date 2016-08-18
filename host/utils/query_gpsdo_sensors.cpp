//
// Copyright 2012,2014-2016 Ettus Research LLC
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

#include <uhd/utils/paths.hpp>
#include <uhd/utils/thread_priority.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/usrp_clock/multi_usrp_clock.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <complex>
#include <boost/thread.hpp>
#include <string>
#include <cmath>
#include <ctime>
#include <cstdlib>

namespace po = boost::program_options;
namespace fs = boost::filesystem;

void print_notes(void) {
  // Helpful notes
  std::cout << boost::format("**************************************Helpful Notes on Clock/PPS Selection**************************************\n");
  std::cout << boost::format("As you can see, the default 10 MHz Reference and 1 PPS signals are now from the GPSDO.\n");
  std::cout << boost::format("If you would like to use the internal reference(TCXO) in other applications, you must configure that explicitly.\n");
  std::cout << boost::format("****************************************************************************************************************\n");
}

int query_clock_sensors(const std::string &args) {
  std::cout << boost::format("\nCreating the clock device with: %s...\n") % args;
  uhd::usrp_clock::multi_usrp_clock::sptr clock = uhd::usrp_clock::multi_usrp_clock::make(args);

  //Verify GPS sensors are present
  std::vector<std::string> sensor_names = clock->get_sensor_names(0);
  if(std::find(sensor_names.begin(), sensor_names.end(), "gps_locked") == sensor_names.end()) {
    std::cout << boost::format("\ngps_locked sensor not found.  This could mean that this unit does not have a GPSDO.\n\n");
    return EXIT_FAILURE;
  }

  // Print NMEA strings
  try {
      uhd::sensor_value_t gga_string = clock->get_sensor("gps_gpgga");
      uhd::sensor_value_t rmc_string = clock->get_sensor("gps_gprmc");
      uhd::sensor_value_t servo_string = clock->get_sensor("gps_servo");
      std::cout << boost::format("\nPrinting available NMEA strings:\n");
      std::cout << boost::format("%s\n%s\n") % gga_string.to_pp_string() % rmc_string.to_pp_string();
      std::cout << boost::format("\nPrinting GPS servo status:\n");
      std::cout << boost::format("%s\n\n") % servo_string.to_pp_string();
  } catch (uhd::lookup_error &e) {
      std::cout << "NMEA strings not implemented for this device." << std::endl;
  }
  std::cout << boost::format("GPS Epoch time: %.5f seconds\n") % clock->get_sensor("gps_time").to_real();
  std::cout << boost::format("PC Clock time:  %.5f seconds\n") % time(NULL);

  //finished
  std::cout << boost::format("\nDone!\n\n");

  return EXIT_SUCCESS;
}

int UHD_SAFE_MAIN(int argc, char *argv[]){
  uhd::set_thread_priority_safe();

  std::string args;

  //Set up program options
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help", "help message")
    ("args", po::value<std::string>(&args)->default_value(""), "Device address arguments specifying a single USRP")
    ("clock", "query a clock device's sensors")
    ;
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  //Print the help message
  if (vm.count("help")) {
      std::cout << boost::format("Query GPSDO Sensors, try to lock the reference oscillator to the GPS disciplined clock, and set the device time to GPS time")
          << std::endl
          << std::endl
          << desc;
      return EXIT_FAILURE;
  }

  //If specified, query a clock device instead
  if(vm.count("clock")) {
      return query_clock_sensors(args);
  }

  //Create a USRP device
  std::cout << boost::format("\nCreating the USRP device with: %s...\n") % args;
  uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);
  std::cout << boost::format("Using Device: %s\n") % usrp->get_pp_string();

  //Verify GPS sensors are present (i.e. EEPROM has been burnt)
  std::vector<std::string> sensor_names = usrp->get_mboard_sensor_names(0);

  if (std::find(sensor_names.begin(), sensor_names.end(), "gps_locked") == sensor_names.end()) {
      std::cout << boost::format("\ngps_locked sensor not found.  This could mean that you have not installed the GPSDO correctly.\n\n");
      std::cout << boost::format("Visit one of these pages if the problem persists:\n");
      std::cout << boost::format(" * N2X0/E1X0: http://files.ettus.com/manual/page_gpsdo.html\n");
      std::cout << boost::format(" * X3X0: http://files.ettus.com/manual/page_gpsdo_x3x0.html\n\n");
      return EXIT_FAILURE;
  }

  // Explicitly set time source to gpsdo
  boost::this_thread::sleep(boost::posix_time::seconds(1));
  try {
      usrp->set_time_source("gpsdo");
  } catch (uhd::value_error &e) {
      std::cout << "could not set the time source to \"gpsdo\"; error was:" <<std::endl;
      std::cout << e.what() << std::endl;
      std::cout << "trying \"external\"..." <<std::endl;
      try {
          usrp->set_time_source("external");
      } catch (uhd::value_error&) {
          std::cout << "\"external\" failed, too." << std::endl;
      }
  }
  std::cout<< std::endl << "Time source is now " << usrp->get_time_source(0) << std::endl;

  //Check for GPS lock
  uhd::sensor_value_t gps_locked = usrp->get_mboard_sensor("gps_locked",0);
  if(not gps_locked.to_bool()) {
      std::cout << boost::format("\nGPS does not have lock. Wait a few minutes and try again.\n");
      std::cout << boost::format("NMEA strings and device time may not be accurate until lock is achieved.\n\n");
  } else {
      std::cout << boost::format("GPS Locked");
  }

  std::cout << "\nSetting the reference clock source to \"gpsdo\"...\n";
  try {
      usrp->set_clock_source("gpsdo");
  } catch (uhd::value_error &e) {
      std::cout << "could not set the clock source to \"gpsdo\"; error was:" <<std::endl;
      std::cout << e.what() << std::endl;
      std::cout << "trying \"external\"..." <<std::endl;
      try{
          usrp->set_clock_source("external");
      } catch (uhd::value_error&) {
          std::cout << "\"external\" failed, too." << std::endl;
      }
  }
  std::cout<< std::endl << "Clock source is now " << usrp->get_clock_source(0) << std::endl;

  print_notes();


  //Check for 10 MHz lock
  if(std::find(sensor_names.begin(), sensor_names.end(), "ref_locked") != sensor_names.end()) {
      uhd::sensor_value_t gps_locked = usrp->get_mboard_sensor("ref_locked",0);
      if(not gps_locked.to_bool()) {
          std::cout << boost::format("USRP NOT Locked to GPSDO 10 MHz Reference.\n");
          std::cout << boost::format("Double check installation instructions (N2X0/E1X0 only): https://www.ettus.com/content/files/gpsdo-kit_4.pdf\n\n");
          std::cout << boost::format("Locking the internal reference to the GPSDO might take a second to reach stability. Retrying in 10 s...\n\n");
          boost::this_thread::sleep(boost::posix_time::seconds(10));
      }
      if(usrp->get_mboard_sensor("ref_locked",0).to_bool()) {
          std::cout << boost::format("USRP Locked to GPSDO 10 MHz Reference.\n");
      }
  } else {
      std::cout << boost::format("ref_locked sensor not present on this board.\n");
  }

  //Check PPS and compare UHD device time to GPS time
  boost::this_thread::sleep(boost::posix_time::seconds(1));
  uhd::sensor_value_t gps_time = usrp->get_mboard_sensor("gps_time");
  uhd::time_spec_t last_pps_time = usrp->get_time_last_pps();

  //we only care about the full seconds
  signed gps_seconds = gps_time.to_int();
  long long pps_seconds = last_pps_time.to_ticks(1.0);

  if(pps_seconds != gps_seconds) {
      std::cout << boost::format("\nGPS and UHD Device time are NOT aligned;\nlast_pps: %ld vs gps: %ld. Trying to set the device time to GPS time...")
                % pps_seconds % gps_seconds
                << std::endl;
      //full next after next second
      uhd::time_spec_t next_pps_time(gps_seconds + 2.0);
      //instruct the USRP to wait for the next PPS edge, then set the new time on the following PPS
      usrp->set_time_unknown_pps(next_pps_time);
      //allow some time to make sure the PPS has come…
      boost::this_thread::sleep(boost::posix_time::milliseconds(1100));
      //…then ask
      gps_seconds = usrp->get_mboard_sensor("gps_time").to_int();
      pps_seconds = usrp->get_time_last_pps().to_ticks(1.0);
  }

  std::cout << boost::format("last_pps: %ld vs gps: %ld.")
            % pps_seconds % gps_seconds
            << std::endl;
  if (pps_seconds == gps_seconds) {
      std::cout << boost::format("GPS and UHD Device time are aligned.\n");
  } else {
      std::cout << boost::format("Could not align UHD Device time to GPS time. Giving up.\n");
  }

  //print NMEA strings
  try {
      uhd::sensor_value_t gga_string = usrp->get_mboard_sensor("gps_gpgga");
      uhd::sensor_value_t rmc_string = usrp->get_mboard_sensor("gps_gprmc");
      std::cout << boost::format("Printing available NMEA strings:\n");
      std::cout << boost::format("%s\n%s\n") % gga_string.to_pp_string() % rmc_string.to_pp_string();
  } catch (uhd::lookup_error&) {
      std::cout << "NMEA strings not implemented for this device." << std::endl;
  }
  std::cout << boost::format("GPS Epoch time at last PPS: %.5f seconds\n") % usrp->get_mboard_sensor("gps_time").to_real();
  std::cout << boost::format("UHD Device time last PPS:   %.5f seconds\n") % (usrp->get_time_last_pps().get_real_secs());
  std::cout << boost::format("UHD Device time right now:  %.5f seconds\n") % (usrp->get_time_now().get_real_secs());
  std::cout << boost::format("PC Clock time:              %.5f seconds\n") % time(NULL);

  //finished
  std::cout << boost::format("\nDone!\n\n");

  return EXIT_SUCCESS;
}
