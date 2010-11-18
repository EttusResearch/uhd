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

#ifndef INCLUDED_GPS_CTRL_HPP
#define INCLUDED_GPS_CTRL_HPP

#include "usrp2_iface.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

using namespace boost::posix_time;

class usrp2_gps_ctrl : boost::noncopyable{
public:
  typedef boost::shared_ptr<usrp2_gps_ctrl> sptr;

  /*!
   * Make a GPS config for Jackson Labs or generic NMEA GPS devices
   */
  static sptr make(usrp2_iface::sptr iface);

  /*!
   * Get the current GPS time and date
   * \return current GPS time and date as boost::posix_time::ptime object
   */
  virtual ptime get_time(void) = 0;

  /*!
   * Tell you if there's a supported GPS connected or not
   * \return true if a supported GPS is connected
   */
  virtual bool gps_detected(void) = 0;

  //TODO: other fun things you can do with a GPS.

};

#endif /* INCLUDED_CLOCK_CTRL_HPP */
