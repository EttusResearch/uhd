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

def _get_i2c_adapter_from_parent(parent, context):
    """
    Helper to get the i2c adapter from a given parent device.
    This logic is common for different device lookup methods.
    """
    if not parent:
        return None
    devices = list(context.list_devices(parent=parent, subsystem="i2c-dev"))
    properties = [dict(d.properties) for d in devices]
    # The i2c-adapter will have the lowest minor number
    # FIXME: This likely isn't API--It just happens to work as of this writing
    chosen = min([(int(p['MINOR']), p['DEVNAME']) for p in properties])
    return chosen[1]

def of_get_i2c_adapter(of_name):
    """
    Return bus adapter device for given device tree name. To use the OF_NAME
    for matching (as this function does), it must be a unique property in the
    device tree (and this must be a device-tree platform).

    The return value is a string, e.g. '/dev/i2c-0'. If nothing is found, it'll
    return None.
    """
    # If has i2c-dev, follow to grab i2c-%d node
    context = pyudev.Context()
    parents = list(context.list_devices(subsystem="i2c-adapter", OF_NAME=of_name))
    if len(parents) > 1:
        raise RuntimeError("Non-unique OF_NAME when getting i2c bus id")
    if len(parents) == 0:
        return None
    parent = parents[0]
    return _get_i2c_adapter_from_parent(parent, context)


def dt_symbol_get_i2c_bus(symbol):
    """
    Return i2c bus associated with the given device tree symbol

    The return value is a string, e.g. '/dev/i2c-0'. If nothing is found, it'll
    return None.
    """
    context = pyudev.Context()
    parent = get_device_from_dt_symbol(symbol, subsystem='i2c', context=context)
    return _get_i2c_adapter_from_parent(parent, context)


def sysname_get_i2c_adapter(sys_name):
    """
    Return bus adapter device for given device tree sys_name.

    The return value is a string, e.g. '/dev/i2c-0'. If nothing is found, it'll
    return None.
    """
    parent = sysname_get_i2c_parent(sys_name)
    return _get_i2c_adapter_from_parent(parent, pyudev.Context())

def sysname_get_i2c_parent(sys_name):
    """
    Returns the parent of an i2c adapter found by sys_name
    """
    # If has i2c-dev, follow to grab i2c-%d node
    context = pyudev.Context()
    parents = list(context.list_devices(subsystem="i2c-adapter", sys_name=sys_name))
    if len(parents) > 1:
        raise RuntimeError("Non-unique sys_name when getting i2c bus id")
    if len(parents) == 0:
        return None
    return parents[0]
