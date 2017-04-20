#
# Copyright 2017 Ettus Research (National Instruments)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

import pyudev
import os
from ..mpmlog import get_logger

def get_eeprom_path(address):
    """
    Return EEPROM device path for a given I2C address
    """
    context = pyudev.Context()
    parent = pyudev.Device.from_name(context, "platform", address)
    paths = [device.device_node if device.device_node is not None else device.sys_path
             for device in context.list_devices(parent=parent, subsystem="nvmem")]
    if len(paths) != 1:
        raise Exception("{0} paths to EEPROM found!".format(len(paths)))
    return paths[0] + "/nvmem"


def get_spidev_nodes(spi_master):
    """
    Return found spidev device paths for a given SPI master
    """
    context = pyudev.Context()
    parent = pyudev.Device.from_name(context, "platform", spi_master)
    paths = [device.sys_path
             for device in context.list_devices(parent=parent, subsystem="spidev")]
    return paths

def get_uio_node(uio_name):
    """
    Return found uio device path for a give parent name
    """
    context = pyudev.Context()
    paths = [device.sys_path
             for device in context.list_devices(subsystem="uio")]
    log = get_logger('get_uio_node')
    log.debug("get_uio_node")
    log.debug("got paths: %s", paths)
    for path in paths:
        with open(os.path.join(path, "maps", "map0", "name"), "r") as uio_file:
            name = uio_file.read()
        log.debug("uio_node name: %s", name.strip())
        if name.strip() == uio_name:
            with open(os.path.join(path, "maps", "map0", "size"), "r") as uio_file:
                size = uio_file.read()
            log.debug("uio_node size: %s", size.strip())
            log.debug("uio_node syspath: %s", path)
            # device = pyudev.Device.from_sys_path(context, path)
            log.debug("got udev device")
            log.debug("device_node: %s size: %s", "/dev/uio0", size.strip())
            return ("/dev/uio0", int(size.strip()))
    return ("", 0)
