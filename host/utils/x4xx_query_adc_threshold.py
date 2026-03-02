#!/usr/bin/env python3
#
# Copyright 2021 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

""" X4xx ADC Threshold Query.

This script queries the minimal threshold value that still detects
the muxed DAC tone on a X4xx device. The reported values may give an
indication about unexpectedly high path losses in the ADC/DAC chains.
Typical values will be roughly in the range between 4000 and 5000, but
it is more important to watch major differences between the individual
channels.
As X410 has gain stages which are set and reset during the operation
of this script, the results will not be completely reproducible while
for X440 this fits quite well. For I and Q tiles, the threshold value
randomly reports back very high values, so only the Real tile values
can be trusted completely, while I and Q tile threshold values may
need to be re-run multiple times to get a reliable measurement.
"""

import argparse
import sys
import time

import uhd


def parse_args():
    """Parse the command line arguments."""
    parser = argparse.ArgumentParser()
    parser.add_argument("-a", "--args", default="", type=str)
    return parser.parse_args()


def is_x410(usrp):
    """Returns True if executed on a X410."""
    return usrp.get_mboard_name() == "x410"


def is_x420(usrp):
    """Returns True if executed on a X420."""
    return usrp.get_mboard_name() == "x420"


def is_x440(usrp):
    """Returns True if executed on a X440."""
    return usrp.get_mboard_name() == "x440"


def get_mixmodes(usrp):
    """Returns the name of the mixer mode."""
    if is_x410(usrp) or is_x440(usrp):
        return ["Real"]
    if is_x420(usrp):
        return ["Real", "I", "Q"]
    print("Unknown x4xx device")
    sys.exit(1)


def settings(usrp):
    """Returns the settings matching the X4xx device."""
    if is_x410(usrp):
        return [{"gain": 30, "rx_freq": 2.5e9, "tx_freq": 2.5e9}]
    if is_x440(usrp):
        return [{"gain": None, "rx_freq": 397.55e6, "tx_freq": 397.55e6}]
    if is_x420(usrp):
        # Gain values taken from ADC self-cal auto-leveling
        return [
            {"gain": 27, "rx_freq": 397.55e6, "tx_freq": 397.55e6},
            {"gain": 25, "rx_freq": 1000e6, "tx_freq": 1397.55e6},
            {"gain": 25, "rx_freq": 1000e6, "tx_freq": 1397.55e6},
        ]
    print("Unknown x4xx device")
    sys.exit(1)


def set_gain(usrp, gain, channel):
    """Sets the gain for X410 and X420 (X440 has no gain stages)."""
    if is_x410(usrp):
        usrp.set_rx_gain_profile("default", channel)
        usrp.set_tx_gain_profile("default", channel)
        usrp.set_rx_gain(gain, channel)
        usrp.set_tx_gain(gain, channel)
        usrp.set_rx_gain_profile("table_noatr", channel)
        usrp.set_tx_gain_profile("table_noatr", channel)
        usrp.set_rx_gain(0b11, "TABLE", channel)
        usrp.set_rx_gain(0b11, "TABLE", channel)
    elif is_x420(usrp):
        radio = usrp.get_radio_control(channel)
        radio.poke32(0x810A4, 0)  # Switch to SW ATR mode
        radio.poke32(0x810A8, 3)  # Switch to TRX to have the chain active
        usrp.set_tx_gain(46, channel)  # Approx. 0 dB
        usrp.set_rx_gain(gain, channel)
        time.sleep(0.1)  # Let the gain settle


def reset_gain(usrp, channel):
    """Resets the gain values for X410 and X420 after running this script."""
    if is_x410(usrp):
        usrp.set_rx_gain_profile("default", channel)
        usrp.set_tx_gain_profile("default", channel)
        usrp.set_rx_gain(0, channel)
        usrp.set_tx_gain(0, channel)
    elif is_x420(usrp):
        usrp.set_tx_gain(0, channel)
        usrp.set_rx_gain(0, channel)
        radio = usrp.get_radio_control(channel)
        radio.poke32(0x810A4, 1)  # Switch back to Classic ATR mode


def check(mc, board, channel, mode, value):
    """Checks if the given threshold value returns a valid threshold status."""
    mc.setup_threshold(board, channel, mode, 0, "hysteresis", 10000, value - 100, value)
    time.sleep(0.01)
    result = mc.get_threshold_status(board, channel, mode, 0)
    return (value, result)


def bisect(mc, board, channel, mode):
    """This method finds the threshold from which on the threshold status will return True."""
    bad = check(mc, board, channel, mode, 16383)
    if bad[1]:
        return bad[0]
    good = check(mc, board, channel, mode, 100)
    if not good[1]:
        return good[0]
    while (good[1] != bad[1]) and (abs(good[0] - bad[0]) > 1):
        upper = good[0]
        lower = bad[0]
        value = lower + (upper - lower) // 2
        result = check(mc, board, channel, mode, value)
        if result[1]:
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
        for j, mode in enumerate(get_mixmodes(usrp)):
            CH_NAME = f"{i} {mode}"
            print(f"{CH_NAME:7} | ", end="")
            set_gain(usrp, settings[j]["gain"], i)
            usrp.set_rx_antenna("CAL_LOOPBACK", i)
            usrp.set_tx_antenna("CAL_LOOPBACK", i)
            usrp.set_rx_freq(settings[j]["rx_freq"], i)
            usrp.set_tx_freq(settings[j]["tx_freq"], i)
            slot = i // ch_per_board
            chan = i % ch_per_board
            # I DAC Mux needs to be enabled for both I and Q subchannels,
            # so just turn them all on (5: All)
            mc.set_dac_mux_enable(slot, chan, 1, 5)
            print(f"{bisect(mc, slot, chan, j):9}")
            mc.set_dac_mux_enable(slot, chan, 0, 5)
            reset_gain(usrp, i)
    if is_x420(usrp):
        # I and Q tiles tend to report random max threshold values. Therefore warn the user.
        print(
            "Ran on X420: I and Q may randomly report maximum threshold values. Re-run the script if necessary."
        )
