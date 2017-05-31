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

import os
import pyudev
from ..mpmlog import get_logger

def get_eeprom_paths(address):
    """
    Return EEPROM device paths for a given I2C address
    """
    context = pyudev.Context()
    parent = pyudev.Device.from_name(context, "platform", address)
    paths = [d.device_node if d.device_node is not None else d.sys_path
             for d in context.list_devices(parent=parent, subsystem="nvmem")]
    # We need to sort this so 9-0050 comes before 10-0050 (etc.)
    maxlen = max((len(os.path.split(p)[1]) for p in paths))
    paths = sorted(
        paths,
        key=lambda x: "{:>0{maxlen}}".format(os.path.split(x)[1], maxlen=maxlen)
    )
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

