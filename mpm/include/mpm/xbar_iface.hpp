//
// Copyright 2017 Ettus Research (National Instruments)
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
#pragma once
#include <boost/noncopyable.hpp>
#include <memory>
#include <mutex>
#include <cstdint>

namespace mpm{

/*!
 * Crossbar route command
 */
using rfnoc_crossbar_cmd =  struct rfnoc_crossbar_cmd {
    /*! destination address */
	uint8_t dest_addr;
    /*! destination port */
	uint8_t dest_port;
};

#define RFNCBWROUTIOC _IOW('R', 1, struct rfnoc_crossbar_cmd)
#define RFNCDELROUTIOC _IOW('D', 1, struct rfnoc_crossbar_cmd)

/*!
 * Crossbar interface class holding a crossbar context
 */
class xbar_iface: boost::noncopyable{
public:
    // use static mutex! lock_guard
    using sptr = std::shared_ptr<xbar_iface>;
    static sptr make(const std::string &device);
    void set_route(uint8_t dst_addr, uint8_t dst_port);
    void del_route(uint8_t dst_addr, uint8_t dst_port);
    ~xbar_iface();
    xbar_iface(const std::string &device);

private:
    static std::mutex _lock;
    int _fd;
};
}


#ifdef LIBMPM_PYTHON
void export_xbar(){
    LIBMPM_BOOST_PREAMBLE("xbar")
    bp::class_<mpm::xbar_iface, boost::noncopyable, std::shared_ptr<mpm::xbar_iface> >("xbar", bp::no_init)
        .def("make", &mpm::xbar_iface::make)
        .staticmethod("make")
        .def("set_route", &mpm::xbar_iface::set_route)
        .def("del_route", &mpm::xbar_iface::del_route)
    ;
}
#endif

