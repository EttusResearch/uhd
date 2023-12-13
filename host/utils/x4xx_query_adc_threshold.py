#!/usr/bin/env python3
#
# Copyright 2021 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

"""
This script queries the minimal threshold value that still detects
the muxed DAC tone on a X4xx device. The reported values may give an
indication about unexpectedly high path losses in the ADC/DAC chains.
Typical values will be roughly in the range between 4000 and 5000, but
it is more important to watch major differences between the individual
channels.
As X410 has gain stages which are set and reset during the operation
of this script, the results will not be completely reproducible while
for X440 this fits quite well.
"""

import argparse
import time
import sys
import uhd

def parse_args():
    """
    Parse the command line arguments
    """
    parser = argparse.ArgumentParser()
    parser.add_argument("-a", "--args", default="", type=str)
    return parser.parse_args()

def is_x410(usrp):
    """
    Returns True if executed on a X410
    """
    return usrp.get_mboard_name() == "x410"

def is_x440(usrp):
    """
    Returns True if executed on a X440
    """
    return usrp.get_mboard_name() == "x440"

def settings(usrp):
    """
    Returns the settings matching the device (X410 or X440)
    """
    if is_x410(usrp):
        return {
            "gain": 30,
            "rx_freq": 2.5e9,
            "tx_freq": 2.5e9
        }
    if is_x440(usrp):
        return {
            "gain": None,
            "rx_freq": 397.55e6,
            "tx_freq": 397.55e6
        }
    print("Unknown x4xx device")
    sys.exit(1)

def set_gain(usrp, gain, channel):
    """
    Sets the gain for X410
    """
    usrp.set_rx_gain_profile("default", channel)
    usrp.set_tx_gain_profile("default", channel)
    usrp.set_rx_gain(gain, channel)
    usrp.set_tx_gain(gain, channel)
    usrp.set_rx_gain_profile("table_noatr", channel)
    usrp.set_tx_gain_profile("table_noatr", channel)
    usrp.set_rx_gain(0b11, "TABLE", channel)
    usrp.set_rx_gain(0b11, "TABLE", channel)

def reset_gain(usrp, channel):
    """
    Resets the gain values for X410 after running this script
    """
    usrp.set_rx_gain_profile("default", channel)
    usrp.set_tx_gain_profile("default", channel)
    usrp.set_rx_gain(0, channel)
    usrp.set_tx_gain(0, channel)

def check(mc, board, channel, value):
    """
    Checks if the given threshold value returns a valid threshold status
    """
    mc.setup_threshold(board, channel, 0, "hysteresis", 100, value-100, value)
    time.sleep(0.01)
    result = mc.get_threshold_status(board, channel, 0)
    return (value, result)

def bisect(mc, board, channel):
    """
    This method finds the threshold from which on the threshold status will return True
    """
    bad = check(mc, board, channel, 16383)
    if bad[1]:
        return bad[0]
    good = check(mc, board, channel, 100)
    if not good[1]:
        return good[0]
    while (good[1] != bad[1]) and (abs(good[0] - bad[0]) > 1):
        upper = good[0]
        lower = bad[0]
        value = lower + (upper - lower) // 2
        result = check(mc, slot, channel, value)
        if result[1] :
            good = result
        else:
            bad = result
    return good[0] if good[1] else None

if __name__ == "__main__":
    print("Determine minimal threshold value to be detected on ADC")
    args = parse_args()
    usrp = uhd.usrp.MultiUSRP(args.args)
    settings = settings(usrp)
    mc = usrp.get_mpm_client()
    mc.set_dac_mux_data(0x7FFF, 0)
    ch_per_board = usrp.get_rx_num_channels() // 2
    print("Channel | Threshold")
    print("--------+----------")
    for i in range(usrp.get_rx_num_channels()):
        print(f"{i:7} | ", end="")
        if is_x410(usrp):
            set_gain(usrp, settings["gain"], i)
        usrp.set_rx_antenna("CAL_LOOPBACK", i)
        usrp.set_tx_antenna("CAL_LOOPBACK", i)
        usrp.set_rx_freq(settings["rx_freq"], i)
        usrp.set_tx_freq(settings["tx_freq"], i)
        slot = i // ch_per_board
        chan = i % ch_per_board
        mc.set_dac_mux_enable(slot, chan, 1)
        print(f"{bisect(mc, slot, chan):9}")
        mc.set_dac_mux_enable(slot, chan, 0)
        if is_x410(usrp):
            reset_gain(usrp, i)
