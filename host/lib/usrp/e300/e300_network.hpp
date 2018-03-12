//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_E300_NETWORK_HPP
#define INCLUDED_E300_NETWORK_HPP

#include <string>
#include <boost/noncopyable.hpp>

#include <uhd/device.hpp>


static const std::string E310_FPGA_FILE_NAME = "usrp_e310_fpga.bit";
static const std::string E300_FPGA_FILE_NAME = "usrp_e300_fpga.bit";

namespace uhd { namespace usrp { namespace e300 {

class UHD_API network_server : boost::noncopyable
{
public:
    typedef boost::shared_ptr<network_server> sptr;
    virtual void run(void) = 0;

    static sptr make(const uhd::device_addr_t &device_addr);
};


}}}
#endif // INCLUDED_E300_NETWORK_HPP
