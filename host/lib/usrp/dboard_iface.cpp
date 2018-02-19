//
// Copyright 2016 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
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
