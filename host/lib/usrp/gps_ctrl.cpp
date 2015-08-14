//
// Copyright 2010-2011,2014-2015 Ettus Research LLC
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
#include <uhd/utils/log.hpp>
#include <uhd/exception.hpp>
#include <uhd/types/sensors.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/cstdint.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp>
#include <boost/tokenizer.hpp>
#include <boost/format.hpp>
#include <boost/regex.hpp>

#include "boost/tuple/tuple.hpp"
#include "boost/foreach.hpp"

using namespace uhd;
using namespace boost::gregorian;
using namespace boost::posix_time;
using namespace boost::algorithm;
using namespace boost::this_thread;

/*!
 * A control for GPSDO devices
 */

gps_ctrl::~gps_ctrl(void){
    /* NOP */
}

class gps_ctrl_impl : public gps_ctrl{
private:
  std::map<std::string, boost::tuple<std::string, boost::system_time, bool> > sensors;

  std::string get_cached_sensor(const std::string sensor, const int freshness, const bool once, const bool touch=true) {
    boost::system_time time = boost::get_system_time();
    try {
      // this is nasty ...
      //std::cout << boost::format("Requested %s - seen? ") % sensor << sensors[sensor].get<2>() << " once? " << once << std::endl;
      if(time - sensors[sensor].get<1>() < milliseconds(freshness) && (!once or !sensors[sensor].get<2>())) {
        sensors[sensor] = boost::make_tuple(sensors[sensor].get<0>(), sensors[sensor].get<1>(), touch);
        return sensors[sensor].get<0>();
      } else {
          return update_cached_sensors(sensor);
      }
    } catch(std::exception &e) {
      UHD_MSG(warning) << "get_cached_sensor: " << e.what();
    }
    return std::string();
  }

    static bool is_nmea_checksum_ok(std::string nmea)
    {
        if (nmea.length() < 5 || nmea[0] != '$' || nmea[nmea.length()-3] != '*')
            return false;

        std::stringstream ss;
        boost::uint32_t string_crc;
        boost::uint32_t calculated_crc = 0;

        // get crc from string
        ss << std::hex << nmea.substr(nmea.length()-2, 2);
        ss >> string_crc;

        // calculate crc
        for (size_t i = 1; i < nmea.length()-3; i++)
            calculated_crc ^= nmea[i];

        // return comparison
        return (string_crc == calculated_crc);
    }

