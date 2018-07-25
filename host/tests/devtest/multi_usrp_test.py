#!/usr/bin/env python
#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
""" Test all python API functions for the connected device. """
from __future__ import print_function
import sys
import argparse
import numpy
import uhd


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
        print("Executing {}".format(method_names))
        try:
            method_callback() is True
        except Exception as ex:
            print("Error while executing `{}`: {}".format(
                method_names, str(ex)
            ), file=sys.stderr)
            self.failed.extend(method_names)
            return False
        return True


def chan_test(usrp, prop, num_chans, error_handling, get_range,
              arg_converter=None):
    """
    Test methods that take channel number as input.
    usrp -- Device object to run tests on.
    prop -- String of function to be tested.
    num_chans -- Integer for number of channels.
    error_handling -- coerce or throw, depending on expected results.
    get_range -- String for the get_range function.
    arg_converter -- String for type to convert values to.
    """
    getter = 'get_{}'.format(prop)
    setter = 'set_{}'.format(prop)
    for chan in range(num_chans):
        # Test value below, above, and within range
        # If a get_range function is passed in:
        try:
            prop_range = getattr(usrp, get_range)(chan)
            min_val = prop_range.start() - 10
            max_val = prop_range.stop() + 10
        # get_range isn't a function, its a list.
        except TypeError:
            min_val = get_range[0] - 10
            max_val = get_range[1] + 10
        mid_point = (min_val + max_val) / 2

        # Values must be converted to TuneRequest type for these functions.
        arg_converter = arg_converter if arg_converter is not None \
            else lambda x: x
        min_val = arg_converter(min_val)
        max_val = arg_converter(max_val)
        mid_point = arg_converter(mid_point)
        # If setter is expected to throw errors.
        if error_handling == 'throw':
            # get a couple of values inside and outside range
            # apply using setter
            # read using getter
            # compare with expected behavior
            try:
                getattr(usrp, setter)(min_val, chan)
            except RuntimeError:
                pass
            else:
                raise Exception('error found in min test of ', prop)
            try:
                getattr(usrp, setter)(max_val, chan)
            except RuntimeError:
                pass
            else:
                raise Exception('error found in max test of ', prop)
            getattr(usrp, setter)(mid_point, chan)
        # If setter implements error coercion
        elif error_handling == 'coerce':

            getattr(usrp, setter)(min_val, chan)
            getattr(usrp, setter)(max_val, chan)

        # Set acceptable value.
        getattr(usrp, setter)(mid_point, chan)
        # Check if the actual value is within range of set value
        get_value = getattr(usrp, getter)(chan)
        get_value = float(get_value)
        mid_point = (prop_range.start() + prop_range.stop()) / 2
        if not numpy.isclose(get_value, mid_point, 0.005):
            raise Exception('error found in setting acceptable value in {}'.
                            format(prop))
    return True


def lo_name_test(usrp, prop, num_chans, get_range):
    """
    Test methods that an lo_name string as an argument.
    usrp -- Device object to run tests on.
    prop -- String of function to be tested.
    num_chans -- Integer for number of channels.
    get_range -- String for the get_range function.
    """
    getter = 'get_{}'.format(prop)
    setter = 'set_{}'.format(prop)
    # For each channel, get lo_names.
    for chan in range(num_chans):
        if prop == 'tx_lo_freq':
            lo_names = getattr(usrp, 'get_tx_lo_names')(chan)
        else:
            lo_names = getattr(usrp, 'get_rx_lo_names')(chan)
        # For each lo_name, set a value below minimum,
        # above maximum, and within range.
        for lo_name in lo_names:
            prop_range = getattr(usrp, get_range)(lo_name, chan)
            min_val = prop_range.start() - 10
            max_val = prop_range.stop() + 10
            mid_point = (min_val + max_val) / 2
            try:
                getattr(usrp, setter)(min_val, chan)
            except RuntimeError:
                raise Exception('error found in min test of ', prop)
            try:
                getattr(usrp, setter)(max_val, chan)
            except RuntimeError:
                raise Exception('error found in max test of ', prop)
            getattr(usrp, setter)(mid_point, chan)
            # Check if the actual value is within range of set value
            get_value = getattr(usrp, getter)(chan)
            get_value = float(get_value)
            mid_point = (prop_range.start() + prop_range.stop()) / 2
            if not numpy.isclose(mid_point, get_value, 0.005):
                raise Exception('error found in setting acceptable value in ',
                                prop)
    return True


