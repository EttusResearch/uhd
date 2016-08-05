//
// Copyright 2016 Ettus Research
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

#include <uhd/usrp/dboard_iface.hpp>

using namespace uhd::usrp;

void dboard_iface::sleep(const boost::chrono::nanoseconds& time)
{
   //nanosleep is not really accurate in userland and it is also not very
   //cross-platform. So just sleep for the minimum amount of time in us.
   if (time < boost::chrono::microseconds(1)) {
      boost::this_thread::sleep_for(boost::chrono::microseconds(1));
   } else {
      boost::this_thread::sleep_for(time);
   }
}
