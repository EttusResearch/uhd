#
# Copyright 2017 Ettus Research, a National Instruments Company
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
from usrp_mpm.mpmlog import get_logger


class GPSDIface(object):
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

    def __enter__(self):
        self.open()
        self.watch_query()
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.stop_query()
        self.close()
        return exc_type is None

    def open(self):
        """Open the socket to GPSD"""
        self.gpsd_socket.connect(('localhost', 2947))
        self.log.trace("Connected to GPSD.")

    def close(self):
        """Close the socket"""
        self.gpsd_socket.close()
        self.log.trace("Closing the connection to GPSD.")

    def watch_query(self):
        """Send a WATCH command, which starts operation"""
        query_cmd = b'?WATCH={"enable":true}'
        self.gpsd_socket.sendall(query_cmd)
        self.log.trace("Sent query: {}".format(query_cmd))

    def poll_query(self):
        """Send a POLL command"""
        query_cmd = b'?POLL;'
        self.gpsd_socket.sendall(query_cmd)
        self.log.trace("Sent query: {}".format(query_cmd))

    def stop_query(self):
        """Send the command to stop operation"""
        query_cmd = b'?WATCH={"enable":false}'
        self.gpsd_socket.sendall(query_cmd)
        self.log.trace("Sent query: {}".format(query_cmd))

    def socket_read_line(self, timeout=60, interval=0.1):
        """
        Read from a socket until newline. If there was no newline until the timeout
        occurs, raise an error. Otherwise, return the line.
        """
        line = b''
        end_time = time.time() + timeout
        while time.time() < end_time:
            socket_ready = select.select([self.gpsd_socket], [], [], 0)[0]
            if socket_ready:
                next_char = self.gpsd_socket.recv(1)
                if next_char == b'\n':
                    return line.decode('ascii')
                line += next_char
            else:
                time.sleep(interval)
        raise RuntimeError("socket_read_line() exceeded read timeout!")

    def get_gps_info(self, resp_class='', timeout=60):
        """Convenience function for getting a response which contains a response class"""
        # Read results until we see one which contains the requested response class, ie TPV or SKY
        result = {}
        end_time = time.time() + timeout
        while not result.get(resp_class, {}):
            try:
                self.poll_query()
                json_result = self.socket_read_line(timeout=timeout)
                self.log.trace(
                    "Received JSON response: {}".format(json_result)
                )
                result = json.loads(json_result)
                self.log.trace(
                    "Keys in response: {}".format(result.keys())
                )
                if (resp_class == "") or (time.time() > end_time):
                    # If we don't have a response class filter, just return the first response
                    # or if we timeout
                    break
            except json.JSONDecodeError:
                # If we get an incomplete packet, this will trigger
                # In this case, just retry
                continue
        # Filter the result by resp_class or return the entire thing
        # In the filtered case, the result contains a list of 'resp_class' responses,
        # so we need to get one valid one.
        return result if (resp_class == "") else result.get(resp_class, [{}])[0]


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


if __name__ == "__main__":
    main()
