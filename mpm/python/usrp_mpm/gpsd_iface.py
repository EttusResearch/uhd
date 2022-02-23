#
# Copyright 2017-2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
GPS service daemon (GPSd) interface class
"""

import socket
import json
import time
import select
import datetime
import math
import re
from usrp_mpm.mpmlog import get_logger

def _deg_to_dm(angle):
    """
    Convert a latitude or longitude from NMEA degrees to degrees minutes
    format (DDDmm.mm)
    """
    fraction_int_tuple = math.modf(angle)
    return fraction_int_tuple[1] * 100 + fraction_int_tuple[0] * 60

def _nmea_checksum(nmea_sentence):
    """Calculate the checksum for a NMEA data sentence"""
    checksum = 0
    if not nmea_sentence.startswith('$'):
        return checksum

    for character in nmea_sentence[1:]:
        checksum ^= ord(character)

    return checksum

def gpgga_from_tpv_sky(tpv_sensor_data, sky_sensor_data):
    """
    Turn a TPV and SKY sensor value dictionary into a GPGGA string
    """
    gpgga = "$GPGGA,"

    if 'time' in tpv_sensor_data:
        time_formatted = re.subn(r'\d{4}-\d{2}-\d{2}T(\d{2}):(\d{2}):(\d{2}\.?\d*)Z',
                                 r'\1\2\3,', tpv_sensor_data.get('time'))
        if time_formatted[1] == 1:
            gpgga += time_formatted[0]
        else:
            gpgga += ","
    else:
        gpgga += ","

    if 'lat' in tpv_sensor_data:
        latitude = tpv_sensor_data.get('lat')
        latitude_direction = 'N' if latitude > 0 else 'S'
        latitude = _deg_to_dm(abs(latitude))
        gpgga += "{:09.4f},{},".format(latitude, latitude_direction)
    else:
        gpgga += "0.0,S,"

    if 'lon' in tpv_sensor_data:
        longitude = tpv_sensor_data['lon']
        longitude_direction = 'E' if longitude > 0 else 'W'
        longitude = _deg_to_dm(abs(longitude))
        gpgga += "{:010.4f},{},".format(longitude, longitude_direction)
    else:
        gpgga += "0.0,W,"

    quality = 0
    if tpv_sensor_data['mode'] > 1:
        if tpv_sensor_data.get('status') == 2:
            quality = 2
        else:
            quality = 1
    gpgga += "{:d},".format(quality)

    if 'satellites' in sky_sensor_data:
        satellites_used = 0
        for satellite in sky_sensor_data['satellites']:
            if 'used' in satellite and satellite['used']:
                satellites_used += 1
        gpgga += "{:02d},".format(satellites_used)
    else:
        gpgga += ","

    if 'hdop' in sky_sensor_data:
        gpgga += "{:.2f},".format(sky_sensor_data['hdop'])
    else:
        gpgga += ","

    if 'alt' in tpv_sensor_data:
        gpgga += "{:2f},{},".format(tpv_sensor_data['alt'], 'M')
    else:
        gpgga += ",,"

    # separation data is not present in tpv or sky sensor data
    gpgga += ",,"

    # differential data is not present
    gpgga += ",,"

    gpgga += "*{:02X}".format(_nmea_checksum(gpgga))

    return gpgga

class GPSDIface:
    """
    Interface to the GPS service daemon (GPSd).

    The GPSDIface implementation can be used as a context manager, and GPSD results should be
    gotten with the get_gps_info() function. This will filter by the GPSD response class
    (resp_class) and return that class message. If no filter is provided, this function will return
    the first response (not counting the VERSION message).

    The MPM SKY sensors returns the first available response- there shouldn't be anything tricky
    about this. The MPM TPV sensor, however, returns an interesting value in the shortest time
    possible. If the GPSD has received a TPV report since we connected (and sent the start
    command), this function should return immediately. However, if no report is ready, the function
    waits until an interesting result is ready and returns that. This is achieved by discarding
    `mode=0` responses.
    """
    def __init__(self):
        # Make a logger
        try:
            self.log = get_logger('GPSDIface')
        except AssertionError:
            from usrp_mpm.mpmlog import get_main_logger
            self.log = get_main_logger('GPSDIface')
        # Make a socket to connect to GPSD
        self.gpsd_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.gpsd_sockfile = self.gpsd_socket.makefile(encoding='ascii')

    def __enter__(self):
        self.open()
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.disable_watch()
        self.close()
        return exc_type is None

    def open(self):
        """Open the socket to GPSD"""
        self.gpsd_socket.connect(('localhost', 2947))
        version_str = self.read_class("VERSION")
        self.enable_watch()
        self.log.trace("GPSD version: %s", version_str)

    def close(self):
        """Close the socket"""
        self.gpsd_socket.close()
        self.log.trace("Closing the connection to GPSD.")

    def enable_watch(self):
        """Send a WATCH command, which starts operation"""
        self.gpsd_socket.sendall(b'?WATCH={"enable":true};')
        self.log.trace(self.read_class("DEVICES"))
        self.log.trace(self.read_class("WATCH"))

    def poll_request(self, socket_timeout=60, num_retry=10):
        """Send a POLL command

        Raises
        ------
        json.JSONDecodeError
            If the data returned from GPSd cannot be decoded with JSON.
        RuntimeError
            If unsuccessfully connecting to GPSD within num_retry.
        """
        query_cmd = b'?POLL;'
        for _ in range(num_retry):
            try:
                self.gpsd_socket.sendall(query_cmd)
                return self.read_class("POLL", socket_timeout)
            except socket.error:
                self.log.warning("Reconnecting to GPSD.")
                try:
                    self.gpsd_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                    self.open()
                except socket.error:
                    self.log.warning("Error during GPSD reconnect.")
                    continue
        raise RuntimeError("Unsuccessfully connecting to GPSD  within {} tries".format(num_retry))

    def disable_watch(self):
        """Send the command to stop operation"""
        query_cmd = b'?WATCH={"enable":false};'
        self.gpsd_socket.sendall(query_cmd)

    def socket_read_line(self, timeout=60):
        """
        Read from a socket until newline. If there was no newline until the timeout
        occurs, raise an error. Otherwise, return the line.
        """
        old_timeout = self.gpsd_socket.gettimeout()
        self.gpsd_socket.settimeout(timeout)
        try:
            # get a file interface to socket, read until newline, and trim it
            return self.gpsd_sockfile.readline().strip()
        finally:
            self.gpsd_socket.settimeout(old_timeout)

    def read_class(self, class_name, socket_timeout=60):
        """return json data for spcecfic key of 'class'
        This function will read until socket timeout (or no data on socket)

        Raises
        ------
        json.JSONDecodeError
            If the data returned from GPSd cannot be decoded with JSON.
        """
        while True:
            json_result = json.loads(self.socket_read_line(socket_timeout))
            if json_result.get('class', '') == class_name:
                return json_result

    def get_gps_info(self, resp_class='', timeout=60):
        """
        This will do:
        - Poll gpsd for info until we get a response that contains the requested
          response class (tpv or sky)
        - If no response class is requested, just return the entire dictionary
        - Otherwise, filter out the response class value and return that
        - The return value is always a dictionary
        - If the request times out, we return an empty dictionary
        """
        result = {}
        end_time = time.time() + timeout
        while not result.get(resp_class):
            try:
                # Do poll request with socket timeout of 5s here.
                # It should not be that long, since GPSD should send POLL object promptly.
                result = self.poll_request(5)
                # If we don't have a response class filter, just return the
                # first response
                if not resp_class:
                    return result
                # If we time out, return nothing
                if time.time() > end_time:
                    self.log.warning(
                        "Timeout trying to get GPS info (response class `{}')"
                        .format(resp_class))
                    return {}
            except json.JSONDecodeError:
                # If we get an incomplete packet, this will trigger
                # In this case, just retry
                self.log.warning("JSON decode error: %s", result)
                continue
        # Filter the result by resp_class or return the entire thing
        # In the filtered case, the result contains a list of 'resp_class' responses,
        # so we need to get one valid one.
        return result.get(resp_class, [{}])[0]


class GPSDIfaceExtension:
    """
    Wrapper class that facilitates the 'extension' of a `context` object. The
    intention here is for a object to instantiate a GPSDIfaceExtension object,
    then call the `extend` method with `context` object as an argument. This
    will then add the GPSDIfaceExtension methods to the `context` object's
    methods. The `context` object can then call the convenience functions to
    retrieve GPS information from GPSd.
    For example:

    class foo:
        def __init__(self):
            self.gps_ext = GPSDIfaceExtension()
            self.gps_ext.extend(self)
            # The GPSDIfaceExtension methods are now registered with foo, so
            # we can call `get_gps_time`
            print(self.get_gps_time())
    """
    def __init__(self):
        self._gpsd_iface = GPSDIface()
        self._log = self._gpsd_iface.log
        self._initialized = False
        try:
            self._gpsd_iface.open()
            self._initialized = True
        except (ConnectionRefusedError, ConnectionResetError):
            self._log.warning(
                "Could not connect to GPSd! None of the GPS sensors will work!")

    def __del__(self):
        if self._initialized:
            self._gpsd_iface.close()

    def extend(self, context):
        """
        Register the GSPDIfaceExtension object's public function with `context`
        """
        new_methods = [method_name for method_name in dir(self)
                       if not method_name.startswith('_') \
                       and callable(getattr(self, method_name)) \
                       and method_name.endswith("sensor") \
                       and self._initialized]
        for method_name in new_methods:
            new_method = getattr(self, method_name)
            self._log.trace("%s: Adding %s method", context, method_name)
            setattr(context, method_name, new_method)
        return new_methods

    def get_gps_time_sensor(self):
        """
        Retrieve the GPS time using a TPV response from GPSd, and returns as a sensor dict.
        This returns a sensor dictionary on the second edge containing the latest second.
        For example, if we call this function at the gps time of 1.001s it will wait until
        just after 2.000s to return 2 second. This effect is similar to get gps time on
        the next edge of pps.
        """
        def parse_time(time_str):
            """parse a string of time in format of %Y-%m-%dT%H:%M:%S.%fZ
               return in unit second
            """
            time_dt = datetime.datetime.strptime(time_str, "%Y-%m-%dT%H:%M:%S.%fZ")
            epoch_dt = datetime.datetime(1970, 1, 1)
            return (time_dt - epoch_dt).total_seconds()
        # Read responses from GPSD until we get a non-trivial mode and until next second.
        gps_time_prev = 0
        while True:
            gps_info = self._gpsd_iface.get_gps_info(resp_class='tpv', timeout=15)
            gps_mode = gps_info.get("mode", 0)
            gps_time = parse_time(gps_info.get("time", ""))
            if gps_mode == 0:
                self._log.warning("GPSD reported invalid mode."
                                  "Return from GPSD is %s", gps_info)
                continue
            if gps_time_prev == 0:
                gps_time_prev = gps_time
                continue
            if int(gps_time) - int(gps_time_prev) >= 1:
                return {
                    'name': 'gps_time',
                    'type': 'INTEGER',
                    'unit': 'seconds',
                    'value': str(int(gps_time)),
                }

    def get_gps_tpv_sensor(self):
        """Get a TPV response from GPSd as a sensor dict"""
        self._log.trace("Polling GPS TPV results from GPSD")
        # Read responses from GPSD until we get a non-trivial mode
        while True:
            gps_info = self._gpsd_iface.get_gps_info(resp_class='tpv', timeout=15)
            self._log.trace("GPS info: {}".format(gps_info))
            if gps_info.get("mode", 0) > 0:
                break
        # Return the JSON'd results
        gps_tpv = json.dumps(gps_info)
        return {
            'name': 'gps_tpv',
            'type': 'STRING',
            'unit': '',
            'value': gps_tpv,
        }

    def get_gps_sky_sensor(self):
        """Get a SKY response from GPSd as a sensor dict"""
        self._log.trace("Polling GPS SKY results from GPSD")
        # Just get the first SKY result
        gps_info = self._gpsd_iface.get_gps_info(resp_class='sky', timeout=15)
        # Return the JSON'd results
        gps_sky = json.dumps(gps_info)
        return {
            'name': 'gps_sky',
            'type': 'STRING',
            'unit': '',
            'value': gps_sky,
        }

    def get_gps_gpgga_sensor(self):
        """Get GPGGA sensor data by parsing TPV and SKY sensor data"""
        self._log.trace("Polling GPS TPV and SKY results from GPSD")
        # Read responses from GPSD until we get both a SKY response and TPV
        # response in non-trivial mode
        while True:
            gps_info = self._gpsd_iface.get_gps_info(resp_class='', timeout=15)
            self._log.trace("GPS info: {}".format(gps_info))
            # Response types are 'list of dicts', but they can be empty lists so
            # we need to prepare for that:
            tpv_sensor_data = gps_info.get('tpv')
            sky_sensor_data = gps_info.get('sky')
            if tpv_sensor_data and sky_sensor_data:
                tpv_sensor_data = tpv_sensor_data[0]
                sky_sensor_data = sky_sensor_data[0]
                if tpv_sensor_data.get("mode", 0) > 0:
                    break
        return {
            'name': 'gpgga',
            'type': 'STRING',
            'unit': '',
            'value': gpgga_from_tpv_sky(tpv_sensor_data, sky_sensor_data),
        }

    def get_gps_lock(self):
        """
        Get the GPS lock status using the TPV data.

        The Jackson Labs GPS modules have a pin to query GPS lock, which is a
        better option.
        """
        if not self._initialized:
            self._log.warning("Cannot query GPS lock, GPSd not initialized!")
            return False
        # Read responses from GPSD until we get a non-trivial mode
        while True:
            gps_info = self._gpsd_iface.get_gps_info(resp_class='tpv', timeout=15)
            self._log.trace("GPS info: {}".format(gps_info))
            if gps_info.get("mode", 0) > 0:
                break
        # 2 == 2D fix, 3 == 3D fix.
        # https://gpsd.gitlab.io/gpsd/gpsd_json.html
        return gps_info.get("mode", 0) >= 2


def main():
    """Test functionality of the GPSDIface class"""
    # Do some setup
    import argparse

    def parse_args():
        """Parse the command-line arguments"""
        parser = argparse.ArgumentParser(description="Read messages from GPSD")
        parser.add_argument("--timeout", help="Timeout for the GPSD read", default=20)
        return parser.parse_args()

    args = parse_args()

    with GPSDIface() as gps_iface:
        result = gps_iface.get_gps_info(resp_class='', timeout=args.timeout)
        tpv_result = gps_iface.get_gps_info(resp_class='tpv', timeout=args.timeout)
        sky_result = gps_iface.get_gps_info(resp_class='sky', timeout=args.timeout)
    print("Sample result: {}".format(result))
    print("TPV: {}".format(tpv_result))
    print("SKY: {}".format(sky_result))
    gps_ext = GPSDIfaceExtension()
    for _ in range(10):
        print(gps_ext.get_gps_time_sensor().get('value'))

if __name__ == "__main__":
    main()
