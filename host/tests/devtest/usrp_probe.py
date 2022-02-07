#!/usr/bin/env python3
#
# Copyright 2015-2016 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
""" Run uhd_find_devices and parse the output. """

import re
import subprocess

def get_usrp_list(device_filter=None, env=None):
    """ Returns a list of dicts that contain USRP info """
    try:
        cmd = ['uhd_find_devices']
        if device_filter is not None:
            cmd += ['--args', device_filter]
        output = subprocess.check_output(cmd, env=env, universal_newlines=True)
    except subprocess.CalledProcessError:
        return []
    split_re = "\n*-+\n-- .*\n-+\n"
    uhd_strings = re.split(split_re, output)
    result = []
    for uhd_string in uhd_strings:
        if not re.match("Device Address", uhd_string):
            continue
        this_result = {k: v for k, v in re.findall("    ([a-z_]+): (.*)", uhd_string)}
        if this_result.get('reachable') == "No":
            continue
        args_string = ""
        try:
            args_string = "type={},serial={}".format(this_result['type'], this_result['serial'])
        except KeyError:
            continue
        this_result['args'] = args_string
        result.append(this_result)
    return result

def get_num_chans(device_args=None, env=None):
    """ Returns a dictionary that contains the number of TX and RX channels """
    # First, get the tree
    try:
        cmd = ['uhd_usrp_probe', '--tree']
        if device_args is not None:
            cmd += ['--args', device_args]
        output = subprocess.check_output(cmd, env=env, universal_newlines=True)
    except subprocess.CalledProcessError:
        return {}

    # Now look through the tree for frontend names
    rx_channels = 0
    tx_channels = 0
    for line in output.splitlines():
        if re.match('.*/[rt]x_frontends/[0-9AB]+/name$',line):
            # Get the frontend name
            try:
                cmd = ['uhd_usrp_probe']
                if device_args is not None:
                    cmd += ['--args', device_args]
                cmd += ['--string', line]
                output = subprocess.check_output(cmd, env=env, universal_newlines=True)
                # Ignore unknown frontends
                if (output.find("Unknown") == -1):
                    # Increment respective count
                    if re.match('.*/rx_frontends/[0-9AB]+/name$', line):
                        rx_channels += 1
                    else:
                        tx_channels += 1
            except subprocess.CalledProcessError:
                pass

    # Finally, return the counts
    return {'tx': tx_channels, 'rx': rx_channels}

if __name__ == "__main__":
    print(get_usrp_list())
    print(get_usrp_list('type=x300'))
