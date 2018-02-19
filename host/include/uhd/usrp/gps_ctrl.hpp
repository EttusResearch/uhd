//
// Copyright 2010-2011,2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_GPS_CTRL_HPP
#define INCLUDED_GPS_CTRL_HPP

#include <uhd/types/serial.hpp>
#include <uhd/types/sensors.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <boost/function.hpp>
#include <vector>

namespace uhd{

class UHD_API gps_ctrl : boost::noncopyable{
public:
  typedef boost::shared_ptr<gps_ctrl> sptr;

  virtual ~gps_ctrl(void) = 0;

  /*!
   * Make a GPS config for internal GPSDOs or generic NMEA GPS devices
   */
  static sptr make(uart_iface::sptr uart);

  /*!
   * Retrieve the list of sensors this GPS object provides
   */
  virtual std::vector<std::string> get_sensors(void) = 0;

  /*!
   * Retrieve the named sensor
   */
  virtual uhd::sensor_value_t get_sensor(std::string key) = 0;

  /*!
   * Tell you if there's a supported GPS connected or not
   * \return true if a supported GPS is connected
   */
  virtual bool gps_detected(void) = 0;

  //TODO: other fun things you can do with a GPS.

};

} //namespace uhd

#endif /* INCLUDED_GPS_CTRL_HPP */