def range_test(usrp, prop, num_chans, error_handling=None,
               args_type='chan', arg_converter=None, get_range=None):
    """
    Function to perform range_tests using getrange, getters, and setters.
    usrp -- Device object to run tests on.
    prop -- String of function to be tested.
    num_chans -- Integer for number of channels.
    error_handling -- coercer or throw, depending on expected results.
    args_type -- The type of argument that must be passed into the function.
    arg_converter -- String for type to convert values to.
    get_range -- String for get_range function
    """
    assert error_handling in ('coerce', 'throw')
    if get_range is None:
        get_range = 'get_{}_range'.format(prop)
    if args_type == 'chan':
        to_ret = chan_test(usrp, prop, num_chans,
                           error_handling, get_range, arg_converter)
    else:
        to_ret = lo_name_test(usrp, prop, num_chans, get_range)
    return to_ret


def discrete_options_test(usrp, prop, num_chans,
                          error_handling=None):
    """
    Function to perform tests on methods that return list of possible discrete
    values (floats).
    usrp -- Device object to run tests on.
    prop -- String of function to be tested.
    num_chans -- Integer for number of channels.
    error_handling -- coercer or throw, depending on expected results.
    """
    assert error_handling in ('coerce', 'throw')

    get_range = 'get_{}s'.format(prop)
    # Generate all possible set values
    return chan_test(usrp, prop, num_chans, error_handling,
                     get_range)


def list_test(usrp, prop, error_handling=None):
    """
    Function to perform tests on methods that return lists of possible
    discrete values (strings).
    usrp -- Device object to run tests on.
    prop -- String of function to be tested.
    error_handling -- coercer or throw, depending on expected results.
    """
    assert error_handling in ('coerce', 'throw')
    # The following functions have different get_range functions.
    if 'gain_profile' in prop:
        get_range = 'get_{}_names'.format(prop)
    else:
        get_range = 'get_{}s'.format(prop)
    getter = 'get_{}'.format(prop)
    setter = 'set_{}'.format(prop)

    # lo_source function does not take int as argument
    names = getattr(usrp, get_range)(0) if 'lo_source' not in prop \
        else getattr(usrp, get_range)()
    # Try to set every possible value.
    for name in names:
        # GPSDO may not be connected.
        if name == 'gpsdo' or name == 'internal':
            continue
        try:
            getattr(usrp, setter)(name)
        except RuntimeError:
            raise Exception('error found in setting {} to {}'
                            .format(prop, name))
        # Check if get function returns set value.
        get_value = getattr(usrp, getter)(0)
        if get_value != name:
            raise Exception('Error in setting acceptable value in {}'
                            .format(prop))
    return True


def get_test(usrp, prop, num_chans):
    """
    For testing only get methods that have no set counterpart.
    usrp -- Device object to run tests on.
    prop -- String of function to be tested.
    num_chans -- Integer for number of channels.
    """
    getter = 'get_{}'.format(prop)
    for chan in range(num_chans):
        # Refresh dictionary for each channel
        dictionary = getattr(usrp, "get_usrp_rx_info")(chan) if "rx" in prop \
                    else getattr(usrp, "get_usrp_tx_info")(chan)
        value = getattr(usrp, getter)(chan) if prop != 'mboard_name' \
            else getattr(usrp, getter)(0)
        # get_mboard_name function maps to mboard_id field in dict.
        dict_value = dictionary['mboard_id'] if prop == 'mboard_name' \
            else dictionary[prop]
        if value != dict_value:
            raise Exception("value in dict is {} and callback value is {}"
                            .format(dictionary[prop], value))
    return True


