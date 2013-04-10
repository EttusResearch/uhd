//
// Copyright 2012 Ettus Research LLC
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
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <complex>
#include <boost/thread.hpp>
#include <string>
#include <cmath>
#include <cstdlib>

namespace po = boost::program_options;
namespace fs = boost::filesystem;

void print_notes(void) {
  // Helpful notes
  std::cout << boost::format("**************************************Helpful Notes on Clock/PPS Selection**************************************\n");
  std::cout << boost::format("As you can see, the default 10 MHz Reference and 1 PPS signals are now from the GPSDO.\n");
  std::cout << boost::format("If you would like to use the internal reference(TCXO) in other applications, you must configure that explicitly.\n");
  std::cout << boost::format("You can no longer select the external SMAs for 10 MHz or 1 PPS signaling.\n");
  std::cout << boost::format("****************************************************************************************************************\n");
}

int UHD_SAFE_MAIN(int argc, char *argv[]){
  uhd::set_thread_priority_safe();

  std::string args;

  //Set up program options
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help", "help message")
    ("args", po::value<std::string>(&args)->default_value(""), "Specify a single USRP.")
    ;
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  //Print the help message
  if (vm.count("help")) {
    std::cout << boost::format("Query GPSDO Sensors %s") % desc << std::endl;
    return EXIT_FAILURE;
  }

  //Create a USRP device
  std::cout << boost::format("\nCreating the USRP device with: %s...\n") % args;
  uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);
  std::cout << boost::format("Using Device: %s\n") % usrp->get_pp_string();

  print_notes();


  //Verify GPS sensors are present (i.e. EEPROM has been burnt)
  std::vector<std::string> sensor_names = usrp->get_mboard_sensor_names(0);

  if(std::find(sensor_names.begin(), sensor_names.end(), "gps_locked") == sensor_names.end()) {
    std::cout << boost::format("\ngps_locked sensor not found.  This could mean that you have not installed the GPSDO correctly.\n\n");
    std::cout << boost::format("Visit this page if the problem persists:\n");
    std::cout << boost::format("http://files.ettus.com/uhd_docs/manual/html/gpsdo.html\n\n");
    exit(EXIT_FAILURE);
  }

  //Check for GPS lock
  uhd::sensor_value_t gps_locked = usrp->get_mboard_sensor("gps_locked",0);
  if(not gps_locked.to_bool()) {
    std::cout << boost::format("\nGPS does not have lock. Wait a few minutes and try again.\n");
    std::cout << boost::format("NMEA strings and device time may not be accurate until lock is achieved.\n\n");
  } else
      std::cout << boost::format("GPS Locked\n");

  //Check for 10 MHz lock
  if(std::find(sensor_names.begin(), sensor_names.end(), "ref_locked") != sensor_names.end()) {
    uhd::sensor_value_t gps_locked = usrp->get_mboard_sensor("ref_locked",0);
    if(not gps_locked.to_bool()) {
      std::cout << boost::format("USRP NOT Locked to GPSDO 10 MHz Reference.\n");
      std::cout << boost::format("Double check installation instructions: https://www.ettus.com/content/files/gpsdo-kit_2.pdf\n\n");
    } else
        std::cout << boost::format("USRP Locked to GPSDO 10 MHz Reference.\n");
  }else
    std::cout << boost::format("ref_locked sensor not present on this board.\n");

  //Check PPS and compare UHD device time to GPS time
  boost::this_thread::sleep(boost::posix_time::seconds(1));
  uhd::sensor_value_t gps_time = usrp->get_mboard_sensor("gps_time");
  const uhd::time_spec_t last_pps_time = usrp->get_time_last_pps();
  if (last_pps_time.to_ticks(1.0) == gps_time.to_int()) {
    std::cout << boost::format("GPS and UHD Device time are aligned.\n");
  } else
    std::cout << boost::format("\nGPS and UHD Device time are NOT aligned. Try re-running the program. Double check 1 PPS connection from GPSDO.\n\n");
    
  //print NMEA strings
  std::cout << boost::format("Printing available NMEA strings:\n");
  uhd::sensor_value_t gga_string = usrp->get_mboard_sensor("gps_gpgga");
  uhd::sensor_value_t rmc_string = usrp->get_mboard_sensor("gps_gprmc");
  std::cout << boost::format("%s\n%s\n%s\n") % gga_string.to_pp_string() % rmc_string.to_pp_string() % gps_time.to_pp_string();
  std::cout << boost::format("UHD Device time: %.0f seconds\n") % (last_pps_time.get_real_secs());

  //finished
  std::cout << boost::format("\nDone!\n\n");

  return EXIT_SUCCESS;
}
