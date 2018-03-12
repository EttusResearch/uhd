//
// Copyright 2013-2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
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

#include <uhd/utils/log.hpp>
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

    config.buff_length  = std::stoul(
        e300_get_sysfs_attr(E300_AXI_FPGA_SYSFS, "buffer_length"));
    config.ctrl_length = std::stoul(
        e300_get_sysfs_attr(E300_AXI_FPGA_SYSFS, "control_length"));
    config.phys_addr = std::stoul(
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
