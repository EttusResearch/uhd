#!/usr/bin/env python
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
        output = subprocess.check_output(cmd, env=env)
    except subprocess.CalledProcessError:
        return []
    split_re = "\n*-+\n-- .*\n-+\n"
    uhd_strings = re.split(split_re, output)
    result = []
    for uhd_string in uhd_strings:
        if not re.match("Device Address", uhd_string):
            continue
        this_result = {k: v for k, v in re.findall("    ([a-z]+): (.*)", uhd_string)}
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

if __name__ == "__main__":
    print get_usrp_list()
    print get_usrp_list('type=x300')
