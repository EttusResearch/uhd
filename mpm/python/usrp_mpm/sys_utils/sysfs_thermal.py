#
# Copyright 2017 Ettus Research, A National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
"""
sysfs thermal sensors API
"""

import pyudev

def read_thermal_sensor_value(sensor_type, data_probe):
    """
    This function will return the float value of the thermal sensor
    which is under thermal subsystem.

    Arguments:
    sensor_type -- Is attribute "type" of udev.  This can be fpga-thermal-zone,
                   magnesium-db0-zone, etc.
    data_probe -- is one of the attribute of that sensor. This can be 'temp' in
                  the case of thermal-zone or 'cur_state' in the case of a
                  cooling device.
    """
    reading_sensors = [x.attributes.asint(data_probe) for x in pyudev.Context()
                       .list_devices(subsystem='thermal')
                       .match_attribute('type', sensor_type)]
    return reading_sensors[0]
