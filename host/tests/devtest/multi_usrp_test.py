#!/usr/bin/env python3
#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
""" Test all python API functions for the connected device. """

import sys
import argparse
import numpy
import uhd
try:
    from ruamel import yaml
except:
    import yaml

# pylint: disable=broad-except

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


def chan_test(usrp, prop, num_chans, error_handling, get_range, arg_converter=None):
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
        initial_value = getattr(usrp, getter)(chan)
        # Test value below, above, and within range
        # If a get_range function is passed in:
        try:
            prop_range = getattr(usrp, get_range)(chan)
            min_val = prop_range.start() - 10
            max_val = prop_range.stop() + 10
            mid_point = prop_range.clip((min_val + max_val) / 2, True)
        # get_range isn't a function, its a list.
        except TypeError:
            min_val = get_range[0] - 10
            max_val = get_range[1] + 10
            mid_point = (get_range[0] + get_range[1]) / 2

        # Values must be converted to TuneRequest type for these functions.
        arg_converter = arg_converter if arg_converter is not None \
            else lambda x: x
        min_val = arg_converter(min_val)
        max_val = arg_converter(max_val)
        # If setter is expected to throw errors, we set some invalid values and
        # verify we get an exception:
        if error_handling == 'throw':
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
        # If setter implements error coercion, then we should be able to set
        # invalid values without an exception occurring:
        elif error_handling == 'coerce':
            getattr(usrp, setter)(min_val, chan)
            getattr(usrp, setter)(max_val, chan)

        # Set acceptable value.
        getattr(usrp, setter)(arg_converter(mid_point), chan)
        # Check if the actual value is within range of set value
        get_value = float(getattr(usrp, getter)(chan))
        if not numpy.isclose(get_value, mid_point, 0.005):
            raise Exception(
                f'Error found in setting acceptable value in {prop}.\n'
                f'Expected {mid_point}, got {get_value}.')
        # Put it back the way we found it
        getattr(usrp, setter)(initial_value, chan)
    return True


def lo_name_test(usrp, prop, num_chans, get_range):
    """
    Test methods that have an lo_name string as an argument.
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
            initial_value = getattr(usrp, getter)(lo_name, chan)
            prop_range = getattr(usrp, get_range)(lo_name, chan)
            min_val = prop_range.start()
            max_val = prop_range.stop()
            mid_point = \
                prop_range.clip(
                    (prop_range.start() + prop_range.stop()) / 2, True)
            try:
                getattr(usrp, setter)(min_val, lo_name, chan)
            except RuntimeError:
                raise Exception('error found in min test of ', prop)
            try:
                getattr(usrp, setter)(max_val, lo_name, chan)
            except RuntimeError:
                raise Exception('error found in max test of ', prop)
            getattr(usrp, setter)(mid_point, lo_name, chan)
            # Check if the actual value is within range of set value
            get_value = float(getattr(usrp, getter)(lo_name, chan))
            if not numpy.isclose(mid_point, get_value, 0.005):
                raise Exception(
                    f'Error found in setting acceptable value in {prop} '
                    f'for LO {lo_name} on channel {chan}.\n'
                    f'Expected value: {mid_point} Actual: {get_value}')
            # Put it back the way we found it
            getattr(usrp, setter)(initial_value, lo_name, chan)
    return True


def range_test(usrp, prop, num_chans, error_handling=None,
               args_type='chan', arg_converter=None, get_range=None):
    """
    Function to perform range_tests using getrange, getters, and setters.
    usrp           -- Device object to run tests on.
    prop           -- String of function to be tested.
    num_chans      -- Integer for number of channels.
    error_handling -- coerce or throw, depending on expected results.
    args_type      -- The type of argument that must be passed into the function.
                      Possible values: 'chan' means the argument is a channel number.
                      'lo_name' means it is an LO name.
    arg_converter  -- String for type to convert values to.
    get_range      -- String for get_range function
    """
    assert error_handling in ('coerce', 'throw')
    assert args_type in ('chan', 'lo_name')
    if get_range is None:
        get_range = 'get_{}_range'.format(prop)
    if args_type == 'chan':
        to_ret = chan_test(usrp, prop, num_chans,
                           error_handling, get_range, arg_converter)
    else: # args_type == 'lo_name':
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
    return chan_test(usrp, prop, num_chans, error_handling, get_range)


def list_test(usrp, prop, error_handling, post_hook=None, safe_values=None):
    """
    Function to perform tests on methods that return lists of possible
    discrete values (strings).
    usrp -- Device object to run tests on.
    prop -- String of function to be tested.
    error_handling -- coercer or throw, depending on expected results.
    post_hook -- Callback to call unconditionally after executing test (e.g. to
                 reset something)
    safe_values -- A list of safe values that can be tested from the range.
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
        if safe_values and name not in safe_values:
            print(f"Skipping value `{name}' for prop {prop}, considered unsafe.")
            continue
        initial_value = getattr(usrp, getter)(0)
        try:
            getattr(usrp, setter)(name)
        except RuntimeError as ex:
            raise Exception('error found in setting {} to {} => {}'
                            .format(prop, name, str(ex)))
        # Check if get function returns set value.
        get_value = getattr(usrp, getter)(0)
        if get_value != name:
            raise Exception('Error in setting acceptable value in {}'
                            .format(prop))
        getattr(usrp, setter)(initial_value)
    if post_hook:
        post_hook()
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
        if "_info" in prop:
            if not "module_serial" in dictionary.keys() or dictionary["module_serial"] == "n/a":
                raise Exception("Module serial number missing")
        else:
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
    usrp.send_waveform(numpy.asarray([1, 0, 1, 0], dtype=numpy.complex64), 5, rate)
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
    if prop in ('set_rx_iq_balance', 'set_tx_iq_balance'):
        for chan in range(0, num_chans):
            getattr(usrp, prop)(1, chan)
    else:
        # Set rx_dc_offset and tx_dc_offset to 0 for testing purposes.
        for chan in range(0, num_chans):
            getattr(usrp, prop)(0, chan)
    return True


