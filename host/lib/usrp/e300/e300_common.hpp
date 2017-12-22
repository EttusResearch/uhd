//
// Copyright 2014 Ettus Research LLC
//
// SPDX-License-Identifier: GPL-3.0
//

#ifndef INCLUDED_E300_COMMON_HPP
#define INCLUDED_E300_COMMON_HPP

#include <string>

namespace uhd { namespace usrp { namespace e300 {

namespace common {

void load_fpga_image(const std::string &path);

};

}}}

#endif // INCLUDED_E300_COMMON_HPP