def recv_num_samps(usrp):
    """
    Test recv_num_samps method.
    usrp -- Device object to run tests on.
    """
    num_samps = 1000
    rate = getattr(usrp, "get_rx_rate")()
    samples = getattr(usrp, "recv_num_samps")(num_samps, rate)
    # Check that number of samples returned is correct and no sample is 0.
    if len(samples[0]) != num_samps:
        raise Exception("Number of samples received is not number requested.")
    return True


def send_waveform(usrp):
    """
    Test send_waveform method.
    usrp -- Device object to run tests on.
    """
    rate = getattr(usrp, "get_rx_rate")()
    getattr(usrp, "send_waveform")(numpy.asarray([1, 0, 1, 0]), 5, rate)
    return True


def test_sensor_api(usrp, sensor_type, num_indices):
    """
    Test the sensor API. It consists of two API calls ("get_sensor_names" and
    "get_sensor"). It will read out all the sensors and print their values.

    This will work for the mboard and TX/RX sensors.
    usrp -- Device object to run tests on.
    sensor_type -- String containing "RX" or "TX"
    num_indices -- Integer denoting the number of sensors.
    """
    list_sensor_method = 'get_{}_sensor_names'.format(sensor_type)
    get_sensor_method = 'get_{}_sensor'.format(sensor_type)
    for sensor_index in range(num_indices):
        sensor_names = getattr(usrp, list_sensor_method)(sensor_index)
        for sensor_name in sensor_names:
            print("Reading sensor: {sensor_name} ({sensor_index}/{num_indices}): "
                  .format(
                      sensor_name=sensor_name,
                      sensor_index=sensor_index,
                      num_indices=num_indices,
                  ))
            sensor_value = getattr(usrp, get_sensor_method)(
                sensor_name,
                sensor_index
            )
            print(str(sensor_value))


def test_time(usrp, prop):
    """
    Perform tests on functions that take time as an argument.
    usrp -- Device object to run tests on.
    prop -- String of function to be tested.
    """
    time = getattr(usrp, 'get_time_last_pps')()
    time = time + 10
    getattr(usrp, prop)(time)
    return True


def set_subdev_spec(usrp, prop):
    """
    Function for performing test on set_subdev_spec methods.
    usrp -- Device object to run tests on.
    prop -- String of function to be tested.
    """
    spec = getattr(usrp, 'get_rx_subdev_spec')() if prop == \
        'set_rx_subdev_spec' else getattr(usrp, 'get_tx_subdev_spec')()
    getattr(usrp, prop)(spec)
    return True


def mboard_range_test(usrp, prop, num_mboards):
    """
    Execute tests on methods that need to be tested on a range of motherboards.
    usrp -- Device object to run tests on.
    prop -- String of function to be tested.
    num_mboards -- Integer value of number of motherboards.
    """
    for mboard in range(0, num_mboards):
        if getattr(usrp, prop)(mboard) is None:
            raise Exception("{} function with argument {} returns None".
                            format(prop, mboard))
    return True


def chan_range_test(usrp, prop, num_chans):
    """
    Execute tests on methods that need to be tested on a range of channels.
    usrp -- Device object to run tests on.
    prop -- String of function to be tested.
    num_chans -- Integer value of number of channels.
    """
    # The following functions do not require the mboard index.
    if prop == 'set_rx_iq_balance' or prop == 'set_tx_iq_balance':
        for chan in range(0, num_chans):
            getattr(usrp, prop)(1, chan)
    else:
        # Set rx_dc_offset and tx_dc_offset to 0 for testing purposes.
        for chan in range(0, num_chans):
            getattr(usrp, prop)(0, chan)
    return True


def gpio_attr_test(usrp, prop, num_mboards):
    """
    Perform tests for get_gpio_attr and set_gpio_attr.

    usrp -- Device object to run tests on.
    prop -- String of function to be tested.
    num_mboards -- Integer value of number of motherboards.
    """
    if prop == 'get_gpio_attr':
        for mboard in range(0, num_mboards):
            bank = getattr(usrp, 'get_gpio_banks')(mboard)
            getattr(usrp, prop)(bank[0], 'CTRL')
    elif prop == 'set_gpio_attr':
        if num_mboards > 0:
            bank = getattr(usrp, 'get_gpio_banks')(0)
            getattr(usrp, prop)(bank[0], 'CTRL', 0)
    return True


