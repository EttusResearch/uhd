//
// Copyright 2010-2011 Ettus Research LLC
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

#include <uhd/usrp/gps_ctrl.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/utils/props.hpp>
#include <uhd/exception.hpp>
#include <uhd/types/sensors.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/cstdint.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp>
#include <boost/tokenizer.hpp>

using namespace uhd;
using namespace boost::gregorian;
using namespace boost::posix_time;
using namespace boost::algorithm;
using namespace boost::this_thread;

/*!
 * A GPS control for Jackson Labs devices (and other NMEA compatible GPS's)
 */

class gps_ctrl_impl : public gps_ctrl{
public:
  gps_ctrl_impl(gps_send_fn_t send, gps_recv_fn_t recv){
    _send = send;
    _recv = recv;

    std::string reply;
    bool i_heard_some_nmea = false, i_heard_something_weird = false;
    gps_type = GPS_TYPE_NONE;
    
    //first we look for a Jackson Labs Firefly (since that's what we provide...)
    _recv(); //get whatever junk is in the rx buffer right now, and throw it away
    _send("HAAAY GUYYYYS\n"); //to elicit a response from the Firefly

    //then we loop until we either timeout, or until we get a response that indicates we're a JL device
    int timeout = GPS_TIMEOUT_TRIES;
    while(timeout--) {
      reply = _recv();
      if(reply.find("Command Error") != std::string::npos) {
        gps_type = GPS_TYPE_JACKSON_LABS;
        break;
      } 
      else if(reply.substr(0, 3) == "$GP") i_heard_some_nmea = true; //but keep looking for that "Command Error" response
      else if(reply.length() != 0) i_heard_something_weird = true; //probably wrong baud rate
      sleep(milliseconds(200));
    }

    if((i_heard_some_nmea) && (gps_type != GPS_TYPE_JACKSON_LABS)) gps_type = GPS_TYPE_GENERIC_NMEA;

    if((gps_type == GPS_TYPE_NONE) && i_heard_something_weird) {
      UHD_MSG(error) << "GPS invalid reply \"" << reply << "\", assuming none available" << std::endl;
    }

    switch(gps_type) {
    case GPS_TYPE_JACKSON_LABS:
      UHD_MSG(status) << "Found a Jackson Labs GPS" << std::endl;
      init_firefly();

    case GPS_TYPE_GENERIC_NMEA:
      if(gps_type == GPS_TYPE_GENERIC_NMEA) UHD_MSG(status) << "Found a generic NMEA GPS device" << std::endl;
      if(get_time() == ptime()) {
          UHD_MSG(status) << "No valid GPRMC packet found. Ignoring discovered GPS.";
          gps_type = GPS_TYPE_NONE;
      }
      break;

    case GPS_TYPE_NONE:
    default:
      break;

    }
  }

  ~gps_ctrl_impl(void){
    /* NOP */
  }

  //return a list of supported sensors
  std::vector<std::string> get_sensors(void) {
    std::vector<std::string> ret = boost::assign::list_of
        ("gps_gpgga")
        ("gps_gprmc")
        ("gps_gpgsa")
        ("gps_time")
        ("gps_locked");
    return ret;
  }

