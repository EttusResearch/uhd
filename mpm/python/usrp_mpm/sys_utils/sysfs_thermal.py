#
# Copyright 2017-2019 Ettus Research, A National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
"""
sysfs thermal sensors API
"""

import statistics
import traceback
import pyudev

class SysfsThermalSensors():
    """
    Provides access to IIO/thermal temperature sensors in the sysfs tree.

    During __init__ all IIO/thermal devices that expose in_temp_raw are enumerated
    once and their static attribute name are cached in sensor_map.
    Subsequent reads only fetch in_temp_raw from
    the already-known device object, mirroring the HwmonTempSensors pattern.
    """

    def __init__(self, log=None, subsystem="iio", attribute="name", data_probe=["in_temp_raw"]):
        self.log = log
        self.context = pyudev.Context()
        # sensor name -> {'device': pyudev.Device}
        self.sensor_map = {}
        cnt=1
        for device in self.context.list_devices(subsystem=subsystem):
            if not any([attr in device.attributes.available_attributes for attr in data_probe]):
                continue
            try:
                name = device.attributes.asstring(attribute)
                if name not in self.sensor_map:   
                    self.sensor_map[name] = {"device": device}
                else:
                    self.sensor_map[name+str(cnt)] = {"device": device}
                    cnt += 1
                    
                if self.log:
                    self.log.debug(f"Found {subsystem} temp sensor: {name}")
            except Exception as ex:
                if self.log:
                    self.log.warning(f"Could not parse {subsystem} sensor at {device.sys_path}: {str(ex)}")
        
        self.log.debug(f"Found {self.sensor_map}")

        if not self.sensor_map and self.log:
            self.log.warning(f"No {subsystem} temperature sensors found.")

    # ------------------------------------------------------------------
    # Instance method – uses cached sensor_map for fast repeated reads
    # ------------------------------------------------------------------

    def read_thermal_sensor_value(self, sensor_names, data_probe=None, reduce_fn=statistics.mean):
        """
        Read current temperature for each name in sensor_names.

        Uses sensor_map built during __init__: only in_temp_raw is fetched
        at call time; offset and scale are reused from the cache.
        Matches the HwmonTempSensors.read_thermal_sensor_value interface.

        Arguments:
        sensor_names -- iterable of IIO device name strings, e.g. ['RFSoC']
        reduce_fn    -- callable applied to the list of temperatures
        """
        temps = []
        try:
            for sensor_name in sensor_names:
                if sensor_name not in self.sensor_map:
                    raise KeyError(sensor_name)
                entry = self.sensor_map[sensor_name]
                # sysfs-bus-iio: temp_in_mC = (raw + offset) * scale
                # https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-bus-iio
                if data_probe is None:
                    data_probe = ["in_temp_raw"]
                    temp_raw    = float(entry["device"].attributes.asstring("in_temp_raw"))
                    temp_offset = float(entry["device"].attributes.asstring("in_temp_offset"))
                    temp_scale  = float(entry["device"].attributes.asstring("in_temp_scale"))
                    temp_in_deg_c = (temp_raw + temp_offset) * temp_scale / 1000
                else:
                    temp_in_deg_c = float(entry["device"].attributes.asstring(data_probe)) / 1000
                temps.append(temp_in_deg_c)
        except ValueError:
            if self.log:
                self.log.warning("Error when converting temperature value.")
            temps = [-1]
        except KeyError as ex:
            if self.log:
                self.log.warning(f"Can't read temp on thermal_zone {sensor_names}.")
            temps = [-1]
        return {
            "name": "temperature",
            "type": "REALNUM",
            "unit": "C",
            "value": str(reduce_fn(temps))}
    
    def read_fan_sensor_value(self, fan_name, data_probe):
        """
        This function will return the rpm value of the fan sensor.

        Arguments:
        fan_name -- Is "attribute" of udev.  This can be fpga-thermal-zone,
                   magnesium-db0-zone, croc-ec-thermal etc.
        """
        try:
            if fan_name not in self.sensor_map:
                raise KeyError(fan_name)
            entry = self.sensor_map[fan_name]
            rpm = entry["device"].attributes.asstring(data_probe)
            if self.log:
                self.log.debug(f"{fan_name}: {rpm} RPM")
        except ValueError as ex:
            if self.log:
                self.log.warning(f"Error occurred when getting {fan_name} speed value: {ex}")
            rpm = "-1"
        except KeyError as ex:
            if self.log:
                self.log.warning(f"Can't read {data_probe} on {fan_name}.")
            rpm = "-1"
        return {
            "name": "cooling fan",
            "type": "INTEGER",
            "unit": "rpm",
            "value": str(rpm),
        }

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
    reading_sensors = [
        float(x.attributes.asstring(data_probe))
        for x in pyudev.Context()
        .list_devices(subsystem=subsystem)
        .match_attribute(attribute,sensor_type)
    ]
    return reading_sensors

def main():
    temp = "-1"
    raw_val = {}
    data_probes = ["temp1_input"]
    for data_probe in data_probes:
        # read_sysfs_sensors_value(sensor_type, data_probe, subsystem, attribute)
        sensor_types = ["jc-42.4-temp","jc42"]
        for sensor_type in sensor_types:
            try:
                raw_val["temp1_input"] = read_sysfs_sensors_value(
                        sensor_type, data_probe, "hwmon", "name"
                    )[0]
                temp = str(raw_val["temp1_input"] / 1000)
                print(f'get_mb_temp_sensor: {temp} C')
            except IndexError:
                print(f"{sensor_type} sensor not found.")
            except Exception as e:
                print(f"{sensor_type} sensor not found or error occurred: {e}") 
                print(traceback.format_exc())

    data_probes = ["in_temp0_raw", "in_temp0_scale", "in_temp0_offset"]
    for data_probe in data_probes:
        raw_val[data_probe] = read_sysfs_sensors_value("xadc",
                                                       data_probe,
                                                       "iio",
                                                       "name")[0]
    temp = str(
        (raw_val["in_temp0_raw"] + raw_val["in_temp0_offset"])
        * raw_val["in_temp0_scale"]
        / 1000
    )
    print(f'get_fpga_temp_sensor: {temp} C')

if __name__ == "__main__":
    main()