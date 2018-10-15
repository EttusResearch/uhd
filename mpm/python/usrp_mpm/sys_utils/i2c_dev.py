#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Utilities for i2c lookups
"""

import pyudev

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
    if not parent:
        return None
    devices = list(context.list_devices(parent=parent, subsystem="i2c-dev"))
    properties = [dict(d.properties) for d in devices]
    # The i2c-adapter will have the lowest minor number
    # FIXME: This likely isn't API--It just happens to work as of this writing
    chosen = min([(int(p['MINOR']), p['DEVNAME']) for p in properties])
    return chosen[1]