def iq_balance_test(usrp, prop, num_chans):
    """
    Function for testing rx and tx iq_balance functions.
    usrp -- Device object to run tests on.
    prop -- String of function to be tested.
    num_chans -- Integer value of number of channels.
    """
    for chan in range(num_chans):
        getattr(usrp, prop)(1, chan)
    return True


def filter_test(usrp, prop):
    """
    Test specifically for the get_filter function
    usrp -- Device object to run tests on.
    prop -- String of function to be tested.
    """
    filters = getattr(usrp, 'get_filter_names')()
    if getattr(usrp, prop)(filters[0]) is None:
        raise Exception("{} function with {} arguments returns None"
                        .format(prop, filters[0]))
    return True


def run_api_test(usrp):
    """
    Name functions to be tested.
    usrp -- device object to run tests on
    """
    num_rx_chans = usrp.get_rx_num_channels()
    num_tx_chans = usrp.get_tx_num_channels()

    num_mboards = usrp.get_num_mboards()

    method_executor = MethodExecutor()

    # Append functions already called or will be called implicitly.
    method_executor.tested.append('get_rx_num_channels')
    method_executor.tested.append('get_tx_num_channels')
    method_executor.tested.append('get_usrp_rx_info')
    method_executor.tested.append('get_usrp_tx_info')
    method_executor.tested.append('get_num_mboards')

    actual_tests = [
        (['get_rx_freq', 'set_rx_freq', 'get_rx_freq_range'],
         lambda: range_test(usrp, 'rx_freq', num_rx_chans, 'coerce',
                            uhd.types.TuneRequest)),
        (['get_rx_gain', 'set_rx_gain', 'get_rx_gain_range'],
         lambda: range_test(usrp, 'rx_gain', num_rx_chans, 'coerce')),
        (['get_rx_bandwidth', 'set_rx_bandwidth', 'get_rx_bandwidth_range'],
         lambda: range_test(usrp, 'rx_bandwidth', num_rx_chans, 'coerce')),
        (['get_rx_lo_freq', 'set_rx_lo_freq', 'get_rx_lo_freq_range'],
         lambda: range_test(usrp, 'rx_lo_freq', num_rx_chans, 'coerce',
                            'lo_name')),
        (['get_rx_rate', 'set_rx_rate', 'get_rx_rates'],
         lambda: discrete_options_test(usrp, 'rx_rate', num_rx_chans,
                                       'coerce')),

        (['get_time_source', 'set_time_source', 'get_time_source_names'],
         lambda: list_test(usrp, 'time_source', 'coerce')),
        (['get_clock_source', 'set_clock_source', 'get_clock_names'],
         lambda: list_test(usrp, 'clock_source', 'coerce')),
        (['get_rx_antenna', 'set_rx_antenna', 'get_rx_antenna_names'],
         lambda: list_test(usrp, 'rx_antenna', 'coerce')),
        (['get_tx_antenna', 'set_tx_antenna', 'get_tx_antenna_names'],
         lambda: list_test(usrp, 'tx_antenna', 'coerce')),
        (['get_rx_lo_source', 'set_rx_lo_source', 'get_rx_lo_source_names'],
         lambda: list_test(usrp, 'rx_lo_source', 'coerce')),
        (['get_normalized_rx_gain', 'set_normalized_rx_gain',
          'get_normalied_rx_gain_range'],
         lambda: range_test(usrp, 'normalized_rx_gain', num_rx_chans,
                            'coerce', [0, 1])),
        (['get_rx_subdev_name'],
         lambda: get_test(usrp, 'rx_subdev_name', num_rx_chans)),
        (['get_rx_subdev_spec'],
         usrp.get_rx_subdev_spec),

        (['get_tx_freq', 'set_tx_freq', 'get_tx_freq_range'],
         lambda: range_test(usrp, 'tx_freq', num_tx_chans, 'coerce',
                            uhd.types.TuneRequest)),
        (['get_tx_gain', 'set_tx_gain', 'get_tx_gain_range'],
         lambda: range_test(usrp, 'tx_gain', num_tx_chans, 'coerce')),
        (['get_tx_bandwidth', 'set_tx_bandwidth', 'get_tx_bandwidth_range'],
         lambda: range_test(usrp, 'tx_bandwidth', num_tx_chans, 'coerce')),
        (['get_tx_lo_freq', 'set_tx_lo_freq', 'get_tx_lo_freq_range'],
         lambda: range_test(usrp, 'tx_lo_freq', num_tx_chans, 'coerce',
                            'lo_name')),
        (['get_tx_rate', 'set_tx_rate', 'get_tx_rates'],
         lambda: discrete_options_test(usrp, 'tx_rate', num_rx_chans,
                                       'coerce')),
        (['get_tx_lo_source', 'set_tx_lo_source', 'get_tx_lo_names'],
         lambda: list_test(usrp, 'tx_lo_source', 'coerce')),
        (['get_normalized_tx_gain', 'set_normalized_tx_gain', 'get_normalized_tx_gain_range'],
         lambda: range_test(usrp, 'normalized_tx_gain', num_rx_chans,
                            'coerce', [0, 1])),
        (['get_tx_subdev_name'],
         lambda: get_test(usrp, "tx_subdev_name", num_tx_chans)),
        (['get_tx_subdev_spec'],
         usrp.get_tx_subdev_spec),
        (['get_mboard_name'],
         lambda: get_test(usrp, 'mboard_name', num_tx_chans)),
        (['recv_num_samps'],
         lambda: recv_num_samps(usrp)),
        (['send_waveform'],
         lambda: send_waveform(usrp)),
        (['get_tx_gain_profile', 'set_tx_gain_profile', 'get_tx_gain_profile_names'],
         lambda: list_test(usrp, 'tx_gain_profile', 'coerce')),
        (['get_rx_gain_profile', 'set_rx_gain_profile', 'get_rx_gain_profile_names'],
         lambda: list_test(usrp, 'rx_gain_profile', 'coerce')),
        (['get_master_clock_rate', 'set_master_clock_rate', 'get_master_clock_rate_range'],
         lambda: range_test(usrp, 'master_clock_rate', 1, 'throw')),
        (['get_mboard_sensor_names', 'get_mboard_sensor'],
         lambda: test_sensor_api(usrp, 'mboard', num_mboards)),
        (['get_tx_sensor_names', 'get_tx_sensor'],
         lambda: test_sensor_api(usrp, 'tx', num_tx_chans)),
        (['get_rx_sensor_names', 'get_rx_sensor'],
         lambda: test_sensor_api(usrp, 'rx', num_rx_chans)),
        (['set_time_next_pps'],
         lambda: test_time(usrp, "set_time_next_pps")),
        (['set_time_now'],
         lambda: test_time(usrp, "set_time_now")),
        (['set_rx_subdev_spec'],
         lambda: set_subdev_spec(usrp, "set_rx_subdev_spec")),
        (['set_tx_subdev_spec'],
         lambda: set_subdev_spec(usrp, "set_tx_subdev_spec")),
        (['get_gpio_banks'],
         lambda: mboard_range_test(usrp, "get_gpio_banks", num_mboards)),
        (['get_time_now'],
         lambda: mboard_range_test(usrp, "get_time_now", num_mboards)),
        (['get_time_last_pps'],
         lambda: mboard_range_test(usrp, "get_time_last_pps", num_mboards)),
        (['enumerate_registers'],
         lambda: mboard_range_test(usrp, "enumerate_registers", num_mboards)),
        (['set_rx_dc_offset'],
         lambda: chan_range_test(usrp, "set_rx_dc_offset", num_rx_chans)),
        (['set_tx_dc_offset'],
         lambda: chan_range_test(usrp, "set_tx_dc_offset", num_tx_chans)),
        (['set_rx_agc'],
         lambda: chan_range_test(usrp, "set_rx_agc", num_rx_chans)),
        (['get_gpio_attr'],
         lambda: gpio_attr_test(usrp, "get_gpio_attr", num_mboards)),
        (['set_gpio_attr'],
         lambda: gpio_attr_test(usrp, "set_gpio_attr", num_mboards)),
        (['get_fe_rx_freq_range'], usrp.get_fe_rx_freq_range),
        (['get_fe_tx_freq_range'], usrp.get_fe_tx_freq_range),
        (['get_filter_names'], usrp.get_filter_names),
        (['get_normalized_tx_gain'], usrp.get_normalized_tx_gain),
        (['get_pp_string'], usrp.get_pp_string),
        (['get_rx_antennas'], usrp.get_rx_antennas),
        (['get_rx_gain_names'], usrp.get_rx_gain_names),
        (['get_rx_lo_export_enabled'], usrp.get_rx_lo_export_enabled),
        (['get_rx_lo_names'], usrp.get_rx_lo_names),
        (['get_rx_lo_sources'], usrp.get_rx_lo_sources),
        (['get_time_sources'],
         lambda: mboard_range_test(usrp, "get_time_sources", num_mboards)),
        (['get_time_synchronized'], usrp.get_time_synchronized),
        (['get_tx_antennas'], usrp.get_tx_antennas),
        (['get_tx_gain_names'], usrp.get_tx_gain_names),
        (['get_tx_lo_export_enabled'], usrp.get_tx_lo_export_enabled),
        (['get_tx_lo_sources'], usrp.get_tx_lo_sources),
        (['set_rx_iq_balance'],
         lambda: iq_balance_test(usrp, "set_rx_iq_balance", num_rx_chans)),
        (['set_tx_iq_balance'],
         lambda: iq_balance_test(usrp, "set_tx_iq_balance", num_tx_chans)),
        (['get_clock_sources'],
         lambda: mboard_range_test(usrp, "get_clock_sources", num_mboards)),
        (['get_filter'],
         lambda: filter_test(usrp, "get_filter")),
        (['clear_command_time'], usrp.clear_command_time),
    ]

    white_listed = ['__class__', '__delattr__', '__dict__', '__doc__',
                    '__format__', '__getattribute__', '__hash__', '__init__',
                    '__module__', '__new__', '__reduce__', '__reduce_ex__',
                    '__repr__', '__setattr__', '__sizeof__', '__str__',
                    '__subclasshook__', '__weakref__',
                    'make',
                    'issue_stream_cmd',
                    'set_rx_lo_export_enabled',  # Not supported w/all devices.
                    'set_tx_lo_export_enabled',  # Not supported w/all devices.
                    'set_time_source_out',  # Not supported on all devices.
                    'get_register_info',  # Requires path to register
                    'set_clock_config',  # Requires clock_config_t input
                    'get_rx_stream', 'get_tx_stream',  # Require stream_args_t
                    # Need register path, but enumerate_registers returns None
                    'read_register', 'write_register',
                    'set_command_time',  # Time_spec_t required
                    'set_filter',  # Requires passing sptr into function
                    'set_user_register',  # Not always callable
                    'set_clock_source_out',
                    'get_tx_dboard_iface',
                    'get_rx_dboard_iface',
                    'set_time_unknown_pps',
                   ]
    success = True

    # ...then we can run them all through the executor like this:
    for method_names, test_callback in actual_tests:
        if not method_executor.execute_methods(method_names, test_callback):
            success = False

    untested = [test for test in dir(usrp) if test not in
                method_executor.tested and test not in white_listed]

    if method_executor.failed:
        print("The following API calls caused failures:")
        print(method_executor.failed)

    if untested:
        print("The following functions were not tested:")
        print(untested)

    return success


def parse_args():
    """
    Parse args.
    """
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--args',
        default='None'
    )
    return parser.parse_args()


def main():
    """
    Returns True on Success
    """
    args = parse_args()
    usrp = uhd.usrp.MultiUSRP(args.args)
    ret_val = run_api_test(usrp)
    if ret_val != 1:
        raise Exception("Python API Tester Received Errors")
    return ret_val

if __name__ == "__main__":
    exit(not main())
