#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Utilities for interfacing with ectool
"""

import subprocess

def run_cmd(cmd):
    """Run ectool utility's command named cmd."""
    cmd_str = ' '.join(['ectool', cmd])
    try:
        output = subprocess.check_output(
            cmd_str,
            stderr=subprocess.STDOUT,
            shell=True,
        )
    except subprocess.CalledProcessError as ex:
        raise RuntimeError("Failed to execute {} ectool command".format(cmd))
    return output.decode("utf-8")

def get_num_fans():
    """ Run ectool utility's command pwmgetnumfans to get number of fans."""
    output = run_cmd('pwmgetnumfans')
    num_fans = [int(s) for s in output.split() if s.isdigit()][0]
    return num_fans

def get_fan_rpm():
    """Run ectool utility's command pwmgetfanrpm to get fan rpm."""
    num_fans = get_num_fans()
    if num_fans == 0:
        raise RuntimeError("Number of fans is zero.")
    output = run_cmd('pwmgetfanrpm')
    fan_rpm_info = [int(s) for s in output.split() if s.isdigit()]
    if len(fan_rpm_info) == 2 * num_fans:
        return {
            "fan{}".format(fan) : fan_rpm_info[fan * 2 + 1]
            for fan in range (0, num_fans)
        }
    else:
        raise RuntimeError("Error getting fan rpm using ectool, at least one fan" \
                            " may be stalled. Command output: {}".format(output))