def gpio_attr_test(usrp, num_mboards):
    """
    Perform tests for get_gpio_attr and set_gpio_attr.

    usrp -- Device object to run tests on.
    num_mboards -- Integer value of number of motherboards.
    """
    for mboard in range(0, num_mboards):
        banks = usrp.get_gpio_banks(mboard)
        for bank in banks:
            value = usrp.get_gpio_attr(bank, 'CTRL')
            usrp.set_gpio_attr(bank, 'CTRL', value)
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


def filter_test(usrp, prop, num_chans):
    """
    Test specifically for the get_filter function
    usrp -- Device object to run tests on.
    prop -- String of function to be tested.
    """
    for chan in range(num_chans):
        filters = getattr(usrp, 'get_{}_filter_names'.format(prop))(chan)
        for filter_name in filters:
            # Read a filter object...
            filter_obj = getattr(usrp, 'get_{}_filter'.format(prop))(filter_name, chan)
            if filter_obj is None:
                raise Exception("Filter object for {} returns None"
                                .format(filter_name))
            # ... and write it back:
            try:
                getattr(usrp, 'set_{}_filter'.format(prop))(filter_name, filter_obj, chan)
            except RuntimeError as ex:
                if "can not be written" not in str(ex):
                    raise
    return True


def tree_test(usrp):
    """
    Test prop tree access
    """
    tree = usrp.get_tree()
    name = tree.access_str("/name").get()
    print("Property tree got name: " + name)
    return True


def power_test(usrp, direction, num_chans):
    """
    Test the power reference API. If we don't have a power cal API available,
    then we check it fails.
    """
    has_cb = getattr(usrp, f'has_{direction}_power_reference')
    get_range_cb = getattr(usrp, f'get_{direction}_power_range')
    get_cb = getattr(usrp, f'get_{direction}_power_reference')
    set_cb = getattr(usrp, f'set_{direction}_power_reference')
    for chan in range(num_chans):
        # pylint: disable=bare-except
        if not has_cb(chan):
            try:
                get_range_cb(chan)
            except:
                pass
            else:
                raise(f"get_{direction}_power_range({chan}): "
                      "Expected exception (no power cal), none occurred.")
            try:
                get_cb(chan)
            except:
                pass
            else:
                raise(f"get_{direction}_power_reference({chan}): "
                      "Expected exception (no power cal), none occurred.")
            try:
                set_cb(100, chan)
            except:
                pass
            else:
                raise(f"set_{direction}_power_reference({chan}): "
                      "Expected exception (no power cal), none occurred.")
            continue
        # pylint: enable=bare-except
        # Now check power API for reals:
        initial_value = getattr(usrp, f'get_{direction}_gain')(chan)
        # Test value below, above, and within range
        prop_range = get_range_cb(chan)
        min_val = prop_range.start() - 10
        max_val = prop_range.stop() + 10
        mid_point = prop_range.clip((min_val + max_val) / 2, True)
        # These should not throw
        set_cb(min_val, chan)
        set_cb(max_val, chan)
        # Set acceptable value in the middle
        set_cb(mid_point, chan)
        # Check if the actual value is within range of set value
        actual_value = get_cb(chan)
        # We'll allow a pretty big variance, the power coercion has a lot of
        # constraints
        if not numpy.isclose(actual_value, mid_point, 10):
            raise Exception(
                f'Error found in setting midpoint power value for {direction}.\n'
                f'Expected {mid_point}, got {actual_value}.')
        # Put it back the way we found it
        getattr(usrp, f'set_{direction}_gain')(initial_value, chan)
    return True

