#
# Copyright 2017-2019 Ettus Research, A National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
"""
sysfs thermal sensors API
"""
import statistics

import pyudev

class HwmonTempSensors:
    """
    This class provides access to the temperature and fan sensors under the subsystem.
    It queries udev for devices under the subsystem, finds
    the one corresponding to the ChromeEC microcontroller,
    and reads the temperature and fan sensor values from it.
    """
    def __init__(self, log=None, subsystem="hwmon", dev_filter={}, sensor_list=["temp", "fan"]):
        self.context = pyudev.Context()
        self.sensor_map = dict()
        self.log = log
        self.subsystem = subsystem

        self.device = [x for x in self.context.list_devices(subsystem=subsystem, **dev_filter)]
        if len(self.device) == 0:
            raise RuntimeError(f"Could not find {subsystem} device with filter {dev_filter}")
        
        self.device=self.device[0]
        
        # create map of sensors -> tempX_label, tempY_input, fanZ_input
        for attr in self.device.attributes.available_attributes:
            if "temp" in sensor_list and "temp" in attr:
                if attr.startswith("temp") and attr.endswith("_label"):
                    label = self.device.attributes.get(attr).decode(errors="ignore")
                    input_attr = attr.replace("_label", "_input")
                elif attr.startswith("temp") and attr.endswith("_input"):
                    label = attr
                    input_attr = attr                
                elif attr.startswith("in_temp"):
                    label = attr
                    input_attr = attr
            elif "fan" in sensor_list and attr.startswith("fan") and attr.endswith("_input"):
                label = attr
                input_attr = attr
            else:
                continue
            try:
                # dummy read - this will fail with KeyError if either
                # - the input attribute is not available or
                # - input attribute is available but no data can be read from it
                #self.device.attributes.asint(input_attr)
                float(self.device.attributes.asstring(input_attr))
                self.sensor_map[label] = input_attr
            except KeyError:
                if self.log:
                    self.log.warning(f"Sensor {label} cannot be read")
            if self.log:
                self.log.trace(f"Sensor map: {self.sensor_map}")

    def print_sensors(self):
        """Print the sensor values"""
        for sensor_name, input_attr in self.sensor_map.items():
            if input_attr.startswith("temp"):
                temp_C = self.device.attributes.asint(input_attr) / 1000
                if self.log:
                    self.log.debug(f"{sensor_name} {input_attr}: {temp_C} C")
            elif input_attr.startswith("fan"):
                rpm = self.device.attributes.asint(input_attr)
                if self.log:
                    self.log.debug(f"{sensor_name} {input_attr}: {rpm} RPM")

    def read_thermal_sensor_value(self, sensor_names, reduce_fn=statistics.mean):
        """
        This function will return the float value of the thermal sensor.

        Arguments:
        sensor_name -- Is "attribute" of udev.  This can be fpga-thermal-zone,
                   magnesium-db0-zone, croc-ec-thermal etc.
        reduce_fn    -- callable applied to the list of temperatures
        """
        temps = []
        for sensor_name in sensor_names:
            if sensor_name not in self.sensor_map:
                raise KeyError(f"{self.subsystem} subsystem sensor {sensor_name} not found!")
            try:
                temp_C = self.device.attributes.asint(self.sensor_map[sensor_name]) / 1000.0
                temps.append(temp_C)
            except ValueError:
                if self.log:
                    self.log.warning("Error when converting temperature value.")
        return {
            "name": "temperature",
            "type": "REALNUM",
            "unit": "C",
            "value": str(reduce_fn(temps)),
        }

    def read_fan_sensor_value(self, fan_name):
        """
        This function will return the rpm value of the fan sensor.

        Arguments:
        fan_name -- Is "attribute" of udev.  This can be fpga-thermal-zone,
                   magnesium-db0-zone, croc-ec-thermal etc.
        """
        for sensor_name in self.sensor_map.keys():
            if sensor_name.startswith(fan_name):
                try:
                    rpm = self.device.attributes.asint(self.sensor_map[sensor_name])
                    if self.log:
                        self.log.debug(f"{sensor_name}: {rpm} RPM")
                except ValueError as ex:
                    if self.log:
                        self.log.warning(f"Error occurred when getting {fan_name} speed value: {ex}")
                return {
                    "name": "cooling fan",
                    "type": "INTEGER",
                    "unit": "rpm",
                    "value": str(rpm),
                }
        if self.log:
            self.log.warning(f"HWMON subsystem sensor '{fan_name}' not found.")

    def read_raw_sensor_value(self, sensor_name):
        """
        This function will return the raw integer value of the sensor.

        Arguments:
        sensor_name -- Is "attribute" of udev.  This can be fpga-thermal-zone,
                   magnesium-db0-zone, croc-ec-thermal etc.
        """
        raw_val = -1
        if sensor_name not in self.sensor_map:
            raise KeyError(f"{self.subsystem} subsystem sensor {sensor_name} not found!")
        try:
            raw_val = float(self.device.attributes.asstring(self.sensor_map[sensor_name]))
        except ValueError:
            if self.log:
                self.log.warning("Error when converting temperature value.")
        return raw_val