  uhd::sensor_value_t get_sensor(std::string key) {
    if(key == "gps_gpgga"
    or key == "gps_gprmc"
    or key == "gps_gpgsa") {
        return sensor_value_t(
                 boost::to_upper_copy(key),
                 get_nmea(boost::to_upper_copy(key.substr(4,8))),
                 "");
    }
    else if(key == "gps_time") {
        return sensor_value_t("GPS epoch time", int(get_epoch_time()), "seconds");
    }
    else if(key == "gps_locked") {
        return sensor_value_t("GPS lock status", locked(), "locked", "unlocked");
    }
    else {
        UHD_THROW_PROP_GET_ERROR();
    }
  }

private:
  void init_firefly(void) {
    //issue some setup stuff so it spits out the appropriate data
    //none of these should issue replies so we don't bother looking for them
    //we have to sleep between commands because the JL device, despite not acking, takes considerable time to process each command.
     sleep(milliseconds(FIREFLY_STUPID_DELAY_MS));
    _send("SYST:COMM:SER:ECHO OFF\n");
     sleep(milliseconds(FIREFLY_STUPID_DELAY_MS));
    _send("SYST:COMM:SER:PRO OFF\n");
     sleep(milliseconds(FIREFLY_STUPID_DELAY_MS));
    _send("GPS:GPGGA 1\n");
     sleep(milliseconds(FIREFLY_STUPID_DELAY_MS));
    _send("GPS:GGAST 0\n");
     sleep(milliseconds(FIREFLY_STUPID_DELAY_MS));
    _send("GPS:GPRMC 1\n");
     sleep(milliseconds(FIREFLY_STUPID_DELAY_MS));
    _send("GPS:GPGSA 1\n");
     sleep(milliseconds(FIREFLY_STUPID_DELAY_MS));
  }
 
  //retrieve a raw NMEA sentence
  std::string get_nmea(std::string msgtype) {
    msgtype.insert(0, "$");
    std::string reply;
    if(not gps_detected()) {
        UHD_MSG(error) << "get_nmea(): unsupported GPS or no GPS detected";
        return std::string();
    }
    int timeout = GPS_TIMEOUT_TRIES;
    while(timeout--) {
        reply = _recv();
        if(reply.substr(0, 6) == msgtype)
          return reply;
        boost::this_thread::sleep(milliseconds(GPS_TIMEOUT_DELAY_MS));
    }
    UHD_MSG(error) << "get_nmea(): no " << msgtype << " message found";
    return std::string();
  }

  //helper function to retrieve a field from an NMEA sentence
  std::string get_token(std::string sentence, size_t offset) {
    boost::tokenizer<boost::escaped_list_separator<char> > tok(sentence);
    std::vector<std::string> toked;

    tok.assign(sentence);
    toked.assign(tok.begin(), tok.end());

    if(toked.size() <= offset) {
      UHD_MSG(error) << "get_token: too few tokens in reply " << sentence;
      return std::string();
    }
    return toked[offset];
  }

  ptime get_time(void) {
    std::string reply = get_nmea("GPRMC");

    std::string datestr = get_token(reply, 9);
    std::string timestr = get_token(reply, 1);

    if(datestr.size() == 0 or timestr.size() == 0) {
        return ptime();
    }

    //just trust me on this one
    return ptime( date( 
                     greg_year(boost::lexical_cast<int>(datestr.substr(4, 2)) + 2000),
                     greg_month(boost::lexical_cast<int>(datestr.substr(2, 2))), 
                     greg_day(boost::lexical_cast<int>(datestr.substr(0, 2))) 
                   ),
                  hours(  boost::lexical_cast<int>(timestr.substr(0, 2)))
                + minutes(boost::lexical_cast<int>(timestr.substr(2, 2)))
                + seconds(boost::lexical_cast<int>(timestr.substr(4, 2)))
             );
  }
  
  time_t get_epoch_time(void) {
      return (get_time() - from_time_t(0)).total_seconds();
  }

  bool gps_detected(void) {
    return (gps_type != GPS_TYPE_NONE);
  }

  bool locked(void) {
      std::string reply = get_nmea("GPGGA");
      if(reply.size() <= 1) return false;

      return (get_token(reply, 6) != "0");
  }

  gps_send_fn_t _send;
  gps_recv_fn_t _recv;

  enum {
    GPS_TYPE_JACKSON_LABS,
    GPS_TYPE_GENERIC_NMEA,
    GPS_TYPE_NONE
  } gps_type;

  static const int GPS_TIMEOUT_TRIES = 10;
  static const int GPS_TIMEOUT_DELAY_MS = 200;
  static const int FIREFLY_STUPID_DELAY_MS = 200;
};

/***********************************************************************
 * Public make function for the GPS control
 **********************************************************************/
gps_ctrl::sptr gps_ctrl::make(gps_send_fn_t send, gps_recv_fn_t recv){
    return sptr(new gps_ctrl_impl(send, recv));
}