def run_api_test(usrp, device_config):
    """
    Name functions to be tested.
    usrp -- device object to run tests on
    device_config -- Dictionary that contains further configuration for various
                     tests.
    """
    num_rx_chans = device_config.get('num_rx_channels', usrp.get_rx_num_channels())
    num_tx_chans = device_config.get('num_tx_channels', usrp.get_tx_num_channels())
    num_mboards = usrp.get_num_mboards()

    method_executor = MethodExecutor()

    # Append functions already called or will be called implicitly.
    method_executor.tested.extend((
        'make',
        'issue_stream_cmd', # Gets called by recv_num_samps
        'get_rx_num_channels', # Got called above
        'get_tx_num_channels', # Got called above
        'get_num_mboards', #  Got called above
        'get_rx_stream',
        'get_tx_stream',  # Require stream_args_t
    ))
    method_executor.tested.extend(device_config.get('imply_tested', []))

    actual_tests = [
        (['get_usrp_rx_info'],
         lambda: get_test(usrp, 'usrp_rx_info', num_rx_chans)),
        (['get_usrp_tx_info'],
         lambda: get_test(usrp, 'usrp_tx_info', num_tx_chans)),
        (['get_rx_freq', 'set_rx_freq', 'get_rx_freq_range'],
         lambda: range_test(usrp, 'rx_freq', num_rx_chans, 'coerce',
                            arg_converter=uhd.types.TuneRequest)),
        (['get_rx_gain', 'set_rx_gain', 'get_rx_gain_range'],
         lambda: range_test(usrp, 'rx_gain', num_rx_chans, 'coerce')),
        (['get_rx_bandwidth', 'set_rx_bandwidth', 'get_rx_bandwidth_range'],
         lambda: range_test(usrp, 'rx_bandwidth', num_rx_chans, 'coerce')),
        (['get_rx_lo_freq', 'set_rx_lo_freq', 'get_rx_lo_freq_range'],
         lambda: range_test(usrp, 'rx_lo_freq', num_rx_chans, 'coerce',
                            args_type='lo_name')),
        (['get_rx_rate', 'set_rx_rate', 'get_rx_rates'],
         lambda: discrete_options_test(usrp, 'rx_rate', num_rx_chans,
                                       'coerce')),

        (['get_time_source', 'set_time_source', 'get_time_sources'],
         lambda: list_test(usrp, 'time_source', 'coerce',
                           safe_values=device_config.get('time_sources', ('internal')))),
        (['get_clock_source', 'set_clock_source', 'get_clock_sources'],
         lambda: list_test(usrp, 'clock_source', 'coerce',
                           safe_values=device_config.get('clock_sources', ('internal')))),
        (['get_sync_source', 'set_sync_source', 'get_sync_sources'],
         lambda: list_test(
             usrp, 'clock_source', 'coerce',
             safe_values=device_config.get('sync_sources', ({
                 'clock_source': 'internal',
                 'time_source': 'internal',
             })))),
        (['get_rx_antenna', 'set_rx_antenna', 'get_rx_antennas'],
         lambda: list_test(usrp, 'rx_antenna', 'coerce')),
        (['get_tx_antenna', 'set_tx_antenna', 'get_tx_antennas'],
         lambda: list_test(usrp, 'tx_antenna', 'coerce')),
        (['get_rx_lo_source', 'set_rx_lo_source', 'get_rx_lo_source_names'],
         lambda: list_test(usrp, 'rx_lo_source', error_handling='throw')),
        (['get_normalized_rx_gain', 'set_normalized_rx_gain'],
         lambda: range_test(usrp, 'normalized_rx_gain', num_rx_chans,
                            error_handling='throw', get_range=[0, 1])),
        (['get_rx_subdev_name'],
         lambda: get_test(usrp, 'rx_subdev_name', num_rx_chans)),
        (['get_rx_subdev_spec'],
         usrp.get_rx_subdev_spec),

        (['get_tx_freq', 'set_tx_freq', 'get_tx_freq_range'],
         lambda: range_test(usrp, 'tx_freq', num_tx_chans, 'coerce',
                            arg_converter=uhd.types.TuneRequest)),
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
        (['get_normalized_tx_gain', 'set_normalized_tx_gain'],
         lambda: range_test(usrp, 'normalized_tx_gain', num_rx_chans,
                            error_handling='throw', get_range=[0, 1])),
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
         lambda: range_test(usrp, 'master_clock_rate', 1, 'coerce')),
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
        (['set_rx_dc_offset'],
         lambda: chan_range_test(usrp, "set_rx_dc_offset", num_rx_chans)),
        (['set_tx_dc_offset'],
         lambda: chan_range_test(usrp, "set_tx_dc_offset", num_tx_chans)),
        (['set_rx_agc'],
         lambda: chan_range_test(usrp, "set_rx_agc", num_rx_chans)),
        (['get_gpio_attr', 'set_gpio_attr', 'get_gpio_banks'],
         lambda: gpio_attr_test(usrp, num_mboards)),
        (['get_fe_rx_freq_range'], usrp.get_fe_rx_freq_range),
        (['get_fe_tx_freq_range'], usrp.get_fe_tx_freq_range),
        (['get_pp_string'], usrp.get_pp_string),
        (['get_rx_gain_names'], usrp.get_rx_gain_names),
        (['get_rx_lo_export_enabled'], usrp.get_rx_lo_export_enabled),
        (['get_rx_lo_names'], usrp.get_rx_lo_names),
        (['get_rx_lo_sources'], usrp.get_rx_lo_sources),
        (['get_time_synchronized'], usrp.get_time_synchronized),
        (['get_tx_gain_names'], usrp.get_tx_gain_names),
        (['get_tx_lo_export_enabled'], usrp.get_tx_lo_export_enabled),
        (['get_tx_lo_sources'], usrp.get_tx_lo_sources),
        (['set_rx_iq_balance'],
         lambda: iq_balance_test(usrp, "set_rx_iq_balance", num_rx_chans)),
        (['set_tx_iq_balance'],
         lambda: iq_balance_test(usrp, "set_tx_iq_balance", num_tx_chans)),
        (['get_rx_filter', 'set_rx_filter', 'get_rx_filter_names'],
         lambda: filter_test(usrp, "rx", num_rx_chans)),
        (['get_tx_filter', 'set_tx_filter', 'get_tx_filter_names'],
         lambda: filter_test(usrp, "tx", num_rx_chans)),
        (['clear_command_time'], usrp.clear_command_time),
        (['get_tree'], lambda: tree_test(usrp)),
        (['has_rx_power_reference',
          'set_rx_power_reference',
          'get_rx_power_reference',
          'get_rx_power_range',
         ], lambda: power_test(usrp, 'rx', num_rx_chans),
        ),
        (['has_tx_power_reference',
          'set_tx_power_reference',
          'get_tx_power_reference',
          'get_tx_power_range',
         ], lambda: power_test(usrp, 'rx', num_rx_chans),
        ),
    ]

    # List of tests that we don't test, but that's OK.
    white_listed = ['set_rx_lo_export_enabled',  # Not supported w/all devices.
                    'set_tx_lo_export_enabled',  # Not supported w/all devices.
                    'set_time_source_out',  # Not supported on all devices.
                    'get_register_info',  # Requires path to register
                    # Need register path, but enumerate_registers returns None
                    'read_register', 'write_register',
                    'set_command_time',  # Time_spec_t required
                    'set_filter',  # Requires passing sptr into function
                    'set_user_register',  # Not always callable
                    'set_clock_source_out',
                    'get_tx_dboard_iface',
                    'get_rx_dboard_iface',
                    'set_time_unknown_pps',
                    'get_radio_control',
                    'get_mb_controller',
                    'get_mpm_client',
                   ]
    blacklist = device_config.get('skip', [])
    success = True

    # ...then we can run them all through the executor like this:
    for method_names, test_callback in actual_tests:
        if any(method in blacklist for method in method_names):
            continue
        if not method_executor.execute_methods(method_names, test_callback):
            success = False

    untested = [
        test + (" [blacklisted]" if test in blacklist else "") for test in dir(usrp)
        if test not in method_executor.tested \
                and test not in white_listed \
                and not test.startswith('_')
    ]

    if method_executor.failed:
        print("The following API calls caused failures:")
        print("* " + "\n* ".join(method_executor.failed))
    if untested:
        print("The following functions were not tested:")
        print("* " + "\n* ".join(untested))

    return success