def main():
    import argparse
    from usrp_mpm.mpmlog import DEBUG, get_main_logger

    """ Main function for testing this system utility stand-alone."""
    parser = argparse.ArgumentParser(description="Test HwmonTempSensors")
    parser.add_argument(
        "--device",
        choices=["x4xx", "n3xx", "e32x", "e31x"],
        required=True,
        help="Target device type",
    )
    args = parser.parse_args()

    DEVICE_FILTER = {
        "n3xx"      : {"OF_NAME": "embedded-controller"},
        "x4xx"      : {"OF_NAME": "cros-ec"},
        "e32x"      : {"OF_NAME": "embedded-controller"},
        "e31x_hwmon": {"OF_NAME": "temp"},
        "e31x_iio"  : {"OF_NAME": "adc"},
    }

    log = get_main_logger()
    log.setLevel(DEBUG)

    if "e31x" not in args.device:
        dev_filter = DEVICE_FILTER[args.device]
        #log.debug(f"Using dev_filter: {dev_filter} for device: {args.device}")
        sensors = HwmonTempSensors(log=log, dev_filter=dev_filter, sensor_list=["temp", "fan"])
        sensors.print_sensors()
    if args.device == "x4xx":
        log.debug(f"RFSoC: {sensors.read_thermal_sensor_value(['RFSoC'])}")
        log.debug(f"fan1: {sensors.read_fan_sensor_value('fan1')}")
        log.debug(f"fan2: {sensors.read_fan_sensor_value('fan2')}")
    elif args.device == "n3xx":
        log.debug(f"fan1: {sensors.read_fan_sensor_value('fan1')}")
        log.debug(f"fan2: {sensors.read_fan_sensor_value('fan2')}")
        log.debug(f"TMP431_Internal: {sensors.read_thermal_sensor_value(['TMP431_Internal'])}")
        log.debug(f"TMP431_Remote: {sensors.read_thermal_sensor_value(['TMP431_Remote'])}")
    elif args.device == "e32x":
        temp_sensor_list =['TMP464_Internal',\
         'TMP464_Remote_1',\
         'TMP464_Remote_2',\
         'TMP464_Remote_3',\
         'TMP464_Remote_4',\
        ]
        for sensor in temp_sensor_list:
            log.debug(f"{sensor}: {sensors.read_thermal_sensor_value([sensor])}")
        log.debug(f"fan1: {sensors.read_fan_sensor_value('fan1')}")
    elif args.device == "e31x":
        dev_filter = DEVICE_FILTER["e31x_hwmon"]
        sensors_hwmon = HwmonTempSensors(log=None, subsystem="hwmon", dev_filter=dev_filter, sensor_list=["temp", "fan"])
        sensors_hwmon.print_sensors()
        temp_sensor_list =['temp1_input']
        for sensor in temp_sensor_list:
            log.debug(f"{sensor}: {sensors_hwmon.read_thermal_sensor_value([sensor])}")
        dev_filter = DEVICE_FILTER["e31x_iio"]
        sensors_iio = HwmonTempSensors(log=None, subsystem="iio", dev_filter=dev_filter, sensor_list=["temp"])
        temp_sensor_list =['in_temp0_raw', 'in_temp0_offset', 'in_temp0_scale']
        raw_val = [sensors_iio.read_raw_sensor_value(sensor) for sensor in temp_sensor_list]
        temp = (raw_val[0] + raw_val[1])*raw_val[2] / 1000
        log.debug(f'get_fpga_temp_sensor: {temp} C')
    else:
        log.debug(f"Unsupported device type: {args.device}")

    #log.debug(f"type {type(sensors)}")

if __name__ == "__main__":
    main()
