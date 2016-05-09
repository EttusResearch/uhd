#!/usr/bin/env python
#
# Copyright 2015-2016 Ettus Research LLC
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
