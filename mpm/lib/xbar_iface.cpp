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
