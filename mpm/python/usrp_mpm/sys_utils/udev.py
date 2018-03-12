#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Utilities for udev lookups
"""

import os
import pyudev

def get_eeprom_paths(address):
    """
    Return list of EEPROM device paths for a given I2C address.
    If no device paths are found, an empty list is returned.
    """
    context = pyudev.Context()
    parent = pyudev.Device.from_name(context, "platform", address)
    paths = [d.device_node if d.device_node is not None else d.sys_path
             for d in context.list_devices(parent=parent, subsystem="nvmem")]
    if len(paths) == 0:
        return []
    # We need to sort this so 9-0050 comes before 10-0050 (etc.)
    maxlen = max((len(os.path.split(p)[1]) for p in paths))
    paths = sorted(
        paths,
        key=lambda x: "{:>0{maxlen}}".format(os.path.split(x)[1], maxlen=maxlen)
    )
    return [os.path.join(x, 'nvmem') for x in paths]

def get_spidev_nodes(spi_master):
    """
    Return list of spidev device paths for a given SPI master. If no valid paths
    can be found, an empty list is returned.
    """
    context = pyudev.Context()
    parent = pyudev.Device.from_name(context, "platform", spi_master)
    return [
        device.device_node
        for device in context.list_devices(parent=parent, subsystem="spidev")
    ]

