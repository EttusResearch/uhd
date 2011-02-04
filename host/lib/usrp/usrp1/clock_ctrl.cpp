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

#include "clock_ctrl.hpp"
#include "fpga_regs_standard.h"
#include <uhd/utils/assert.hpp>
#include <boost/cstdint.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/foreach.hpp>
#include <utility>
#include <iostream>

using namespace uhd;

/***********************************************************************
 * Constants
 **********************************************************************/
static const double default_master_clock_rate = 64e6;

/***********************************************************************
 * Clock Control Implementation
 **********************************************************************/
class usrp1_clock_ctrl_impl : public usrp1_clock_ctrl {
public:
    usrp1_clock_ctrl_impl(usrp1_iface::sptr iface): _iface(iface){
        this->set_master_clock_freq(default_master_clock_rate);
    }

    void set_master_clock_freq(double freq){
        _freq = freq;
    }

    double get_master_clock_freq(void){
        return _freq;
    }

private:
    usrp1_iface::sptr _iface;
    double _freq;
};

/***********************************************************************
 * Clock Control Make
 **********************************************************************/
usrp1_clock_ctrl::sptr usrp1_clock_ctrl::make(usrp1_iface::sptr iface){
    return sptr(new usrp1_clock_ctrl_impl(iface));
}
