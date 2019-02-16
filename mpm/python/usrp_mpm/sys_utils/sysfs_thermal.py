#
# Copyright 2017-2019 Ettus Research, A National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
"""
sysfs thermal sensors API
"""

import pyudev

def read_sysfs_sensors_value(sensor_type, data_probe, subsystem, attribute):
    """
    This function will return a list of all the float value of
    the sysfs thermal sensor subsystems and attribute

    Arguments:
    sensor_type -- Is "attribute" of udev.  This can be fpga-thermal-zone,
                   magnesium-db0-zone, croc-ec-thermal etc.
    data_probe -- is one of the attribute of that sensor. This can be 'temp' in
                  the case of thermal-zone or 'cur_state' in the case of a
                  cooling device.
    subsystem -- of the thermal sensor
    attribute -- matching attribute for the sensor e.g. 'type', 'name'
    """
    reading_sensors = [float(x.attributes.asstring(data_probe)) for x in pyudev.Context()
                       .list_devices(subsystem=subsystem)
                       .match_attribute(attribute, sensor_type)]
    return reading_sensors

def read_thermal_sensors_value(sensor_type, data_probe, subsystem='thermal', attribute='type'):
    """
    This function will return a list of all the float value of
    the thermal sensor subsystem = 'thermal' and type = sensor_type

    Arguments:
    sensor_type -- Is attribute "type" of udev.  This can be fpga-thermal-zone,
                   magnesium-db0-zone, croc-ec-thermal etc.
    data_probe -- is one of the attribute of that sensor. This can be 'temp' in
                  the case of thermal-zone or 'cur_state' in the case of a
                  cooling device.
    """
    return read_sysfs_sensors_value(sensor_type, data_probe, subsystem, attribute)

def read_thermal_sensor_value(sensor_type, data_probe, subsystem='thermal', attribute='type'):
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
    sensor_val = read_thermal_sensors_value(sensor_type, data_probe, subsystem, attribute)
    if not sensor_val:
        raise IndexError("No {} attribute found for {} sensor.".format(data_probe, sensor_type))
    return sensor_val[0]
