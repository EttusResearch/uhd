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

def get_eeprom_paths(address):
    """
    Return EEPROM device paths for a given I2C address
    """
    context = pyudev.Context()
    parent = pyudev.Device.from_name(context, "platform", address)
    paths = [device.device_node if device.device_node is not None else device.sys_path
             for device in context.list_devices(parent=parent, subsystem="nvmem")]
    return [os.path.join(x.encode('ascii'), 'nvmem') for x in paths]

def get_spidev_nodes(spi_master):
    """
    Return found spidev device paths for a given SPI master
    """
    context = pyudev.Context()
    parent = pyudev.Device.from_name(context, "platform", spi_master)
    paths = [device.device_node.encode('ascii')
             for device in context.list_devices(parent=parent, subsystem="spidev")]
    return paths

