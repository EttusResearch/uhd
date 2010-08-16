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

#include "gps_ctrl.hpp"
#include <uhd/utils/assert.hpp>
#include <boost/cstdint.hpp>
#include <string>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/tokenizer.hpp>

using namespace uhd;
using namespace boost::gregorian;
using namespace boost::posix_time;
using namespace boost::algorithm;

/*!
 * A usrp2 GPS control for Jackson Labs devices
 */

//TODO: multiple baud rate support (requires mboard_impl changes for poking UART registers)
class usrp2_gps_ctrl_impl : public usrp2_gps_ctrl{
public:
  usrp2_gps_ctrl_impl(usrp2_iface::sptr iface){
    _iface = iface;

    std::string reply;
    bool i_heard_some_nmea = false, i_heard_something_weird = false;

    gps_type = GPS_TYPE_NONE;

//    set_uart_baud_rate(GPS_UART, 115200);
    //first we look for a Jackson Labs Firefly (since that's what we sell with the USRP2+...)

    _iface->read_uart(GPS_UART); //get whatever junk is in the rx buffer right now, and throw it away
    _iface->write_uart(GPS_UART, "HAAAY GUYYYYS\n"); //to elicit a response from the Firefly

    //then we loop until we either timeout, or until we get a response that indicates we're a JL device
    int timeout = GPS_TIMEOUT_TRIES;
    while(timeout--) {
      reply = safe_gps_read();
      if(trim_right_copy(reply) == "Command Error") {
        gps_type = GPS_TYPE_JACKSON_LABS;
        break;
      }
      else if(reply.substr(0, 3) == "$GP") i_heard_some_nmea = true; //but keep looking for that "Command Error" response
      else if(reply.length() != 0) i_heard_something_weird = true; //probably wrong baud rate
    }

    if((i_heard_some_nmea) && (gps_type != GPS_TYPE_JACKSON_LABS)) gps_type = GPS_TYPE_GENERIC_NMEA;

    //otherwise, we can try some other common baud rates looking to see if a GPS is connected (todo, later)
    if((gps_type == GPS_TYPE_NONE) && i_heard_something_weird) {
      std::cout << "Invalid reply, possible incorrect baud rate" << std::endl;
    }

    bool found_gprmc = false;

    switch(gps_type) {
    case GPS_TYPE_JACKSON_LABS:
      std::cout << "Found a Jackson Labs GPS" << std::endl;
      //issue some setup stuff so it spits out the appropriate data
      //none of these should issue replies so we don't bother looking for them
      //we have to sleep between commands because the JL device, despite not acking, takes considerable time to process each command.
       boost::this_thread::sleep(boost::posix_time::milliseconds(FIREFLY_STUPID_DELAY_MS));
      _iface->write_uart(GPS_UART, "SYST:COMM:SER:ECHO OFF\n");
       boost::this_thread::sleep(boost::posix_time::milliseconds(FIREFLY_STUPID_DELAY_MS));
      _iface->write_uart(GPS_UART, "SYST:COMM:SER:PRO OFF\n");
       boost::this_thread::sleep(boost::posix_time::milliseconds(FIREFLY_STUPID_DELAY_MS));
      _iface->write_uart(GPS_UART, "GPS:GPGGA 0\n");
       boost::this_thread::sleep(boost::posix_time::milliseconds(FIREFLY_STUPID_DELAY_MS));
      _iface->write_uart(GPS_UART, "GPS:GGAST 0\n");
       boost::this_thread::sleep(boost::posix_time::milliseconds(FIREFLY_STUPID_DELAY_MS));
      _iface->write_uart(GPS_UART, "GPS:GPRMC 1\n");
       boost::this_thread::sleep(boost::posix_time::milliseconds(FIREFLY_STUPID_DELAY_MS));

//      break;

    case GPS_TYPE_GENERIC_NMEA:
      if(gps_type == GPS_TYPE_GENERIC_NMEA) std::cout << "Found a generic NMEA GPS device" << std::endl;
      found_gprmc = false;
      //here we loop around looking for a GPRMC packet. if we don't get one, we don't have a usable GPS.
      timeout = GPS_TIMEOUT_TRIES;
      while(timeout--) {
        reply = safe_gps_read();
        if(reply.substr(0, 6) == "$GPRMC") {
          found_gprmc = true;
          break;
        }
      }
      if(!found_gprmc) {
        if(gps_type == GPS_TYPE_JACKSON_LABS) std::cout << "Firefly GPS not locked or warming up." << std::endl;
        else std::cout << "GPS does not output GPRMC packets. Cannot retrieve time." << std::endl;
        gps_type = GPS_TYPE_NONE;
      }
      break;

    case GPS_TYPE_NONE:
    default:
      break;

    }


  }

