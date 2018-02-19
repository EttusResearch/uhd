//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_GPSD_IFACE_HPP
#define INCLUDED_GPSD_IFACE_HPP

#include <stdint.h>
#include <boost/shared_ptr.hpp>

#include <uhd/usrp/gps_ctrl.hpp>

namespace uhd { namespace usrp {

class gpsd_iface : public virtual uhd::gps_ctrl {
public:
    typedef boost::shared_ptr<gpsd_iface> sptr;
    static sptr make(const std::string &addr, uint16_t port);
};

}};

#endif /* INCLUDED_GPSD_IFACE_HPP */
