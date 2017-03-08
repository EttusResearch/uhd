//
// Copyright 2015 Ettus Research LLC
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

class lmk04828_iface
{
public:
    typedef boost::shared_ptr<lmk04828_iface> sptr;
    typedef boost::function<void(std::vector<uint32_t>)> write_fn_t; 
    typedef boost::function<uint8_t(uint32_t)> read_fn_t;

    static sptr make(write_fn_t write_fn, read_fn_t read_fn);

    virtual ~lmk04828_iface() {}

    virtual void verify_chip_id() = 0;

    virtual uint8_t get_chip_id() = 0;

    virtual void init() = 0;

    virtual void send_sysref_pulse() = 0;
};
#endif