  ~usrp2_gps_ctrl_impl(void){

  }

  std::string safe_gps_read() {
    std::string reply;
    try {
        reply = _iface->read_uart(GPS_UART);
  	    //std::cerr << "Got reply from GPS: " << reply.c_str() << " with length = " << reply.length() << std::endl;
    } catch (std::runtime_error err) {
      if(err.what() != std::string("usrp2 no control response")) throw; //sorry can't cope with that
      else { //we don't actually have a GPS installed
        reply = std::string();
      }
    }
    return reply;
  }

  ptime get_time(void) {
    std::string reply;
    ptime now;
    boost::tokenizer<boost::escaped_list_separator<char> > tok(reply);
    std::vector<std::string> toked;
    int timeout = GPS_TIMEOUT_TRIES;
    bool found_gprmc = false;
    switch(gps_type) {
    case GPS_TYPE_JACKSON_LABS: //deprecated in favor of a single NMEA parser
    case GPS_TYPE_GENERIC_NMEA:

      while(timeout--) {
        reply = safe_gps_read();
        if(reply.substr(0, 6) == "$GPRMC") {
          found_gprmc = true;
          break;
        }
      }
      UHD_ASSERT_THROW(found_gprmc);

      tok.assign(reply);
      toked.assign(tok.begin(), tok.end());

      UHD_ASSERT_THROW(toked.size() == 11); //if it's not we got something weird in there

      now = ptime( date( 
                         greg_year(boost::lexical_cast<int>(toked[8].substr(4, 2)) + 2000), //just trust me on this one
                         greg_month(boost::lexical_cast<int>(toked[8].substr(2, 2))), 
                         greg_day(boost::lexical_cast<int>(toked[8].substr(0, 2))) 
                       ),
                   hours(  boost::lexical_cast<int>(toked[1].substr(0, 2)))
                 + minutes(boost::lexical_cast<int>(toked[1].substr(2, 2)))
                 + seconds(boost::lexical_cast<int>(toked[1].substr(4, 2)))
                 );
      break;
    case GPS_TYPE_NONE:
    default:
      throw std::runtime_error("get_time(): Unsupported GPS or no GPS detected\n");
      break;
    }
    return now;
  }

  bool gps_detected(void) {
    return (gps_type != GPS_TYPE_NONE);
  }

private:
  usrp2_iface::sptr _iface;

  enum {
    GPS_TYPE_JACKSON_LABS,
    GPS_TYPE_GENERIC_NMEA,
    GPS_TYPE_NONE
  } gps_type;

  static const int GPS_UART = 2; //TODO: this should be plucked from fw_common.h or memory_map.h or somewhere in common with the firmware
  static const int GPS_TIMEOUT_TRIES = 5;
  static const int FIREFLY_STUPID_DELAY_MS = 200;

};

/***********************************************************************
 * Public make function for the GPS control
 **********************************************************************/
usrp2_gps_ctrl::sptr usrp2_gps_ctrl::make(usrp2_iface::sptr iface){
    return sptr(new usrp2_gps_ctrl_impl(iface));
}
