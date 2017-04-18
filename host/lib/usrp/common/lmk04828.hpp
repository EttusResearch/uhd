//
// Copyright 2017 Ettus Research LLC
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

#ifndef INCLUDE_LMK04828_HPP
#define INCLUDE_LMK04828_HPP

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <vector>
#include <stdint.h>

static const uint32_t LMK04828_ID_DEVICE_TYPE = 6;
static const uint32_t LMK04828_ID_PROD_LSB    = 91;
static const uint32_t LMK04828_ID_PROD_MSB    = 208;
static const uint32_t LMK04828_ID_MASKREV     = 32;

class lmk04828_iface
{
public:
    typedef boost::shared_ptr<lmk04828_iface> sptr;
    typedef boost::function<void(std::vector<uint32_t>)> write_fn_t; 
    typedef boost::function<uint8_t(uint32_t)> read_fn_t;

    //static sptr (write_fn_t write_fn, read_fn_t read_fn);
    lmk04828_iface(write_fn_t, read_fn_t);

    ~lmk04828_iface() {}

    void verify_chip_id();

    uint8_t get_chip_id();

    void init();

    void send_sysref_pulse();

private:
    // use IC Reg Map once values stabilize
//    lmk04828_regs_t _regs;
    
    write_fn_t _write_fn;
    read_fn_t _read_fn;
};
#endif
