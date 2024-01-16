#!/usr/bin/env python3
#
# Copyright 2024 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
""" Test all python API functions for the connected device. """

import sys
import argparse
import uhd

from multi_usrp_test import MethodExecutor


# pylint: disable=broad-except

def time_test(clock, num_mboards):
    """
    Test if each Octoclock (which is equipped with GPS) returns a time value.

    The test is skipped on Octoclocks that are not equipped with GPS.

    clock -- Device object to run tests on.
    num_mboards -- Integer value of number of Octoclocks.
    """
    for mboard in range(0, num_mboards):
        sensor_names = clock.get_sensor_names(mboard)
        if 'gps_time' not in sensor_names:
            print(f'[Octo {mboard}] gps_time sensor is not available, skipping time_test')
            continue
        time = clock.get_time(mboard)
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
        sensor_names = clock.get_sensor_names(mboard)
        print(f'[Octo {mboard}] Got {len(sensor_names)} sensors.')

        for i, sensor_name in enumerate(sensor_names):
            sensor_value = clock.get_sensor(sensor_name, mboard)
            print(f'[{i}/{len(sensor_names)}] ({sensor_name}) {str(sensor_value)}')


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