  std::string update_cached_sensors(const std::string sensor) {
    if(not gps_detected() || (gps_type != GPS_TYPE_INTERNAL_GPSDO)) {
        UHD_MSG(error) << "get_stat(): unsupported GPS or no GPS detected";
        return std::string();
    }

    const std::list<std::string> list = boost::assign::list_of("GPGGA")("GPRMC")("SERVO");
    static const boost::regex status_regex("\\d\\d-\\d\\d-\\d\\d");
    static const boost::regex gp_msg_regex("^\\$GP.*,\\*[0-9A-F]{2}$");
    std::map<std::string,std::string> msgs;

    // Get all GPSDO messages available
    // Creating a map here because we only want the latest of each message type
    for (std::string msg = _recv(); msg.length(); msg = _recv())
    {
        // Strip any end of line characters
        erase_all(msg, "\r");
        erase_all(msg, "\n");

        if (msg.length() < 6)
        {
            UHD_LOGV(regularly) << __FUNCTION__ << ": Short NMEA string: " << msg << std::endl;
            continue;
        }

        // Look for SERVO message
        if (boost::regex_search(msg, status_regex, boost::regex_constants::match_continuous))
        {
            msgs["SERVO"] = msg;
        }
        else if (boost::regex_match(msg, gp_msg_regex) and is_nmea_checksum_ok(msg))
        {
            msgs[msg.substr(1,5)] = msg;
        }
        else
        {
            UHD_LOGV(regularly) << __FUNCTION__ << ": Malformed NMEA string: " << msg << std::endl;
        }
    }

    boost::system_time time = boost::get_system_time();

    // Update sensors with newly read data
    BOOST_FOREACH(std::string key, list) {
        if (msgs[key].length())
            sensors[key] = boost::make_tuple(msgs[key], time, !sensor.compare(key));
    }

    // Return requested sensor if it was updated
    if (msgs[sensor].length())
        return msgs[sensor];

    return std::string();
  }

public:
  gps_ctrl_impl(uart_iface::sptr uart){
    _uart = uart;


    std::string reply;
    bool i_heard_some_nmea = false, i_heard_something_weird = false;
    gps_type = GPS_TYPE_NONE;

    //first we look for an internal GPSDO
    _flush(); //get whatever junk is in the rx buffer right now, and throw it away
    _send("HAAAY GUYYYYS\n"); //to elicit a response from the GPSDO

    //wait for _send(...) to return
    sleep(milliseconds(GPSDO_COMMAND_DELAY_MS));

    //then we loop until we either timeout, or until we get a response that indicates we're a JL device
    const boost::system_time comm_timeout = boost::get_system_time() + milliseconds(GPS_COMM_TIMEOUT_MS);
    while(boost::get_system_time() < comm_timeout) {
      reply = _recv();
      if(reply.find("Command Error") != std::string::npos) {
        gps_type = GPS_TYPE_INTERNAL_GPSDO;
        break;
      }
      else if(reply.substr(0, 3) == "$GP") i_heard_some_nmea = true; //but keep looking for that "Command Error" response
      else if(reply.length() != 0) i_heard_something_weird = true; //probably wrong baud rate
    }

    if((i_heard_some_nmea) && (gps_type != GPS_TYPE_INTERNAL_GPSDO)) gps_type = GPS_TYPE_GENERIC_NMEA;

    if((gps_type == GPS_TYPE_NONE) && i_heard_something_weird) {
      UHD_MSG(error) << "GPS invalid reply \"" << reply << "\", assuming none available" << std::endl;
    }

    switch(gps_type) {
    case GPS_TYPE_INTERNAL_GPSDO:
      UHD_MSG(status) << "Found an internal GPSDO" << std::endl;
      init_gpsdo();
      break;

    case GPS_TYPE_GENERIC_NMEA:
      UHD_MSG(status) << "Found a generic NMEA GPS device" << std::endl;
      break;

    case GPS_TYPE_NONE:
    default:
      UHD_MSG(status) << "No GPSDO found" << std::endl;
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
        ("gps_time")
        ("gps_locked")
        ("gps_servo");
    return ret;
  }

  uhd::sensor_value_t get_sensor(std::string key) {
    if(key == "gps_gpgga"
    or key == "gps_gprmc") {
        return sensor_value_t(
                 boost::to_upper_copy(key),
                 get_cached_sensor(boost::to_upper_copy(key.substr(4,8)), GPS_NMEA_NORMAL_FRESHNESS, false, false),
                 "");
    }
    else if(key == "gps_time") {
        return sensor_value_t("GPS epoch time", int(get_epoch_time()), "seconds");
    }
    else if(key == "gps_locked") {
        return sensor_value_t("GPS lock status", locked(), "locked", "unlocked");
    }
    else if(key == "gps_servo") {
        return sensor_value_t("GPS servo status", get_servo(), "");
    }
    else {
        throw uhd::value_error("gps ctrl get_sensor unknown key: " + key);
    }
  }

private:
  void init_gpsdo(void) {
    //issue some setup stuff so it spits out the appropriate data
    //none of these should issue replies so we don't bother looking for them
    //we have to sleep between commands because the JL device, despite not acking, takes considerable time to process each command.
     sleep(milliseconds(GPSDO_COMMAND_DELAY_MS));
    _send("SYST:COMM:SER:ECHO OFF\n");
     sleep(milliseconds(GPSDO_COMMAND_DELAY_MS));
    _send("SYST:COMM:SER:PRO OFF\n");
     sleep(milliseconds(GPSDO_COMMAND_DELAY_MS));
    _send("GPS:GPGGA 1\n");
     sleep(milliseconds(GPSDO_COMMAND_DELAY_MS));
    _send("GPS:GGAST 0\n");
     sleep(milliseconds(GPSDO_COMMAND_DELAY_MS));
    _send("GPS:GPRMC 1\n");
     sleep(milliseconds(GPSDO_COMMAND_DELAY_MS));
    _send("SERV:TRAC 0\n");
     sleep(milliseconds(GPSDO_COMMAND_DELAY_MS));
  }

  //retrieve a raw NMEA sentence
  std::string get_nmea(std::string msgtype) {
    std::string reply;

    const boost::system_time comm_timeout = boost::get_system_time() + milliseconds(GPS_COMM_TIMEOUT_MS);
    while(boost::get_system_time() < comm_timeout) {
        if(!msgtype.compare("GPRMC")) {
          reply = get_cached_sensor(msgtype, GPS_NMEA_FRESHNESS, true);
        }
        else {
          reply = get_cached_sensor(msgtype, GPS_NMEA_LOW_FRESHNESS, false);
        }
        if(reply.size()) {
          if(reply.substr(1, 5) == msgtype) return reply;
        }
        boost::this_thread::sleep(milliseconds(GPS_TIMEOUT_DELAY_MS));
    }
    throw uhd::value_error(str(boost::format("get_nmea(): no %s message found") % msgtype));
  }

  //helper function to retrieve a field from an NMEA sentence
  std::string get_token(std::string sentence, size_t offset) {
    boost::tokenizer<boost::escaped_list_separator<char> > tok(sentence);
    std::vector<std::string> toked;
    tok.assign(sentence); //this can throw
    toked.assign(tok.begin(), tok.end());

    if(toked.size() <= offset) {
        throw uhd::value_error(str(boost::format("Invalid response \"%s\"") % sentence));
    }
    return toked[offset];
  }

  ptime get_time(void) {
    _flush();
    int error_cnt = 0;
    ptime gps_time;
    while(error_cnt < 2) {
        try {
            std::string reply = get_nmea("GPRMC");

            std::string datestr = get_token(reply, 9);
            std::string timestr = get_token(reply, 1);

            if(datestr.size() == 0 or timestr.size() == 0) {
                throw uhd::value_error(str(boost::format("Invalid response \"%s\"") % reply));
            }

            //just trust me on this one
            gps_time = ptime( date(
                             greg_year(boost::lexical_cast<int>(datestr.substr(4, 2)) + 2000),
                             greg_month(boost::lexical_cast<int>(datestr.substr(2, 2))),
                             greg_day(boost::lexical_cast<int>(datestr.substr(0, 2)))
                           ),
                          hours(  boost::lexical_cast<int>(timestr.substr(0, 2)))
                        + minutes(boost::lexical_cast<int>(timestr.substr(2, 2)))
                        + seconds(boost::lexical_cast<int>(timestr.substr(4, 2)))
                     );
            return gps_time;

        } catch(std::exception &e) {
            UHD_MSG(warning) << "get_time: " << e.what();
            _flush();
            error_cnt++;
        }
    }
    throw uhd::value_error("Timeout after no valid message found");

    return gps_time; //keep gcc from complaining
  }

  time_t get_epoch_time(void) {
      return (get_time() - from_time_t(0)).total_seconds();
  }

  bool gps_detected(void) {
    return (gps_type != GPS_TYPE_NONE);
  }

  bool locked(void) {
    int error_cnt = 0;
    while(error_cnt < 3) {
        try {
            std::string reply = get_cached_sensor("GPGGA", GPS_LOCK_FRESHNESS, false, false);
            if(reply.size() <= 1) return false;

            return (get_token(reply, 6) != "0");
        } catch(std::exception &e) {
            UHD_MSG(warning) << "locked: " << e.what();
            error_cnt++;
        }
    }
    throw uhd::value_error("Timeout after no valid message found");
    return false;
  }

  std::string get_servo(void) {

    //enable servo reporting
    _send("SERV:TRAC 1\n");
    sleep(milliseconds(GPSDO_COMMAND_DELAY_MS));

    std::string reply;

    const boost::system_time comm_timeout = boost::get_system_time() + milliseconds(GPS_COMM_TIMEOUT_MS);
    while(boost::get_system_time() < comm_timeout) {
        reply = get_cached_sensor("SERVO", GPS_NMEA_LOW_FRESHNESS, false);
        if(reply.size())
        {
            //disable it before leaving function
            _send("SERV:TRAC 0\n");
            return reply;
        }
        boost::this_thread::sleep(milliseconds(GPS_TIMEOUT_DELAY_MS));
    }
    throw uhd::value_error("get_stat(): no servo message found");
  }

  uart_iface::sptr _uart;

  void _flush(void){
    while (not _uart->read_uart(0.0).empty()){
        //NOP
    }
  }

  std::string _recv(double timeout = GPS_TIMEOUT_DELAY_MS/1000.){
      return _uart->read_uart(timeout);
  }

  void _send(const std::string &buf){
      return _uart->write_uart(buf);
  }

  enum {
    GPS_TYPE_INTERNAL_GPSDO,
    GPS_TYPE_GENERIC_NMEA,
    GPS_TYPE_NONE
  } gps_type;

  static const int GPS_COMM_TIMEOUT_MS = 1300;
  static const int GPS_NMEA_FRESHNESS = 10;
  static const int GPS_NMEA_LOW_FRESHNESS = 2500;
  static const int GPS_NMEA_NORMAL_FRESHNESS = 1000;
  static const int GPS_SERVO_FRESHNESS = 2500;
  static const int GPS_LOCK_FRESHNESS = 2500;
  static const int GPS_TIMEOUT_DELAY_MS = 200;
  static const int GPSDO_COMMAND_DELAY_MS = 200;
};

/***********************************************************************
 * Public make function for the GPS control
 **********************************************************************/
gps_ctrl::sptr gps_ctrl::make(uart_iface::sptr uart){
    return sptr(new gps_ctrl_impl(uart));
}
