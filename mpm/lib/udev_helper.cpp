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

#include "mpm/udev_helper.hpp"
#include <uhd/exception.hpp>
#include <boost/format.hpp>
#include <boost/make_shared.hpp>
#include <boost/crc.hpp>
#include <utility>
#include <iostream>
#include <string>
#include <cstring>
#include <fstream>

using namespace mpm;

udev_helper::udev_helper(){
    _udev = udev_new();
    if (!_udev) {
        throw uhd::os_error("Failed to create udev!");
	}
    _enumerate = udev_enumerate_new(_udev);

}

udev_helper::~udev_helper(){
	udev_enumerate_unref(_enumerate);
	udev_unref(_udev);
}
std::string udev_helper::get_eeprom(const std::string &address){
    udev_list_entry *devices, *dev_list_entry;
	udev_device *dev, *parent;

    parent = udev_device_new_from_subsystem_sysname(_udev, "platform", address.c_str());
    if (parent == NULL){
        return std::string();
    }
    udev_enumerate_add_match_parent(_enumerate, parent);
    udev_enumerate_add_match_subsystem(_enumerate, "nvmem");
	udev_enumerate_scan_devices(_enumerate);

    devices = udev_enumerate_get_list_entry(_enumerate);
    if (devices == NULL){
        return std::string();
    }
    udev_list_entry_foreach(dev_list_entry, devices) {
        const char *path = NULL, *sys_path = NULL;
        path  = udev_list_entry_get_name(dev_list_entry);
        dev = udev_device_new_from_syspath(_udev, path);
        sys_path = udev_device_get_syspath(dev);
        udev_device_unref(dev);
        return "/sys" + std::string(sys_path) + "/nvmem";
    }
    return std::string();
}

std::vector<std::string> udev_helper::get_spidev_nodes(const std::string &spi_master){
    udev_list_entry *devices, *dev_list_entry;
    udev_device *dev, *parent;

    parent = udev_device_new_from_subsystem_sysname(_udev, "platform", spi_master.c_str());
    udev_enumerate_add_match_parent(_enumerate, parent);
    udev_enumerate_add_match_subsystem(_enumerate, "spidev");
    udev_enumerate_scan_devices(_enumerate);

    devices = udev_enumerate_get_list_entry(_enumerate);
    std::vector<std::string> found_dev_nodes;
    if (devices != NULL){
        udev_list_entry_foreach(dev_list_entry, devices){
            const char *path, *dev_node;
            path = udev_list_entry_get_name(dev_list_entry);
            dev = udev_device_new_from_syspath(_udev, path);
            dev_node = udev_device_get_devnode(dev);
            found_dev_nodes.push_back(std::string(dev_node));
            udev_device_unref(dev);
        }
    }
    return found_dev_nodes;
}
