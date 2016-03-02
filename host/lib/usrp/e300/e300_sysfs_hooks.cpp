//
// Copyright 2013-2014 Ettus Research LLC
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

#ifdef E300_NATIVE

#include <cstdio>
#include <cstdlib>
#include <string>
#include <fcntl.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <libudev.h>

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

#include <uhd/utils/msg.hpp>
#include <uhd/exception.hpp>

static const std::string E300_AXI_FPGA_SYSFS = "40000000.axi-fpga";
static const std::string E300_XDEV_SYSFS = "f8007000.ps7-dev-cfg";

std::string e300_get_sysfs_attr(const std::string &node, const std::string &attr)
{
    udev *udev;
    udev_enumerate *enumerate;
    udev_list_entry *devices, *dev_list_entry;
    udev_device *dev;
    std::string retstring;

    udev = udev_new();

    if (!udev) {
        throw uhd::lookup_error("Failed to get udev handle.");
    }

    enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_sysname(enumerate, node.c_str());
    udev_enumerate_scan_devices(enumerate);
    devices = udev_enumerate_get_list_entry(enumerate);

    udev_list_entry_foreach(dev_list_entry, devices)
    {
        const char *path;

        path = udev_list_entry_get_name(dev_list_entry);
        dev = udev_device_new_from_syspath(udev, path);

       retstring = udev_device_get_sysattr_value(dev, attr.c_str());

       udev_device_unref(dev);

       if (retstring.size())
           break;
    }

    udev_enumerate_unref(enumerate);
    udev_unref(udev);

    return retstring;
}

static bool e300_fpga_loaded_successfully(void)
{
    return boost::lexical_cast<bool>(e300_get_sysfs_attr(E300_XDEV_SYSFS, "prog_done"));
}

#include "e300_fifo_config.hpp"
#include <uhd/exception.hpp>

e300_fifo_config_t e300_read_sysfs(void)
{

    if (not e300_fpga_loaded_successfully())
    {
        throw uhd::runtime_error("E300 FPGA load failed!");
    }

    e300_fifo_config_t config;

    config.buff_length  = boost::lexical_cast<unsigned long>(
        e300_get_sysfs_attr(E300_AXI_FPGA_SYSFS, "buffer_length"));
    config.ctrl_length = boost::lexical_cast<unsigned long>(
        e300_get_sysfs_attr(E300_AXI_FPGA_SYSFS, "control_length"));
    config.phys_addr = boost::lexical_cast<unsigned long>(
        e300_get_sysfs_attr(E300_AXI_FPGA_SYSFS, "phys_addr"));

    return config;
}

#else //E300_NATIVE

#include "e300_fifo_config.hpp"
#include <uhd/exception.hpp>

e300_fifo_config_t e300_read_sysfs(void)
{
    throw uhd::assertion_error("e300_read_sysfs() !E300_NATIVE");
}

std::string e300_get_sysfs_attr(const std::string &, const std::string &)
{
    throw uhd::assertion_error("e300_sysfs_attr() !E300_NATIVE");
}

#endif //E300_NATIVE
