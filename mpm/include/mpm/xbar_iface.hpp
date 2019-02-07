//
// Copyright 2017 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#pragma once
#include <boost/noncopyable.hpp>
#include <cstdint>
#include <memory>
#include <mutex>

namespace mpm {

/*!
 * Crossbar route command
 */
using rfnoc_crossbar_cmd = struct rfnoc_crossbar_cmd
{
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
class xbar_iface : boost::noncopyable
{
public:
    // use static mutex! lock_guard
    using sptr = std::shared_ptr<xbar_iface>;
    static sptr make(const std::string& device);
    void set_route(uint8_t dst_addr, uint8_t dst_port);
    void del_route(uint8_t dst_addr, uint8_t dst_port);
    ~xbar_iface();
    xbar_iface(const std::string& device);

private:
    static std::mutex _lock;
    int _fd;
};
} // namespace mpm


#ifdef LIBMPM_PYTHON
void export_xbar(py::module& top_module)
{
    auto m = top_module.def_submodule("xbar");

    py::class_<mpm::xbar_iface, std::shared_ptr<mpm::xbar_iface>>(m, "xbar")
        .def(py::init(&mpm::xbar_iface::make))
        .def("set_route", &mpm::xbar_iface::set_route)
        .def("del_route", &mpm::xbar_iface::del_route);
}
#endif
