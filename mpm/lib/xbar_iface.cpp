//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "mpm/xbar_iface.hpp"
#include <mpm/exception.hpp>
#include <boost/format.hpp>
#include <sys/ioctl.h>
#include <fcntl.h>

using namespace mpm;

std::mutex xbar_iface::_lock; // Initialize lock for all objects

xbar_iface::xbar_iface(const std::string &device){
    _fd = open(device.c_str(), O_RDWR);
}

xbar_iface::~xbar_iface(){
    close(_fd);
}

void xbar_iface::set_route(uint8_t dst_addr, uint8_t dst_port) {
    std::lock_guard<std::mutex> lock(_lock);
    rfnoc_crossbar_cmd cmd = {.dest_addr = dst_addr, .dest_port = dst_port};
    int err = ioctl(_fd, RFNCBWROUTIOC, &cmd);
    if (err < 0) {
        throw mpm::os_error(str(boost::format("setting crossbar route failed! Error: %d") % err));
    }
}

void xbar_iface::del_route(uint8_t dst_addr, uint8_t dst_port){
    std::lock_guard<std::mutex> lock(_lock);
    rfnoc_crossbar_cmd cmd = {.dest_addr = dst_addr, .dest_port = dst_port};
    int err = ioctl(_fd, RFNCDELROUTIOC, &cmd);
    if (err < 0){
        throw mpm::os_error(str(boost::format("deleting crossbar route failed! Error: %d") % err));
    }
}

xbar_iface::sptr xbar_iface::make(const std::string &device){
    return std::make_shared<xbar_iface>(device);
}
