#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Utilities for i2c lookups
"""

import pyudev
from usrp_mpm.sys_utils.udev import get_device_from_dt_symbol

def dt_symbol_get_i2c_bus(symbol: str):
    """
    Return the I2C bus for a given device tree symbol.
    
    :param symbol: The device tree symbol
    :type symbol: str
    :return: The I2C bus
    :rtype: Optional[pyudev.Device]
    """
    context = pyudev.Context()
    try:
        return get_device_from_dt_symbol(symbol, subsystem='i2c', context=context)
    except FileNotFoundError:
        return None


def i2c_bus_to_device_node(i2c_bus: pyudev.Device):
    """
    Return the i2c device node (e.g. "/dev/i2c-0") for a given
    I2C bus.
    
    :param i2c_bus: The I2C bus
    :type i2c_bus: pyudev.Device
    :return: The i2c-dev device node
    :rtype: Optional[str]
    """
    for child in i2c_bus.children:
        if child.subsystem == "i2c-dev":
            return child.device_node
    # Fallthrough: return None if no i2c-dev node was found
    return None


def dt_symbol_get_i2c_device_node(symbol: str):
    """
    Return the i2c device node (e.g. "/dev/i2c-0") for a given
    device tree symbol.
    
    :param symbol: The device tree symbol
    :type symbol: str
    :return: The i2c-dev device node
    :rtype: Optional[pyudev.Device]
    """
    i2c_bus = dt_symbol_get_i2c_bus(symbol)
    return i2c_bus_to_device_node(i2c_bus) if i2c_bus else None
