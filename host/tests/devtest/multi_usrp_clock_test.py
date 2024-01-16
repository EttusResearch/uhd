#!/usr/bin/env python3
#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
""" Test all python API functions for the connected device. """

import sys
import argparse
import uhd


# pylint: disable=broad-except

def time_test(clock, num_mboards):
    """
    Test if each Octoclock returs a time value.

    clock -- Device object to run tests on.
    num_mboards -- Integer value of number of Octoclocks.
    """
    for mboard in range(0, num_mboards):
        time = getattr(clock, 'get_time')(mboard)
        if time is None:
            raise Exception(f'get_time() function on Octoclock {mboard} returns None')
        else:
            print(f'[Octo {mboard}] Current time: {time}')
    return True


def test_sensor_api(clock, num_mboards):
    """
    Test the sensor API. It consists of two API calls ("get_sensor_names" and
    "get_sensor"). It will read out all the sensors and print their values
    for each octoclock.

    clock -- Device object to run tests on.
    num_mboards -- Integer value of number of Octoclocks.
    """

    for mboard in range(num_mboards):
        sensor_names = getattr(clock, 'get_sensor_names')(mboard)
        print(f'[Octo {mboard}] Got {len(sensor_names)} sensors.')

        for i, sensor_name in enumerate(sensor_names):
            sensor_value = getattr(clock, 'get_sensor')(sensor_name, mboard)
            print(f'[{i}/{len(sensor_names)}] ({sensor_name}) {str(sensor_value)}')


class MethodExecutor(object):
    """
    Object for executing tests. Handles returned errors and mantains
    tested & failed lists.
    """
    def __init__(self):
        self.tested = []
        self.failed = []

    def execute_methods(self, method_names, method_callback):
        """
        Execute API methods in 'method_names' by calling 'method_callback'.

        The callback must throw an exception if something went wrong. If no
        exception is thrown, the assumption is that all went fine.

        method_names -- List of methods that will be called.
        method_callback -- String containing the method to call.
        """
        self.tested.extend(method_names)
        method_names_str = ", ".join([method + "()" for method in method_names])
        print(f"Executing methods: {method_names_str}")
        try:
            method_callback()
        except Exception as ex:
            print(f"Error while executing methods: \n`{method_names_str}`:\n{ex}",
                  file=sys.stderr)
            self.failed.extend(method_names)
            return False
        return True


def run_api_test(clock):
    """
    Name functions to be tested.
    clock -- device object to run tests on
    """
    num_mboards = clock.get_num_boards()

    method_executor = MethodExecutor()

    method_executor.tested.extend(('make',
                                   'get_num_mboards', #  Got called above
    ))

    actual_tests = [
        (['get_pp_string'], clock.get_pp_string),
        (['get_sensor_names', 'get_sensor'],
         lambda: test_sensor_api(clock, num_mboards)),
        (['get_time'],
         lambda: time_test(clock, num_mboards)),
    ]

    success = True
    # ...then we can run them all through the executor like this:
    for method_names, test_callback in actual_tests:
        if not method_executor.execute_methods(method_names, test_callback):
            success = False

    if method_executor.failed:
        print("The following API calls caused failures:")
        print("* " + "\n* ".join(method_executor.failed))

    return success


def parse_args():
    """
    Parse args.
    """
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--args', default='addr=192.168.10.3',
    )
    return parser.parse_args()


def main():
    """
    Returns True on Success
    """
    args = parse_args()
    clock = uhd.usrp_clock.MultiUSRPClock(args.args)
    ret_val = run_api_test(clock)

    if ret_val != 1:
        raise Exception("Python API Tester Received Errors")
    return ret_val

if __name__ == "__main__":
    sys.exit(not main())