def parse_args():
    """
    Parse args.
    """
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--args', default='',
    )
    parser.add_argument(
        '--dump-defaults',
        help="Specify a device type, and the default config will be dumped as YAML"
    )
    parser.add_argument(
        '--device-config',
        help="Specify path to YAML file to use as device config"
    )
    return parser.parse_args()


def get_device_config(usrp_type, device_config_path=None):
    """
    Return a device configuration object.
    """
    if device_config_path:
        with open(device_config_path, 'r') as yaml_f:
            return yaml.load(yaml_f)
    if usrp_type in ('B205mini', 'B200mini', 'B200', 'B210'):
        return {
            'skip': [
                'set_rx_lo_export_enabled',
                'set_tx_lo_export_enabled',
                'get_rx_lo_source',
                'set_rx_lo_source',
                'get_rx_lo_source_names',
                'get_tx_lo_source',
                'set_tx_lo_source',
                'get_tx_lo_names',
                'get_master_clock_rate',
                'set_master_clock_rate',
                'get_master_clock_rate_range',
                'get_gpio_attr',
                'set_gpio_attr',
                'get_gpio_banks',
            ],
        }
    if usrp_type == 'x410':
        return {
            'skip': [
                # No AGC on ZBX
                'set_rx_agc',
                # No IQ imbalance on ZBX
                'set_rx_iq_balance',
                'set_tx_iq_balance',
                # No DC offset on ZBX
                'set_rx_dc_offset',
                'set_tx_dc_offset',
                # No LO source control on ZBX
                'set_rx_lo_source',
                'set_tx_lo_source',
                'set_rx_lo_export_enabled',
                'set_tx_lo_export_enabled',
            ],
            'clock_sources': ['internal', 'mboard'],
        }
    if usrp_type == 'x440':
        return {
            'skip': [
                # No AGC on FBX
                'set_rx_agc',
                # No IQ imbalance on FBX
                'set_rx_iq_balance',
                'set_tx_iq_balance',
                # No DC offset on FBX
                'set_rx_dc_offset',
                'set_tx_dc_offset',
                # No LO source control on FBX
                'set_rx_lo_source',
                'set_tx_lo_source',
                'set_rx_lo_export_enabled',
                'set_tx_lo_export_enabled',
                # No Filters on FBX
                'get_rx_filter',
                'set_rx_filter',
                'get_rx_filter_names',
                'get_tx_filter',
                'set_tx_filter',
                'get_tx_filter_names',
                # No Gain control on FBX, getters return 0, setters do nothing
                'get_normalized_rx_gain',
                'set_normalized_rx_gain',
                'get_normalized_tx_gain',
                'set_normalized_tx_gain',
                # Changing clock or time source after device init throws an error
                'set_clock_source',
                'set_time_source',
            ],
            'clock_sources': ['internal', 'mboard'],
        }
    return {}

def dump_defaults(usrp_type):
    """
    Print the hard-coded defaults as YAML
    """
    defaults = get_device_config(usrp_type)
    print(yaml.dump(defaults, default_flow_style=False))

def main():
    """
    Returns True on Success
    """
    args = parse_args()
    if args.dump_defaults:
        dump_defaults(args.dump_defaults)
        return 0
    usrp = uhd.usrp.MultiUSRP(args.args)
    usrp_type = usrp.get_usrp_rx_info().get('mboard_id')
    device_config = get_device_config(usrp_type, args.device_config)
    ret_val = run_api_test(usrp, device_config)
    if ret_val != 1:
        raise Exception("Python API Tester Received Errors")
    return ret_val

if __name__ == "__main__":
    sys.exit(not main())
